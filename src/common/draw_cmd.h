#pragma once
#include "common.h"

enum DrawCmdType {
    DRAW_CMD_RECT_SOLID,
    DRAW_CMD_RECT_OUTLINE,
    DRAW_CMD_TEXTURE,
};

struct DrawCmd {
    DrawCmdType type      {};
    Vector3     position  {};
    Rectangle   rect      {};
    Color       color     {};
    union {
        Texture2D texture;
        int       thickness;
    };

    static DrawCmd RectSolid(Rectangle rect, Color color)
    {
        DrawCmd cmd{};
        cmd.type = DRAW_CMD_RECT_SOLID;
        cmd.position = { rect.x, rect.y };
        cmd.rect = { rect.x, rect.y, rect.width, rect.height };
        cmd.color = color;
        return cmd;
    }

    static DrawCmd RectOutline(Rectangle rect, Color color, int thickness)
    {
        DrawCmd cmd{};
        cmd.type = DRAW_CMD_RECT_OUTLINE;
        cmd.position = { rect.x, rect.y };
        cmd.rect = rect;
        cmd.color = color;
        cmd.thickness = thickness;
        return cmd;
    }

    static DrawCmd Texture(Texture tex, Rectangle rect, Vector3 pos, Color color)
    {
        DrawCmd cmd{};
        cmd.type = DRAW_CMD_TEXTURE;
        cmd.position = { pos.x, pos.y };
        cmd.rect = rect;
        cmd.color = color;
        cmd.texture = tex;
        return cmd;
    }

    void Draw(void) const;

    bool operator<(const DrawCmd& rhs) const {
        const float me = position.y + position.z + rect.height;
        const float other = rhs.position.y + rhs.position.z + rhs.rect.height;
        return other < me;
    }
};
struct DrawCmdQueue : public std::priority_queue<DrawCmd> {
    void Draw(void) {
        while (!empty()) {
            const DrawCmd &cmd = top();
            cmd.Draw();
            pop();
        }
    }
};