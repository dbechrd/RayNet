#include "anya.h"

#define ANYA_EPSILON 0.0001f

std::vector<Anya_Interval> Anya_SplitAtCorners(Anya_Interval &I)
{
    std::vector<Anya_Interval> intervals{};

    // TODO: Split I

    return intervals;
}

std::vector<Anya_Node> Anya_GenStartSuccessors(Anya_State &state, Vector2 s)
{
    std::vector<Anya_Node> successors{};

    Anya_Interval max_intervals[4]{};
    Anya_Interval &I_max_left  = max_intervals[0];
    Anya_Interval &I_max_right = max_intervals[1];
    Anya_Interval &I_max_up    = max_intervals[2];
    Anya_Interval &I_max_down  = max_intervals[3];

    I_max_up   .y = s.y - 1;
    I_max_down .y = s.y + 1;

    // Max interval for all visible points left of s [)
    {
        float x_min = s.x;

        for (;;) {
            bool tl = state.Query(x_min - 1, s.y - 1);
            bool bl = state.Query(x_min - 1, s.y);
            if (tl && bl) {
                // _____
                // |x| |
                // |x|_|
                break;
            } else if (tl && state.Query(x_min, s.y)) {  // TL & BR
                // _____
                // |x| |
                // |_|x|
                break;
            } else if (bl && state.Query(x_min, s.y - 1)) {  // BL & TR
                // _____
                // | |x|
                // |x|_|
                break;
            }
            x_min -= 1;
        }

        I_max_left.y = s.y;
        I_max_left.x_min = x_min;
        I_max_left.x_max = s.x - ANYA_EPSILON;
    };

    // Max interval for all visible points right of s (]
    {
        float x_max = s.x;

        for (;;) {
            bool tr = state.Query(x_max, s.y - 1);
            bool br = state.Query(x_max, s.y);
            if (tr && br) {
                // _____
                // | |x|
                // |_|x|
                break;
            } else if (tr && state.Query(x_max - 1, s.y)) {  // TR & BL
                // _____
                // | |x|
                // |x|_|
                break;
            } else if (br && state.Query(x_max - 1, s.y - 1)) {  // BR & TL
                // _____
                // |x| |
                // |_|x|
                break;
            }
            x_max += 1;
        }

        I_max_left.y = s.y;
        I_max_left.x_min = s.x + ANYA_EPSILON;
        I_max_left.x_max = x_max;
    };

    // TODO: all visible points row above s []
    // None   Left   Right  Both
    // _____  _____  _____  _____
    // |x|x|  | |x|  |x| |  | | |
    // |.|.|  |.|.|  |.|.|  |.|.|

    // TODO: all visible points row below s []
    // None   Left   Right  Both
    // _____  _____  _____  _____
    // |.|.|  |.|.|  |.|.|  |.|.|
    // |x|x|  |_|x|  |x|_|  |_|_|

    std::vector<Anya_Interval> intervals{};
    for (auto &max_interval : max_intervals) {
        auto split_intervals = Anya_SplitAtCorners(max_interval);
        for (auto &split_interval : split_intervals) {
            successors.push_back({ split_interval, s });
        }
    }

    return successors;
}

std::vector<Anya_Node> Anya_GenFlatSuccessors(Anya_State &state, Vector2 p, Vector2 r)
{
    std::vector<Anya_Node> successors{};

    Vector2 p_prime{};
    // p_prime = first corner point (or farthest vertex) on p.y row such that (r, p, p') is taut
    Anya_Interval I{};
    // I = { p (open), p_prime (closed) }

    if (r.y == p.y) {
        successors.push_back({ I, r });
    } else {
        successors.push_back({ I, p });
    }

    return successors;
}

// TODO(cleanup): Make this r, a, b
std::vector<Anya_Node> Anya_GenConeSuccessors(Anya_State &state, Vector2 a, Vector2 b, Vector2 r)
{
    std::vector<Anya_Node> successors{};

    Anya_Interval I{};
    Vector2 r_prime{};

    // Non-observable successors of a flat node
    // TODO(cleanup): Make this r, a, b
    if (a.y == b.y == r.y) {
        // TODO: r_prime == a or b, whichever is farther from r
        Vector2 p{};
        // TODO: p = point from adjacent row, reached via right-hand turn at a
        // I = maximum closed interval beginning at p and observable from r_prime
        I.x_min = p.x;
        I.x_max = -1; // TODO: ???
        I.y = p.y;
    } else if (Vector2Equals(a, b)) {
        r_prime = a;
        Vector2 p{};
        // TODO: p = point from adjacent row, computed via linear projection from r through a
        // I = maximum closed interval beginning at p and observable from r_prime
        I.x_min = p.x;
        I.x_max = -1; // TODO: ???
        I.y = p.y;
    } else {
        r_prime = r;
        Vector2 p{};
        // TODO: p = point from adjacent row, computed via linear projection from r through a
        Vector2 p_prime{};
        // TDOO: p' = point from adjacent row, computed via linear projection from r through b
        // I = maximum closed interval with endpoints a and b (which is implicitly observable from r)
        I.x_min = a.x;
        I.x_max = b.x;
        I.y = a.y;
    }

    std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(I);
    for (Anya_Interval &II : intervals) {
        Anya_Node n_prime{ II, r_prime };
        successors.push_back(n_prime);
    }

    return successors;
}

