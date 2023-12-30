#pragma once
#include "common.h"

struct InputCmd {
    uint8_t seq;
    uint8_t facing;
    bool north;
    bool west;
    bool south;
    bool east;
    bool fire;

    static uint8_t QuantizeFacing(Vector2 dir)
    {
        // -pi to +pi radians (from 12 o'clock)
        float rads = Vector2Angle({ 0, 1 }, dir);
        // 0 to 1
        float rads_t = (rads + PI) / (PI * 2);
        // 0 to 256 (+0.5 to get nearest angle when cast floors)
        uint8_t rads_quant = (rads_t * 256.0f + 0.5f);
        return rads_quant;
    }

    static Vector2 ReconstructFacing(uint8_t facing)
    {
        // 0 to 1 (from 12 o'clock)
        float rads_t = facing / 256.0f;
        // 0 to 2pi radians
        float rads = rads_t * PI * 2;
        // As vector
        Vector2 dir = Vector2Rotate({ 0, -1 }, rads);
        return dir;
    }

    void SetFacing(Vector2 dir)
    {
        facing = QuantizeFacing(dir);
    }

    Vector2 Facing(void) const
    {
        return ReconstructFacing(facing);
    }

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