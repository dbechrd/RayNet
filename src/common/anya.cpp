#include "anya.h"

#define ANYA_EPSILON 0.0001f

Anya_Interval::Anya_Interval()
{}

Anya_Interval::Anya_Interval(float y, float x_min, float x_max) : y(y), x_min(x_min), x_max(x_max)
{}

bool Anya_Interval::HasInterval(void)
{
    return x_min < x_max;
}

#if ANYA_PRUNE
bool Anya_Interval::IsValid(Vector2 r)
{
    // TODO: Every p in I is visible from r
    return true;
}
#endif

bool Anya_Interval::Contains(Vector2 p)
{
    // TODO: Open vs. closed intervals!?
    return p.y == y && p.x >= x_min && p.x <= x_max;
}

Anya_Node::Anya_Node(Anya_State &state) : state(&state)
{}

Anya_Node::Anya_Node(Anya_State &state, Anya_Interval interval, Vector2 root)
    : state{ &state }, interval{ interval }, root{ root }
{}

bool Anya_Node::IsStart(void)
{
    return interval.x_min == interval.x_max;
}

bool Anya_Node::IsFlat(void)
{
    return interval.y == root.y;
}

Anya_Path Anya_Node::PathTo(Vector2 t)
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

bool Anya_Node::operator<(const Anya_Node &rhs) const
{
    assert(state == rhs.state);

    Vector2 closest_point = ClosestPointLineSegmentToLine(
        { interval.x_min, interval.y },
        { interval.x_max, interval.y },
        root,
        state->target
    );

    Vector2 rhs_closest_point = ClosestPointLineSegmentToLine(
        { rhs.interval.x_min, rhs.interval.y },
        { rhs.interval.x_max, rhs.interval.y },
        rhs.root,
        state->target
    );

    // Compare the distances of each closest point to see which one is better
    const float dist = Vector2DistanceSqr(closest_point, state->target);
    const float rhs_dist = Vector2DistanceSqr(rhs_closest_point, state->target);

    return dist < rhs_dist;
}

Anya_State::Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata)
    : start(start), target(target), solid_query(solid_query), userdata(userdata)
{}

bool Anya_State::Query(int x, int y)
{
    return solid_query(x, y, userdata);
}

std::vector<Anya_Interval> Anya_SplitAtCorners(Anya_State &state, Anya_Interval &I)
{
    std::vector<Anya_Interval> intervals{};

    int x0 = floorf(I.x_min + 1);
    int x1 = floorf(I.x_max);

    Anya_Interval segment{ I.y, I.x_min, -1 };

    for (int x = x0; x <= x1; x++) {
        if (x == x1) {
            segment.x_max = I.x_max;

            intervals.push_back(segment);
            break;
        }

        const bool tl = state.Query(x - 1, I.y - 1);
        const bool bl = state.Query(x - 1, I.y);
        const bool tr = state.Query(x, I.y - 1);
        const bool br = state.Query(x, I.y);

        const int solid = tl + bl + tr + br;
        const bool corner = solid == 1;
        if (corner) {
            segment.x_max = x;

            if (!segment.HasInterval()) {
                assert(!"invalid segment");
            }
            intervals.push_back(segment);
            segment.x_min = segment.x_max + ANYA_EPSILON;
            segment.x_max = -1;
        }
    }

    if (intervals.empty()) {
        intervals.push_back(I);
    }

    return intervals;
}

std::vector<Anya_Node> Anya_GenStartSuccessors(Anya_State &state)
{
    const Vector2 s = state.start;
    std::vector<Anya_Node> successors{};
    std::vector<Anya_Interval> max_intervals{};

    // Max interval for all visible points in row above s []
    {
        // None   Left   Right  Both
        // _____  _____  _____  _____
        // |x|x|  | |x|  |x| |  | | |
        // |.|.|  |.|.|  |.|.|  |.|.|

        const float y = s.y - 1;
        float x_min = s.x;
        float x_max = s.x;

        while (!state.Query(x_min - 1, y)) {
            x_min--;
        }
        while (!state.Query(x_max, y)) {
            x_max++;
        }

        Anya_Interval I_max_up{ y, x_min, x_max };
        if (I_max_up.HasInterval()) {
            max_intervals.push_back(I_max_up);
        }
    }

    // Max interval for all visible points left of s [)
    {
        const float y = s.y;
        float x_min = s.x;

        for (;;) {
            bool tl = state.Query(x_min - 1, y - 1);
            bool bl = state.Query(x_min - 1, y);
            if (tl && bl) {
                // _____
                // |x| |
                // |x|_|
                break;
            } else if (tl && state.Query(x_min, y)) {  // TL & BR
                // _____
                // |x| |
                // |_|x|
                break;
            } else if (bl && state.Query(x_min, y - 1)) {  // BL & TR
                // _____
                // | |x|
                // |x|_|
                break;
            }
            x_min -= 1;
        }

        Anya_Interval I_max_left{ y, x_min, s.x - ANYA_EPSILON };
        if (I_max_left.HasInterval()) {
            max_intervals.push_back(I_max_left);
        }
    }

    // Max interval for all visible points right of s (]
    {
        const float y = s.y;
        float x_max = s.x;

        for (;;) {
            bool tr = state.Query(x_max, y - 1);
            bool br = state.Query(x_max, y);
            if (tr && br) {
                // _____
                // | |x|
                // |_|x|
                break;
            } else if (tr && state.Query(x_max - 1, y)) {  // TR & BL
                // _____
                // | |x|
                // |x|_|
                break;
            } else if (br && state.Query(x_max - 1, y - 1)) {  // BR & TL
                // _____
                // |x| |
                // |_|x|
                break;
            }
            x_max += 1;
        }

        Anya_Interval I_max_right{ y, s.x + ANYA_EPSILON, x_max };
        if (I_max_right.HasInterval()) {
            max_intervals.push_back(I_max_right);
        }
    }

    // Max interval for all visible points in row below s []
    {
        // None   Left   Right  Both
        // _____  _____  _____  _____
        // |.|.|  |.|.|  |.|.|  |.|.|
        // |x|x|  |_|x|  |x|_|  |_|_|

        const float y = s.y + 1;
        float x_min = s.x;
        float x_max = s.x;

        while (!state.Query(x_min - 1, y)) {
            x_min--;
        }
        while (!state.Query(x_max, y)) {
            x_max++;
        }

        Anya_Interval I_max_down{ y, x_min, x_max };
        if (I_max_down.HasInterval()) {
            max_intervals.push_back(I_max_down);
        }
    }

    for (auto &max_interval : max_intervals) {
        auto split_intervals = Anya_SplitAtCorners(state, max_interval);
        for (auto &split_interval : split_intervals) {
            Anya_Node node{ state };
            node.interval = split_interval;
            node.root = s;
            successors.push_back(node);
        }
    }

    return successors;
}

