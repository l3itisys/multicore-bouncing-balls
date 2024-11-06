// Data structures matching CPU-side definitions
typedef struct {
    float2 position;
    float2 velocity;
    float radius;
    float mass;
    int color;
    int padding;
} Ball;

typedef struct {
    float dt;
    float gravity;
    float restitution;
    float padding;
    float2 screenDimensions;
    float2 reserved;
} SimConstants;

// Helper functions
float2 make_float2(float x, float y) {
    return (float2)(x, y);
}

float length_squared(float2 v) {
    return dot(v, v);
}

// Physics update kernel - multiple threads per ball for better parallelism
__kernel void updateBallPhysics(
    __global Ball* balls,
    __constant SimConstants* constants,
    const int numBalls
) {
    int gid = get_global_id(0);
    int localId = get_local_id(0);
    int groupId = get_group_id(0);
    int localSize = get_local_size(0);
    
    // Each workgroup handles one ball's physics
    int ballIndex = groupId;
    if (ballIndex >= numBalls) return;

    // Load ball data into local memory for faster access
    __local Ball localBall;
    if (localId == 0) {
        localBall = balls[ballIndex];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Update velocity (gravity) - only one thread per workgroup
    if (localId == 0) {
        localBall.velocity.y += constants->gravity * constants->dt;
        localBall.position += localBall.velocity * constants->dt;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Handle boundaries with restitution
    if (ball.position.x - ball.radius < 0.0f) {
        ball.position.x = ball.radius;
        ball.velocity.x = fabs(ball.velocity.x) * constants->restitution;
    }
    else if (ball.position.x + ball.radius > constants->screenDimensions.x) {
        ball.position.x = constants->screenDimensions.x - ball.radius;
        ball.velocity.x = -fabs(ball.velocity.x) * constants->restitution;
    }

    if (ball.position.y - ball.radius < 0.0f) {
        ball.position.y = ball.radius;
        ball.velocity.y = fabs(ball.velocity.y) * constants->restitution;
    }
    else if (ball.position.y + ball.radius > constants->screenDimensions.y) {
        ball.position.y = constants->screenDimensions.y - ball.radius;
        ball.velocity.y = -fabs(ball.velocity.y) * constants->restitution;
    }

    balls[gid] = ball;
}

// Pixel update kernel - multiple threads for rendering
__kernel void updateBallPixels(
    __global Ball* balls,
    __global uchar4* pixelBuffer,
    __constant SimConstants* constants,
    const int width,
    const int height,
    const int numBalls
) {
    int gid = get_global_id(0);
    int totalThreads = get_global_size(0);

    // Each thread processes multiple pixels
    for (int ballIndex = 0; ballIndex < numBalls; ballIndex++) {
        Ball ball = balls[ballIndex];

        // Calculate ball's bounding box
        int minX = max(0, (int)(ball.position.x - ball.radius));
        int maxX = min(width - 1, (int)(ball.position.x + ball.radius));
        int minY = max(0, (int)(ball.position.y - ball.radius));
        int maxY = min(height - 1, (int)(ball.position.y + ball.radius));

        // Calculate total pixels in bounding box
        int totalPixels = (maxX - minX + 1) * (maxY - minY + 1);

        // Divide work among threads
        int pixelsPerThread = (totalPixels + totalThreads - 1) / totalThreads;
        int startPixel = gid * pixelsPerThread;
        int endPixel = min(startPixel + pixelsPerThread, totalPixels);

        // Process assigned pixels
        for (int p = startPixel; p < endPixel; p++) {
            int x = minX + (p % (maxX - minX + 1));
            int y = minY + (p / (maxX - minX + 1));

            // Check if pixel is inside ball
            float dx = x - ball.position.x;
            float dy = y - ball.position.y;
            float distSquared = dx * dx + dy * dy;

            if (distSquared <= ball.radius * ball.radius) {
                int pixelIndex = y * width + x;
                uchar4 color = (uchar4)(
                    ((ball.color >> 16) & 0xFF),
                    ((ball.color >> 8) & 0xFF),
                    (ball.color & 0xFF),
                    128  // Semi-transparent
                );
                pixelBuffer[pixelIndex] = color;
            }
        }
    }
}

// Collision detection and resolution kernel
__kernel void detectCollisions(
    __global Ball* balls,
    __constant SimConstants* constants,
    const int numBalls
) {
    int gid = get_global_id(0);
    int numThreads = get_global_size(0);

    // Calculate work distribution for collision pairs
    int totalPairs = (numBalls * (numBalls - 1)) / 2;
    int pairsPerThread = (totalPairs + numThreads - 1) / numThreads;
    int startPair = gid * pairsPerThread;
    int endPair = min(startPair + pairsPerThread, totalPairs);

    // Process assigned collision pairs
    for (int pair = startPair; pair < endPair; pair++) {
        // Convert pair index to ball indices
        int i = 0;
        int temp = pair;
        while (temp >= numBalls - 1 - i) {
            temp -= numBalls - 1 - i;
            i++;
        }
        int j = temp + i + 1;

        // Ensure i and j are within bounds
        if (i >= numBalls || j >= numBalls) continue;

        Ball ball1 = balls[i];
        Ball ball2 = balls[j];

        // Check for collision
        float2 delta = ball2.position - ball1.position;
        float distSquared = dot(delta, delta);
        float minDist = ball1.radius + ball2.radius;

        if (distSquared < minDist * minDist) {
            float distance = sqrt(distSquared);
            if (distance < 0.0001f) {
                delta = make_float2(minDist, 0.0f);
                distance = minDist;
            }

            // Calculate collision normal
            float2 normal = delta / distance;

            // Calculate relative velocity
            float2 relativeVel = ball2.velocity - ball1.velocity;
            float velAlongNormal = dot(relativeVel, normal);

            // Only resolve if balls are moving toward each other
            if (velAlongNormal < 0) {
                // Calculate impulse
                float impulseMagnitude = -(1.0f + constants->restitution) * velAlongNormal;
                impulseMagnitude /= (1.0f / ball1.mass + 1.0f / ball2.mass);

                // Apply impulse
                float2 impulse = impulseMagnitude * normal;
                ball1.velocity -= impulse / ball1.mass;
                ball2.velocity += impulse / ball2.mass;

                // Position correction (avoid sinking)
                const float percent = 0.8f;
                float2 correction = (max(minDist - distance, 0.0f) /
                                  (1.0f / ball1.mass + 1.0f / ball2.mass)) *
                                  percent * normal;

                ball1.position -= correction / ball1.mass;
                ball2.position += correction / ball2.mass;

                // Write back updated balls
                balls[i] = ball1;
                balls[j] = ball2;
            }
        }
    }
}

