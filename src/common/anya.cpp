#include "anya.h"

#define ANYA_EPSILON 0.0001f

Anya_Interval::Anya_Interval()
{}

Anya_Interval::Anya_Interval(float y, float x_min, float x_max) : y(y), x_min(x_min), x_max(x_max)
{}

bool Anya_Interval::HasInterval(void) const
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

bool Anya_Interval::Contains(Vector2 p) const
{
    // TODO: Open vs. closed intervals!?
    return p.y == y && p.x >= x_min && p.x <= x_max;
}

Anya_Node::Anya_Node(Anya_State &state) : state(&state)
{
}

Anya_Node::Anya_Node(Anya_State &state, Anya_Interval interval, Vector2 root)
    : state{ &state }, interval{ interval }, root{ root }
{
}

bool Anya_Node::IsStart(void) const
{
    return interval.x_min == interval.x_max;
}

bool Anya_Node::IsFlat(void) const
{
    return interval.y == root.y;
}

Vector2 Anya_Node::ClosestPointToTarget(void) const
{
    if (interval.x_min == interval.x_max) {
        return { interval.x_min, interval.y };
    }
    if (state->target.y == interval.y && state->target.x >= interval.x_min && state->target.x <= interval.x_max) {
        return state->target;
    }
    if (root.y == interval.y && interval.y == state->target.y) {
        // co-linear
        Vector2 closest{
            CLAMP(state->target.y, interval.x_min, interval.x_max),
            state->target.y
        };
        return closest;
    }

    Vector2 closest_point{};

    Vector2 p0{ interval.x_min, interval.y };
    Vector2 p1{ interval.x_max, interval.y };

    if (!ClosestPointLineSegmentToRay(p0, p1, root, state->target, closest_point)) {
        Vector2 target_reflected{};
        target_reflected.x = state->target.x;
        target_reflected.y = p0.y + (p0.y - state->target.y);

        assert(ClosestPointLineSegmentToRay(p0, p1, root, target_reflected, closest_point));
    }
    return closest_point;
}

float Anya_Node::DistanceToTarget(void) const
{
    Vector2 p = ClosestPointToTarget();
    return Vector2Distance(p, state->target);
}

float Anya_Node::Cost(void) const
{
    Vector2 p = ClosestPointToTarget();
    return Vector2DistanceSqr(p, state->target);
}

// TODO(dlb): Cache the calculated heuristic value to speed up future comparisons (and enable cheap heuristic debug viz)
bool Anya_Node::operator<(const Anya_Node &rhs) const
{
    assert(state == rhs.state);

    //const Vector2 closest_point = ClosestPointToTarget();
    //const Vector2 rhs_closest_point = rhs.ClosestPointToTarget();
    //
    //// Compare the distances of each closest point to see which one is better
    //const float dist = Vector2DistanceSqr(closest_point, state->target);
    //const float rhs_dist = Vector2DistanceSqr(rhs_closest_point, state->target);

    // NOTE: Backwards on purpose, less distance = better heuristic and priority_queue is a max heap -_-
    return Cost() > rhs.Cost();
}

Anya_State::Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata)
    : start(start), target(target), solid_query(solid_query), userdata(userdata)
{}

bool Anya_State::Query(int x, int y)
{
    return solid_query(x, y, userdata);
}

