#pragma once
#include "common.h"

struct Anya_State;

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
    Anya_State *state;
    int id{};
    int parent{};       // gets overwritten along the way to simplify the path
    int orig_parent{};  // id of node that spawned this successor

    Vector2 root{};
    Anya_Interval interval{};
    float rootCost{};
    float totalCost{};

    Color dbgColor{};

    Anya_Node(Anya_Node &parent, Vector2 root, Anya_Interval interval);
    
    static Anya_Node StartNode(Anya_State &state);

    inline bool IsStart(void) const;
    inline bool IsFlat(void) const;
    Vector2 ClosestPointToTarget(void) const;

private:
    Anya_Node(Anya_State &state, Vector2 start);
    void CalcCost(void);
};

typedef bool (*Anya_SolidQuery)(int x, int y, void *userdata);

struct Anya_State {
    Vector2 start{};
    Vector2 target{};
    Anya_SolidQuery solid_query{};
    void *userdata{};
    int next_id{};
    bool target_found{};

    std::vector<Anya_Node> nodes{};
    std::vector<Anya_Node> nodeSearchOrder{};
    std::vector<Vector2> path{};

    inline int GetId(void)
    {
        assert(nodes.size() == next_id);
        next_id++;
        return nodes.size();
    }
    Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata);

    inline bool Query_NW(int x, int y);
    inline bool Query_NE(int x, int y);
    inline bool Query_SW(int x, int y);
    inline bool Query_SE(int x, int y);

    bool AddNodeAndCheckTarget(Anya_Node &node);
};

void Anya(Anya_State &state, float radius = 1.0f);
