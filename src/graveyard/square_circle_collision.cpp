#include "../common/collision.h"

// Collision debug
void SquareCircleCollision(void) {
    static Vector2 cir_pos{};
    static float cir_radius = 20;
    static Rectangle rec{ 100, 100, 64, 64 };

    cir_pos = GetMousePosition();

    DrawCircleLines(cir_pos.x, cir_pos.y, cir_radius, BLUE);
    DrawRectangleLinesEx(rec, 1.0f, BLUE);

    Manifold manifold{};
    if (dlb_CheckCollisionCircleRec(cir_pos, cir_radius, rec, &manifold)) {
        DrawCircleV(manifold.contact, 3, GREEN);

        Vector2 manifold_vec = Vector2Scale(manifold.normal, manifold.depth);
        Vector2 manifold_end{
            manifold.contact.x + manifold_vec.x,
            manifold.contact.y + manifold_vec.y
        };
        DrawLine(
            manifold.contact.x,
            manifold.contact.y,
            manifold_end.x,
            manifold_end.y,
            ORANGE
        );
        DrawCircleV(manifold_end, 3, ORANGE);
    }
}