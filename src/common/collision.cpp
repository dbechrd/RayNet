#include "collision.h"

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec)
{
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

bool dlb_CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec, Vector2 &contact, Vector2 &normal, float &depth)
{
    float xOverlap = 0;
    float yOverlap = 0;

    Vector2 pt = center;  // circle center point constrained to box
    if (pt.x < rec.x) {
        pt.x = rec.x;
    } else if (pt.x >= rec.x + rec.width) {
        pt.x = rec.x + rec.width;
    } else {
        float recCenterX = rec.x + rec.width / 2;
        xOverlap = pt.x < recCenterX ? rec.x - pt.x : (rec.x + rec.width) - pt.x;
    }

    if (pt.y < rec.y) {
        pt.y = rec.y;
    } else if (pt.y >= rec.y + rec.height) {
        pt.y = rec.y + rec.height;
    } else {
        float recCenterY = rec.y + rec.height / 2;
        yOverlap = pt.y < recCenterY ? rec.y - pt.y : (rec.y + rec.height) - pt.y;
    }

    if (Vector2DistanceSqr(pt, center) < radius * radius) {
        contact = pt;  // closest point to "pt" on the surface of the box
        if (fabsf(xOverlap) < fabsf(yOverlap)) {
            contact.x += xOverlap;
        } else if (fabsf(yOverlap) < fabsf(xOverlap)) {
            contact.y += yOverlap;
        }

        Vector2 pen = Vector2Subtract(center, contact);
        depth = radius - Vector2Length(pen);
        normal = Vector2Normalize(pen);

        if (xOverlap && yOverlap) {
            depth -= radius * 2;
        }
        return true;
    }
    return false;
}
