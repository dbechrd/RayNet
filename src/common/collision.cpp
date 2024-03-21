#include "collision.h"

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec)
{
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

bool dlb_CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec, Manifold *manifold)
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
                manifold->normal = normal;  // this is scuffed
            }

            if (xOverlap && yOverlap) {
                manifold->depth -= radius * 2.0f;  // ???
            }
#if 0
            if (manifold->depth < 0.0f) {
                manifold->depth = -manifold->depth;
                manifold->normal = Vector2Negate(manifold->normal);
            }
#else
            if (Vector2DotProduct(pen, manifold->normal) < 0.0f) {
                manifold->depth = -manifold->depth + radius * 2;
            }
#endif
        }
        return true;
    }
    return false;
}
bool dlb_CheckCollisionCircleRec2(Vector2 center, float radius, Rectangle rec, Manifold *manifold)
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

bool dlb_CheckCollisionCircleEdge(Vector2 center, float radius, Edge edge, Manifold *manifold)
{
    const Vector2 a = edge.line.start;
    const Vector2 b = edge.line.end;

    const Vector2 a_to_center = Vector2Subtract(center, a);
    const Vector2 edge_vec = Vector2Subtract(b, a);
    const float edge_len_sq = Vector2LengthSqr(edge_vec);
    const float center_along_edge = Vector2DotProduct(a_to_center, edge_vec);

    float t = center_along_edge / edge_len_sq;
    t = CLAMP(t, 0.0f, 1.0f);

    const Vector2 closest_point = Vector2Add(a, Vector2Scale(edge_vec, t));
    const Vector2 dist = Vector2Subtract(center, closest_point);
    const float dist_sq = Vector2LengthSqr(dist);
    if (dist_sq <= radius * radius) {
        if (manifold) {
            manifold->contact = closest_point;
            const Vector2 pen = Vector2Subtract(center, manifold->contact);
            const float pen_len = Vector2Length(pen);
            if (pen_len == 0.0f) {
                return false;  // what to do when circle center == contact point!?
            }
            manifold->depth = radius - pen_len;
            manifold->normal = Vector2Scale(pen, 1.0f/pen_len); // edge.normal;
            if (Vector2DotProduct(pen, manifold->normal) < 0.0f) {
                manifold->depth = -manifold->depth + radius * 2;
            }


    //#if 0
    //        Vector2 normal{ edge_vec.y, -edge_vec.x };
    //        normal = Vector2Normalize(edge_vec);
    //        manifold->normal = normal;
    //#else
    //        manifold->normal = edge.normal;
    //#endif
    //        manifold->depth = radius - Vector2Length(dist); //Vector2DotProduct(dist, manifold->normal);
    //        //if (manifold->depth < 0) {
    //        //    manifold->depth = -manifold->depth + radius;
    //        //}
        }
        return true;
    }
    return false;
}

bool ClosestPointLineSegmentToRay(Vector2 segmentA, Vector2 segmentB, Vector2 rayStart, Vector2 rayEnd, Vector2 &result)
{
    // https://stackoverflow.com/a/565282
    // t = (q - p) x s / (r x s)
    // u = (q - p) x r / (r x s)

    Vector2 closest_point{};
    bool intersects = false;

    Vector2 p = segmentA;
    Vector2 r = Vector2Subtract(segmentB, segmentA);
    Vector2 q = rayStart;
    Vector2 s = Vector2Subtract(rayEnd, rayStart);

    Vector2 pq = Vector2Subtract(q, p);
    float r_x_s = Vector2CrossProduct(r, s);
    float pq_x_r = Vector2CrossProduct(pq, r);

    if (r_x_s == 0) {
        if (pq_x_r == 0) {
            //assert(!"co-linear");
        } else {
            //assert(!"parallel");
        }
    } else {
        float pq_x_s = Vector2CrossProduct(pq, s);
        float t = pq_x_s / r_x_s;
        float u = pq_x_r / r_x_s;

        if (u >= 0) { // && u <= 1) {
            float t_segment = CLAMP(t, 0, 1);
            closest_point = Vector2Add(p, Vector2Scale(r, t_segment));
            intersects = true;
        }
    }

    result = closest_point;
    return intersects;
}