bool Anya_IsCorner(Anya_State &state, float x, float y)
{
    assert(floorf(y) == y);
    if (floorf(x) != x) {
        return false;
    }

    const bool tl = state.Query(x - 1, y - 1);
    const bool bl = state.Query(x - 1, y);
    const bool tr = state.Query(x, y - 1);
    const bool br = state.Query(x, y);

    const int solid = tl + bl + tr + br;
    return solid == 1;
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

            if (segment.HasInterval()) {
                intervals.push_back(segment);
            }
            break;
        }

        if (Anya_IsCorner(state, x, I.y)) {
            segment.x_max = x;

            if (segment.HasInterval()) {
                intervals.push_back(segment);
            }
            segment.x_min = segment.x_max + ANYA_EPSILON;
            segment.x_max = -1;
        }
    }

    if (intervals.empty()) {
        assert(I.HasInterval());
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
            assert(split_interval.HasInterval());

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
    
    // p_prime = first corner point (or farthest vertex) on p.y row such that (r, p, p') is taut
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

std::vector<Anya_Node> Anya_GenConeSuccessors(Anya_State &state, Vector2 a, Vector2 b, Vector2 r)
{
    // a and b must be corners (preferably turning points..)
    //assert(floorf(a.x) == a.x);
    //assert(floorf(a.y) == a.y);
    //assert(floorf(b.x) == b.x);
    //assert(floorf(b.y) == b.y);

    std::vector<Anya_Node> successors{};

    Anya_Interval I{};
    Vector2 r_prime{};

    // Non-observable successors of a flat node
    if (a.y == b.y && b.y == r.y) {
        // r_prime == a or b, whichever is farther from r
        r_prime = Vector2DistanceSqr(a, r) > Vector2DistanceSqr(b, r) ? a : b;

        // p = point from adjacent row, reached via right-hand turn at a
        Vector2 p{ a.x, a.y + 1 };
        
        int x_max = p.x;

        // I = maximum closed interval beginning at p and observable from r_prime
        {
            for (;;) {
                bool tr = state.Query(x_max, p.y - 1);
                bool br = state.Query(x_max, p.y);
                if (tr && br) {
                    // _____
                    // | |x|
                    // |_|x|
                    break;
                } else if (tr && state.Query(x_max - 1, p.y)) {  // TR & BL
                    // _____
                    // | |x|
                    // |x|_|
                    break;
                } else if (br && state.Query(x_max - 1, p.y - 1)) {  // BR & TL
                    // _____
                    // |x| |
                    // |_|x|
                    break;
                }
                x_max += 1;
            }
        }

        I.y = p.y;
        I.x_min = p.x;
        I.x_max = x_max;

    // Non-observable successors of a cone node
    } else if (Vector2Equals(a, b)) {
        r_prime = a;
        
        // HACK(dlb): Hard-coded map width!!
        // p = point from adjacent row, computed via linear projection from r through a
        float dy_a = a.y - r.y > 0 ? 1.0f : -1.0f;
        Vector2 p{};
        assert(ClosestPointLineSegmentToRay({ 0, a.y + dy_a }, { 64, a.y + dy_a }, r, a, p));

        int x_max = p.x;

        // I = maximum closed interval beginning at p and observable from r_prime
        {
            for (;;) {
                bool tr = state.Query(x_max, p.y - 1);
                bool br = state.Query(x_max, p.y);
                if (tr && br) {
                    // _____
                    // | |x|
                    // |_|x|
                    break;
                } else if (tr && state.Query(x_max - 1, p.y)) {  // TR & BL
                    // _____
                    // | |x|
                    // |x|_|
                    break;
                } else if (br && state.Query(x_max - 1, p.y - 1)) {  // BR & TL
                    // _____
                    // |x| |
                    // |_|x|
                    break;
                }
                x_max += 1;
            }
        }

        I.y = p.y;
        I.x_min = p.x;
        I.x_max = x_max;

    // Observable successors of a cone node
    } else {
        r_prime = r;
        
        // HACK(dlb): Hard-coded map width!!
        // p = point from adjacent row, computed via linear projection from r through a
        float dy_a = a.y - r.y > 0 ? 1.0f : -1.0f;
        Vector2 p{};
        assert(ClosestPointLineSegmentToRay({ 0, a.y + dy_a }, { 64, a.y + dy_a }, r, a, p));
        
        // p' = point from adjacent row, computed via linear projection from r through b
        // HACK(dlb): Hard-coded map width!!
        float dy_b = b.y - r.y > 0 ? 1.0f : -1.0f;
        Vector2 p_prime{};
        assert(ClosestPointLineSegmentToRay({ 0, b.y + dy_b }, { 64, b.y + dy_b }, r, b, p_prime));

        // I = maximum closed interval with endpoints p and p' (which is implicitly observable from r)
        I.y = p.y;
        I.x_min = p.x;
        I.x_max = p_prime.x;
    }

    if (I.HasInterval()) {
        std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(state, I);
        for (Anya_Interval &II : intervals) {
            assert(II.HasInterval());
            Anya_Node n_prime{ state, II, r_prime };
            successors.push_back(n_prime);
        }
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

        if (Anya_IsCorner(state, p.x, p.y)) {
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

        if (Anya_IsCorner(state, a.x, a.y)) {
            auto a_flats = Anya_GenFlatSuccessors(state, a, r);
            successors.reserve(successors.size() + a_flats.size());
            successors.insert(successors.end(), a_flats.begin(), a_flats.end());

            auto a_cones = Anya_GenConeSuccessors(state, a, a, r);
            successors.reserve(successors.size() + a_cones.size());
            successors.insert(successors.end(), a_cones.begin(), a_cones.end());
        }

        if (Anya_IsCorner(state, b.x, b.y)) {
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
    return !node.interval.HasInterval() || Anya_IsCulDeSac(node) || Anya_IsIntermediate(node);
}
#else
bool Anya_ShouldPrune(Anya_Node &node)
{
    return !node.interval.HasInterval();
}
#endif

void Anya(Anya_State &state)
{
    std::priority_queue<Anya_Node> open{};

    Anya_Node start{ state, { state.start.y, state.start.x, state.start.x }, { -1, -1 } };
    //Anya_Node start{ state, { 44, 25, 26 }, { 25, 45 } };

    open.push(start);
    state.debugNodesGenerated.push_back(start);

    const int maxIters = 1000;
    int iters = 0;

    struct PathNode {
        float cost{};
        Vector2 root{};
    };

    auto Vec2Hash = [](const Vector2 &v) { return hash_combine(v.x, v.y); };
    auto Vec2Equal = [](const Vector2 &l, const Vector2 &r) { return l.x == r.x && l.y == r.y; };
    std::unordered_map<Vector2, PathNode, decltype(Vec2Hash), decltype(Vec2Equal)> path_cache{};

    while (!open.empty() && iters < maxIters) {
        Anya_Node node = open.top();
        state.debugNodesSearched.push_back(node);
        open.pop();

        if (node.interval.Contains(state.target)) {
            path_cache[state.target] = { 0, node.root };
            break;
        }

        //const auto &path_node_iter = path_cache.find(node.root);
        //if (path_node_iter == path_cache.end()) {
        //    path_cache[node.root] = { node.Cost(), node.root };
        //} else if (node.Cost() < path_node_iter->second.cost) {
        //    path_cache[node.root] = { node.Cost(), node.root };
        //}

        auto successors = Anya_Successors(state, node);
        for (auto successor : successors) {
            if (!Anya_ShouldPrune(successor)) {
                successor.successorSet = node.successorSet + 1;
                open.push(successor);
                state.debugNodesGenerated.push_back(successor);

                if (!Vector2Equals(successor.root, node.root)) {
                    const auto &path_node_iter = path_cache.find(successor.root);
                    const float cost = successor.Cost();
                    if (path_node_iter == path_cache.end()) {
                        path_cache[successor.root] = { cost, node.root };
                    } else if (cost < path_node_iter->second.cost) {
                        path_cache[successor.root] = { cost, node.root };
                    }
                }
            }
        }

        iters++;
    }

#if 1
    std::vector<Vector2> grid_path{};
    
    Vector2 root = state.target;
    do {
        grid_path.push_back(root);

        const auto &iter = path_cache.find(root);
        if (iter == path_cache.end()) {
            //assert(!"wtf!");
            break;
        }
        root = iter->second.root;
    } while (!Vector2Equals(root, state.start));

    grid_path.push_back(state.start);

    for (int i = grid_path.size() - 1; i >= 0; i--) {
        state.path.points.push_back(Vector2Scale(grid_path[i], TILE_W));
    }
#endif
}