std::vector<Anya_Node> Anya_Successors(Anya_State &state, Anya_Node &node)
{
    if (node.IsStart()) {
        return Anya_GenStartSuccessors(state, { node.interval.x_min, node.interval.y });
    }

    Anya_Interval I = node.interval;
    Vector2 r = node.root;

    std::vector<Anya_Node> successors{};

    if (node.IsFlat()) {
        Vector2 p{};
        // p = endpoint of I farthest from r

        auto flats = Anya_GenFlatSuccessors(state, p, r);
        successors.reserve(successors.size() + flats.size());
        successors.insert(successors.end(), flats.begin(), flats.end());

        // TODO: how to check turning point??
        bool pTurningPoint = false;
        if (pTurningPoint) {
            auto cones = Anya_GenConeSuccessors(state, p, p, r);
            successors.reserve(successors.size() + cones.size());
            successors.insert(successors.end(), cones.begin(), cones.end());
        }
    } else {
        Vector2 a = { I.x_min, I.y };
        Vector2 b = { I.x_max, I.y };

        auto cones = Anya_GenConeSuccessors(state, a, b, r);
        successors.reserve(successors.size() + cones.size());
        successors.insert(successors.end(), cones.begin(), cones.end());

        // TODO: how to check turning point??
        bool aTurningPoint = false;
        if (aTurningPoint) {
            auto a_flats = Anya_GenFlatSuccessors(state, a, r);
            successors.reserve(successors.size() + a_flats.size());
            successors.insert(successors.end(), a_flats.begin(), a_flats.end());

            auto a_cones = Anya_GenConeSuccessors(state, a, a, r);
            successors.reserve(successors.size() + a_cones.size());
            successors.insert(successors.end(), a_cones.begin(), a_cones.end());
        }

        // TODO: how to check turning point??
        bool bTurningPoint = false;
        if (bTurningPoint) {
            auto b_flats = Anya_GenFlatSuccessors(state, b, r);
            successors.reserve(successors.size() + b_flats.size());
            successors.insert(successors.end(), b_flats.begin(), b_flats.end());

            auto b_cones = Anya_GenConeSuccessors(state, b, b, r);
            successors.reserve(successors.size() + b_cones.size());
            successors.insert(successors.end(), b_cones.begin(), b_cones.end());
        }
    }

    return successors;
}

#if ANYA_PRUNE
bool Anya_IsCulDeSac(Anya_Node &node)
{
    Anya_Interval I{};
    // TODO: I = projection of node (flat or cone, depending on node)
    if (I.IsValid(node.root)) {
        return false;
    }
    return true;
}

bool Anya_IsIntermediate(Anya_Node &node)
{
    if (node.IsFlat()) {
        Vector2 p{};
        // TODO: p = endpoint of node.I frthest from node.root

        bool pTurningPoint = false;
        // TODO: how turning?
        if (pTurningPoint) {
            return false;  // has at least one non-observable successor, cannot be intermediate
        }
    } else {
        // TODO:
        // if node.I has a closed endpoint that is also a corner point {
        //     return false;
        // }
        Anya_Interval I_prime{};
        // TODO: I_prime = interval after projecting node.r through node.I
        // if I_prime contains any corner points {
        //     return false;
        // }
    }
    return true;
}

bool Anya_ShouldPrune(Anya_Node &node)
{
    return Anya_IsCulDeSac(node) || Anya_IsIntermediate(node);
}
#else
bool Anya_ShouldPrune(Anya_Node &node)
{
    return false;
}
#endif

void Anya(Anya_State &state, Vector2 s, Vector2 t)
{
    std::priority_queue<Anya_Node> open{};
    open.push(Anya_Node::FromStart(s));

    while (!open.empty()) {
        Anya_Node node = open.top();
        open.pop();
        if (node.interval.Contains(t)) {
            state.path = node.PathTo(t);
            return;
        }

        auto successors = Anya_Successors(state, node);
        for (auto successor : successors) {
            if (!Anya_ShouldPrune(successor)) {
                open.push(successor);
            }
        }
    }
}