std::vector<Anya_Node> Anya_GenFlatSuccessors(Anya_State &state, Vector2 p, Vector2 r)
{
    std::vector<Anya_Node> successors{};

    float p_prime = p.x;
    
    // TODO: p_prime = first corner point (or farthest vertex) on p.y row such that (r, p, p') is taut
    // Next segment to right of p (]
    {
        for (;;) {
            bool tr = state.Query(p_prime, p.y - 1);
            bool br = state.Query(p_prime, p.y);
            if (tr && br) {
                // _____
                // | |x|
                // |_|x|
                break;
            } else if (tr && state.Query(p_prime - 1, p.y)) {  // TR & BL
                // _____
                // | |x|
                // |x|_|
                break;
            } else if (br && state.Query(p_prime - 1, p.y - 1)) {  // BR & TL
                // _____
                // |x| |
                // |_|x|
                break;
            }
            p_prime += 1;
        }
    }

    // I = { p (open), p_prime (closed) }
    Anya_Interval I{ p.y, p.x + ANYA_EPSILON, p_prime };

    if (I.HasInterval()) {
        if (r.y == p.y) {
            successors.push_back({ state, I, r });
        } else {
            successors.push_back({ state, I, p });
        }
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
    if (a.y == b.y && b.y == r.y) {
        // TODO: r_prime == a or b, whichever is farther from r
        Vector2 p{};
        // TODO: p = point from adjacent row, reached via right-hand turn at a
        // I = maximum closed interval beginning at p and observable from r_prime
        I.y = p.y;
        I.x_min = p.x;
        I.x_max = -1; // TODO: ???
    } else if (Vector2Equals(a, b)) {
        r_prime = a;
        Vector2 p{};
        // TODO: p = point from adjacent row, computed via linear projection from r through a
        // I = maximum closed interval beginning at p and observable from r_prime
        I.y = p.y;
        I.x_min = p.x;
        I.x_max = -1; // TODO: ???
    } else {
        r_prime = r;
        Vector2 p{};
        // TODO: p = point from adjacent row, computed via linear projection from r through a
        Vector2 p_prime{};
        // TDOO: p' = point from adjacent row, computed via linear projection from r through b
        // I = maximum closed interval with endpoints a and b (which is implicitly observable from r)
        I.y = a.y;
        I.x_min = a.x;
        I.x_max = b.x;
    }

    std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(state, I);
    for (Anya_Interval &II : intervals) {
        Anya_Node n_prime{ state, II, r_prime };
        successors.push_back(n_prime);
    }

    return successors;
}

std::vector<Anya_Node> Anya_Successors(Anya_State &state, Anya_Node &node)
{
    if (node.IsStart()) {
        return Anya_GenStartSuccessors(state);
    }

    Anya_Interval I = node.interval;
    Vector2 r = node.root;

    std::vector<Anya_Node> successors{};

    if (node.IsFlat()) {
        const Vector2 p0{ I.x_min, I.y };
        const Vector2 p1{ I.x_max, I.y };
        const float p0_dist = Vector2DistanceSqr(p0, r);
        const float p1_dist = Vector2DistanceSqr(p1, r);

        // p = endpoint of I farthest from r
        Vector2 p = p0_dist > p1_dist ? p0 : p1;

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

void Anya(Anya_State &state)
{
    std::priority_queue<Anya_Node> open{};

    Anya_Node start{ state, { state.start.y, state.start.x, state.start.x }, { -1, -1 } };
    open.push(start);

    const int maxIters = 1000;
    int iters = 0;

    while (!open.empty() && iters < maxIters) {
        Anya_Node node = open.top();
        state.debugNodes.push_back(node);
        open.pop();
        if (node.interval.Contains(state.target)) {
            state.path = node.PathTo(state.target);
            return;
        }

        auto successors = Anya_Successors(state, node);
        for (auto successor : successors) {
            if (!Anya_ShouldPrune(successor)) {
                open.push(successor);
            }
        }

        iters++;
    }
}