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

    bool HasInterval(void) const;
#if ANYA_PRUNE
    bool IsValid(Vector2 r);
#endif
    bool Contains(Vector2 p) const;
};

struct Anya_Node {
    Anya_State *state{};
    Anya_Node *parent{};
    Anya_Interval interval{};
    Vector2 root{};
    int successorSet{};
    int id{};
    Color dbgColor{};

    Anya_Node(Anya_State &state);
    Anya_Node(Anya_State &state, Anya_Interval interval, Vector2 root);

    bool IsStart(void) const;
    bool IsFlat(void) const;
    Vector2 ClosestPointToTarget(void) const;
    float DistanceToTarget(void) const;
    float Cost(void) const;

    bool operator<(const Anya_Node &rhs) const;
};

typedef bool (*Anya_SolidQuery)(int x, int y, void *userdata);

struct Anya_State {
    Vector2 start{};
    Vector2 target{};
    Anya_SolidQuery solid_query{};
    void *userdata{};
    int next_node_id = 0;

    Anya_Path path{};
    std::vector<Anya_Node> debugNodesGenerated{};
    std::vector<Anya_Node> debugNodesSearched{};

    inline int GetId(void)
    {
        return next_node_id++;
    }
    Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata);

    bool Query(int x, int y);
};

void Anya(Anya_State &state);
