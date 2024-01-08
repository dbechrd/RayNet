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
        case DRAW_CMD_TOOLTIP: {
            // TODO: ui.Tooltip(string) that pushes into drawcmdqueue
            // Draw tooltip
            const Vector2 tipSize = dlb_MeasureTextShadowEx(fntSmall, text.data(), text.size(), 0);
            Rectangle tipRect{ position.x, position.y, tipSize.x, tipSize.y };
            Vector2 grow{ 6, 2 };
            tipRect = RectGrow(tipRect, grow);

            DrawRectangleRec(tipRect, Fade(ColorBrightness(DARKGRAY, -0.5f), 0.8f));
            DrawRectangleLinesEx(tipRect, 1, BLACK);

            dlb_DrawTextShadowEx(fntSmall, text.data(), text.size(), { tipRect.x + grow.x, tipRect.y + grow.y }, WHITE);
            break;
        }
    }
}