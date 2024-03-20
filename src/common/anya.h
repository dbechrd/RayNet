#pragma once
#include "common.h"

struct Anya_State;

struct Anya_Path {
    std::vector<Vector2> points{};
};

struct Anya_Interval {
    float y{};
    float x_min{};
    float x_max{};

    Anya_Interval();
    Anya_Interval(float y, float x_min, float x_max);

    bool HasInterval(void);
#if ANYA_PRUNE
    bool IsValid(Vector2 r);
#endif
    bool Contains(Vector2 p);
};

struct Anya_Node {
    Anya_State *state{};
    Anya_Node *parent{};
    Anya_Interval interval{};
    Vector2 root{};
    int successorSet{};

    Anya_Node(Anya_State &state);
    Anya_Node(Anya_State &state, Anya_Interval interval, Vector2 root);

    bool IsStart(void) const;
    bool IsFlat(void) const;
    Vector2 ClosestPointToTarget(void) const;
    float DistanceToTarget(void) const;
    Anya_Path PathTo(Vector2 t) const;

    bool operator<(const Anya_Node &rhs) const;
};

typedef bool (*Anya_SolidQuery)(int x, int y, void *userdata);

struct Anya_State {
    Vector2 start{};
    Vector2 target{};
    Anya_SolidQuery solid_query{};
    void *userdata{};

    Anya_Path path{};
    std::vector<Anya_Node> debugNodes{};

    Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata);

    bool Query(int x, int y);
};

void Anya(Anya_State &state);
