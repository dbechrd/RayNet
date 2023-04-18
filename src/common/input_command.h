#pragma once

struct InputCmd {
    uint32_t seq;
    bool north;
    bool west;
    bool south;
    bool east;
    bool fire;

    Vector2 GenerateMoveForce(float speed) const
    {
        Vector2 moveForce{};
        if (north) {
            moveForce.y -= 1.0f;
        }
        if (west) {
            moveForce.x -= 1.0f;
        }
        if (south) {
            moveForce.y += 1.0f;
        }
        if (east) {
            moveForce.x += 1.0f;
        }
        if (moveForce.x && moveForce.y) {
            float invLength = 1.0f / sqrtf(moveForce.x * moveForce.x + moveForce.y * moveForce.y);
            moveForce.x *= invLength;
            moveForce.y *= invLength;
        }

        moveForce.x *= speed;
        moveForce.y *= speed;
        return moveForce;
    }
};