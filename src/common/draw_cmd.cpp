#include "draw_cmd.h"

void DrawCmd::Draw(void) const
{
    switch (type) {
        case DRAW_CMD_RECT_SOLID: {
            DrawRectangleRec(rect, color);
            break;
        }
        case DRAW_CMD_RECT_OUTLINE: {
            DrawRectangleLinesEx(rect, thickness, color);
            break;
        }
        case DRAW_CMD_TEXTURE: {
            const Vector2 pos = {
                position.x,
                position.y - position.z
            };
            dlb_DrawTextureRec(texture, rect, pos, color);
            break;
        }
    }
}