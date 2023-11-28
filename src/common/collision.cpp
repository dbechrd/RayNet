#include "collision.h"

bool dlb_CheckCollisionPointRec(const Vector2 point, const Rectangle rec)
{
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

bool dlb_CheckCollisionCircleRec(const Vector2 center, const float radius, const Rectangle rec, Manifold *manifold)
{
    float xOverlap = 0;
    float yOverlap = 0;

    Vector2 normal{};

    Vector2 pt = center;  // circle center point constrained to box
    if (pt.x < rec.x) {
        pt.x = rec.x;
        normal.x = -1.0f;
    } else if (pt.x >= rec.x + rec.width) {
        pt.x = rec.x + rec.width;
        normal.x = 1.0f;
    } else {
        float recCenterX = rec.x + rec.width / 2;
        bool leftHalf = pt.x < recCenterX;
        xOverlap = leftHalf ? rec.x - pt.x : (rec.x + rec.width) - pt.x;
        normal.y = leftHalf ? -1.0f : 1.0f;
    }

    if (pt.y < rec.y) {
        pt.y = rec.y;
        normal.y = -1.0f;
    } else if (pt.y >= rec.y + rec.height) {
        pt.y = rec.y + rec.height;
        normal.y = 1.0f;
    } else {
        float recCenterY = rec.y + rec.height / 2;
        bool topHalf = pt.y < recCenterY;
        yOverlap = topHalf ? rec.y - pt.y : (rec.y + rec.height) - pt.y;
        normal.y = topHalf ? -1.0f : 1.0f;
    }

    const float dist = Vector2DistanceSqr(pt, center);
    if (dist < radius * radius) {
        if (manifold) {
            manifold->contact = pt;  // closest point to "pt" on the surface of the box
            if (fabsf(xOverlap) < fabsf(yOverlap)) {
                manifold->contact.x += xOverlap;
            } else {
                // NOTE(dlb): Slight preference to resolving along y when xOverlap == yOverlap
                manifold->contact.y += yOverlap;
            }

            Vector2 pen = Vector2Subtract(center, manifold->contact);
            manifold->depth = radius - Vector2Length(pen);
            manifold->normal = Vector2Normalize(pen);
            if (Vector2LengthSqr(manifold->normal) == 0) {
                manifold->normal = normal;
            }

            if (xOverlap && yOverlap) {
                manifold->depth -= radius * 2.0f;  // ???
            }
            if (manifold->depth < 0.0f) {
                manifold->depth = -manifold->depth;
                manifold->normal = Vector2Negate(manifold->normal);
            }
        }
        return true;
    }
    return false;
}
bool dlb_CheckCollisionCircleRec2(const Vector2 center, const float radius, const Rectangle rec, Manifold *manifold)
{
    const Vector2 cir_a_center = center;
    const float cir_a_radius = radius;

    const Vector2 rec_half{ rec.width / 2, rec.height / 2 };
    const Vector2 cir_b_center = { rec.x + rec_half.x, rec.y + rec_half.y };
    const float cir_b_radius = sqrtf(rec_half.x * rec_half.x + rec_half.y * rec_half.y);

    const Rectangle rec_a = { center.x - radius, center.y - radius, radius * 2, radius * 2 };
    const Rectangle rec_b = rec;

    bool cir_collide = false;
    bool rec_collide = false;

    const Vector2 cir_dist = Vector2Subtract(cir_b_center, cir_a_center);
    const float cir_radii = cir_a_radius + cir_b_radius;
    if (Vector2LengthSqr(cir_dist) < (cir_radii * cir_radii)) {
        // circles colliding
        cir_collide = true;
        /*if (CheckCollisionRecs(rec_a, rec_b)) {
            rec_collide = true;
        }*/
    }
    if (CheckCollisionRecs(rec_a, rec_b)) {
        rec_collide = true;
    }

    bool collide = cir_collide && rec_collide;
    DrawCircleLines(cir_b_center.x, cir_b_center.y, cir_b_radius, collide ? RED : cir_collide ? YELLOW : SKYBLUE);
    DrawRectangleLinesEx(rec_a, 1.0f, collide ? RED : rec_collide ? YELLOW : SKYBLUE);

    if (collide && manifold) {
        manifold->normal = Vector2Normalize(Vector2Subtract(cir_a_center, cir_b_center));
        Vector2 cir_b_contact = Vector2Add(cir_b_center, Vector2Scale(manifold->normal, cir_b_radius));
        DrawCircleV(cir_b_contact, 3, RED);

        Vector2 rec_b_contact{
            CLAMP(cir_b_contact.x, rec_b.x, rec_b.x + rec_b.width),
            CLAMP(cir_b_contact.y, rec_b.y, rec_b.y + rec_b.height),
        };
        manifold->contact = rec_b_contact;
        const float depth = Vector2Length(Vector2Subtract(rec_b_contact, cir_a_center));

        if (dlb_CheckCollisionPointRec(cir_a_center, rec)) {
            manifold->depth = cir_a_radius + depth;
        } else {
            manifold->depth = cir_a_radius - depth;
        }
    }

    return cir_collide && rec_collide;
}
