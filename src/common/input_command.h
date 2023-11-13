#pragma once
#include "common.h"

struct InputCmd {
    uint32_t seq;
    Vector2 facing;
    bool north;
    bool west;
    bool south;
    bool east;
    bool fire;
    double sampledAt;  // client time for debug

    Vector3 GenerateMoveForce(float speed) const
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
            moveForce = Vector2Normalize(moveForce);
        }

        moveForce.x *= speed;
        moveForce.y *= speed;
        return { moveForce.x, moveForce.y, 0 };
    }
};