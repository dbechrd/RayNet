#pragma once
#include "common.h"

struct Anya_Interval {
    float x_min;
    float x_max;
    float y;

    bool IsValid(Vector2 r)
    {
        // TODO: Every p in I is visible from r
        return true;
    }

    bool Contains(Vector2 p)
    {
        // TODO: Open vs. closed intervals!?
        return p.y == y && p.x >= x_min && p.x <= x_max;
    }
};

struct Anya_Node {
    Anya_Interval interval;
    Vector2 root;
    Anya_Node *parent;

    static Anya_Node FromStart(Vector2 s)
    {
        Anya_Node n{};
        n.interval.x_min = s.x;
        n.interval.x_max = s.x;
        n.interval.y = s.y;
        n.root = { -1, -1 };
        return n;
    }

    bool IsStart(void)
    {
        return interval.x_min == interval.x_max;
    }

    bool IsFlat(void)
    {
        return interval.y == root.y;
    }

    Anya_Path PathTo(Vector2 t)
    {
        Anya_Path path{};

        // TODO(perf): Second stack is kinda dumb, maybe just reverse the vector at the end?
        std::stack<Vector2> points{};
        points.push(t);

        Anya_Node *node = this;
        while (node) {
            points.push(node->root);
            node = node->parent;
        }

        path.points.reserve(points.size());
        while (!points.empty()) {
            path.points.push_back(points.top());
            points.pop();
        }

        return path;
    }
};

struct Anya_Path {
    std::vector<Vector2> points;
};

typedef bool (*Anya_GridQuery)(int x, int y, void *userdata);

struct Anya_State {
    Anya_GridQuery grid_query;
    void *userdata;
    Anya_Path path;

    bool Query(int x, int y)
    {
        return grid_query(x, y, userdata);
    }
};