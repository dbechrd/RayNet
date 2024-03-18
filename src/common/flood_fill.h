#pragma once
#include "common.h"

typedef bool (*FloodFill_TryGet)(int x, int y, void *userdata, int *value);
typedef void (*FloodFill_Set)(int x, int y, int value, void *userdata);

struct FloodFill {
public:
    FloodFill(FloodFill_TryGet get, FloodFill_Set set, void *userdata)
        : get(get), set(set), userdata(userdata) {}

    void Fill(int x, int y, int replacement)
    {
        int find;
        if (!get(x, y, userdata, &find) || find == replacement) {
            return;
        }

        int lx;
        int rx;
        int tmp;

        stack.push({ x, y });
        while (!stack.empty()) {
            Coord coord = stack.top();
            stack.pop();

            lx = coord.x;
            rx = coord.x;

            while (lx && get(lx - 1, coord.y, userdata, &tmp) && tmp == find) {
                set(lx - 1, coord.y, replacement, userdata);
                lx -= 1;
            }
            while (get(rx, coord.y, userdata, &tmp) && tmp == find) {
                set(rx, coord.y, replacement, userdata);
                rx += 1;
            }
            if (coord.y) {
                Scan(lx, rx, coord.y - 1, find);
            }
            Scan(lx, rx, coord.y + 1, find);
        }
    }

private:
    struct Coord {
        int x, y;
    };

    std::stack<Coord> stack{};
    FloodFill_TryGet get{};
    FloodFill_Set set{};
    void *userdata;

    void Scan(int lx, int rx, int y, int find)
    {
        bool inSpan = false;
        int value;
        for (int x = lx; x < rx; x++) {
            if (!get(x, y, userdata, &value) || value != find) {
                inSpan = false;
            } else if (!inSpan) {
                stack.push({ x, y });
                inSpan = true;
            }
        }
    }
};