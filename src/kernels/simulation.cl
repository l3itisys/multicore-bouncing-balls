typedef struct {
    float2 position;
    float2 velocity;
    float radius;
    float mass;
    uint color;
    uint padding;
} Ball;

typedef struct {
    float dt;
    float gravity;
    float restitution;
    float padding;
    float2 screenDimensions;
    float2 reserved;
} SimConstants;

__kernel void updateBallPhysics(
    __global Ball* balls,
    __constant SimConstants* constants,
    const int numBalls
) {
    int i = get_global_id(0);
    if (i >= numBalls) return;

    Ball ball = balls[i];
    float dt = constants->dt;
    float gravity = constants->gravity;
    float2 screenDim = constants->screenDimensions;

    // Update velocity and position
    ball.velocity.y += gravity * dt;  // Add gravity (positive Y is down)
    ball.position.x += ball.velocity.x * dt;
    ball.position.y += ball.velocity.y * dt;

    // Boundary collision with proper position clamping
    float restitution = constants->restitution;

    // Horizontal boundaries
    if (ball.position.x - ball.radius < 0.0f) {
        ball.position.x = ball.radius;
        ball.velocity.x = fabs(ball.velocity.x) * restitution;
    }
    else if (ball.position.x + ball.radius > screenDim.x) {
        ball.position.x = screenDim.x - ball.radius;
        ball.velocity.x = -fabs(ball.velocity.x) * restitution;
    }

    // Vertical boundaries
    if (ball.position.y - ball.radius < 0.0f) {
        ball.position.y = ball.radius;
        ball.velocity.y = fabs(ball.velocity.y) * restitution;
    }
    else if (ball.position.y + ball.radius > screenDim.y) {
        ball.position.y = screenDim.y - ball.radius;
        ball.velocity.y = -fabs(ball.velocity.y) * restitution;
    }

    balls[i] = ball;
}

__kernel void detectCollisions(
    __global Ball* balls,
    __constant SimConstants* constants,
    const int numBalls
) {
    int gid = get_global_id(0);
    if (gid >= numBalls) return;

    Ball myBall = balls[gid];
    float restitution = constants->restitution;

    // Check collisions with other balls
    for (int j = 0; j < numBalls; j++) {
        if (j == gid) continue;

        Ball otherBall = balls[j];
        float2 diff = otherBall.position - myBall.position;
        float distSq = dot(diff, diff);
        float minDist = myBall.radius + otherBall.radius;

        if (distSq < minDist * minDist && distSq > 0.0f) {
            float dist = sqrt(distSq);
            float2 normal = diff / dist;

            // Calculate relative velocity
            float2 relativeVel = otherBall.velocity - myBall.velocity;
            float velAlongNormal = dot(relativeVel, normal);

            // Only resolve if balls are moving toward each other
            if (velAlongNormal < 0) {
                float j = -(1.0f + restitution) * velAlongNormal;
                j /= 1.0f/myBall.mass + 1.0f/otherBall.mass;

                float2 impulse = j * normal;
                myBall.velocity -= impulse / myBall.mass;
            }
        }
    }

    balls[gid] = myBall;
}

