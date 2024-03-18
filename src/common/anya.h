#pragma once
#include "common.h"

struct Anya_Path {
    std::vector<Vector2> points;
};

typedef bool (*Anya_SolidQuery)(int x, int y, void *userdata);

struct Anya_State {
    Vector2 start;
    Vector2 target;
    Anya_SolidQuery solid_query;
    void *userdata;
    
    Anya_Path path;

    Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata)
        : start(start), target(target), solid_query(solid_query), userdata(userdata) {}

    bool Query(int x, int y)
    {
        return solid_query(x, y, userdata);
    }
};

struct Anya_Interval {
    float x_min;
    float x_max;
    float y;

    bool HasInterval(void)
    {
        return x_min < x_max;
    }

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
    Anya_State *state;
    Anya_Node *parent;
    
    Anya_Interval interval;
    Vector2 root;

    Anya_Node() {}
    Anya_Node(Anya_State &state) : state(&state) {}
    Anya_Node(Anya_State &state, Anya_Interval interval, Vector2 root)
        : state(&state), interval(interval), root(root) {}

    static Anya_Node FromStart(Anya_State &state)
    {
        Anya_Node n{ state };
        n.interval.x_min = state.start.x;
        n.interval.x_max = state.start.x;
        n.interval.y = state.start.y;
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

    bool operator<(const Anya_Node &rhs) const
    {
        assert(state == rhs.state);

        // Find the point p in the interval that is closest to the target
        Vector2 p{};
        assert(CheckCollisionLines(root, state->target, { interval.x_min, interval.y }, { interval.x_max, interval.y }, &p));

        // Same thing for other node
        Vector2 p_rhs{};
        assert(CheckCollisionLines(rhs.root, state->target, { rhs.interval.x_min, rhs.interval.y }, { rhs.interval.x_max, rhs.interval.y }, &p_rhs));

        // Compare the distances of each closest point to see which one is better
        const float dist = Vector2DistanceSqr(p, state->target);
        const float rhs_dist = Vector2DistanceSqr(p_rhs, state->target);

        return dist < rhs_dist;
    }
};

void Anya(Anya_State &state);
