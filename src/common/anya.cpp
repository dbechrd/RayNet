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

Anya_Node::Anya_Node(Anya_Node &parent, Vector2 root, Anya_Interval interval) :
    state(parent.state),
    id(state->GetId()),
    parent(parent.id),
    root(root),
    interval(interval),
    cost(Cost())
{}

Anya_Node::Anya_Node(Anya_State &state, Vector2 start) :
    state(&state),
    id(state.GetId()),
    parent(-1),
    root({ -1, -1 }),
    interval({ start.y, start.x, start.x }),
    cost(Cost())
{}

Anya_Node Anya_Node::StartNode(Anya_State &state)
{
    Anya_Node start{ state, state.start };
    return start;
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
    float cost = Vector2DistanceSqr(p, state->target);
    if (parent >= 0) {
        cost += state->nodes[parent].cost;
    }
    return cost;
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

bool Anya_IsTurningPoint(Anya_State &state, Vector2 p, Vector2 r)
{
    assert(floorf(p.y) == p.y);
    if (floorf(p.x) != p.x) {
        return false;
    }

    const bool nw = state.Query(p.x - 1, p.y - 1);
    const bool ne = state.Query(p.x,     p.y - 1);
    const bool sw = state.Query(p.x - 1, p.y);
    const bool se = state.Query(p.x,     p.y);

    const bool corner = (nw + ne + sw + se) == 1;
    if (!corner) {
        return false;
    }

    if (p.y < r.y) {
        // cone up
        if (p.x < r.x) {
            // north west
            //        | NE
            //  ____  |____
            //   SW | p
            //      |  \
            //          r
            return ne || sw;
        } else if (p.x > r.x) {
            // north east
            //   NW |  
            //  ____|  ____
            //      p | SE
            //     /  |
            //    r    
            return nw || se;
        } else {
            // north
            //  ___  p  ___
            //  SW | | | SE
            //     | r |
            //
            return sw || se;
        }
    } else if (p.y > r.y) {
        // cone down
        if (p.x < r.x) {
            // south west
            //         r
            //  NW |  /
            //  ___| p ___
            //        | SE
            //        |
            //
            return nw || se;
        } else if (p.x > r.x) {
            // south east
            //   r
            //    \  | NE
            // ___ p |___
            // SW |
            //    |
            //
            return ne || sw;
        } else {
            // south
            //         
            //  NW | r | NE
            //  ___| | |___
            //       p
            //
            return nw || ne;
        }
    } else {
        // flat node
        if (p.x < r.x) {
            // west
            // 
            //  | NE
            //  |____
            // 
            //  p---r
            //   ____
            //  | SE
            //  |
            //
            return ne || se;
        } else if (p.x > r.x) {
            // east
            // 
            //   NW |
            //  ____|
            // 
            //  r---p
            //  ____
            //   SW |
            //      |
            //
            return nw || sw;
        } else {
            assert(!"point == root is not allowed!");
            return false;
        }
    }

    assert(!"can't get here");
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

void Anya_GenStartSuccessors(Anya_Node &start)
{
    Anya_State &state = *start.state;
    const Vector2 s = start.state->start;
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

        while (!state.Query(x_min - 1, y - 1)) {
            x_min--;
        }
        while (!state.Query(x_max, y - 1)) {
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

            Anya_Node node{ start, s, split_interval };
            state.nodes.push_back(node);
        }
    }
}

void Anya_GenFlatSuccessors(Anya_Node &parent, Vector2 p, Vector2 r)
{
    Anya_State &state = *parent.state;

#if 1
    int dir = 0;
    if (p.x > r.x) {
        dir = 1;
    } else if (p.x < r.x) {
        dir = -1;
    } else {
        if (p.y < r.y) {
            // Searching up
            if (state.Query(p.x - 1, p.y)) {
                // If BL, go left to remain taut
                // _____
                // | | |
                // |x|_|
                dir = -1;
            } else if (state.Query(p.x, p.y)) {
                // If BR, go right to remain taut
                // _____
                // | | |
                // |_|x|
                dir = 1;
            } else {
                assert(!"cannot create flat successors on row above if not a corner!");
            }
        } else if (p.y > r.y) {
            // Searching down
            if (state.Query(p.x - 1, p.y - 1)) {
                // If TL, go left to remain taut
                // _____
                // |x| |
                // |_|_|
                dir = -1;
            } else if (state.Query(p.x, p.y - 1)) {
                // If TR, go right to remain taut
                // _____
                // | |x|
                // |_|_|
                dir = 1;
            } else {
                assert(!"cannot create flat successors on row below if not a corner!");
            }
        } else {
            assert(!"p cannot equal r!");
        }
    }
    assert(dir);
#endif

    float p_prime = p.x;

    int block_offset      = dir == 1 ? 0 : -1;  // if search right, offset x by 0, if search left, offset x by -1, with respect to top-left corner of cell
    int block_offset_diag = dir == 1 ? -1 : 0;  // opposite block offset, for diagonal blocker checks

    // p_prime = first corner point (or farthest vertex) on p.y row such that (r, p, p') is taut
    for (;;) {
        // Either top/bottom left, or top/bottom right, depending on search direction
        bool t = state.Query(p_prime + block_offset, p.y - 1);
        bool b = state.Query(p_prime + block_offset, p.y);
#if 0
        if (t || b) {
            // first corner point in either direction (i.e. shortest possible flat successor)
            break;
        }
#else
        if (t && b) {
            break;
        } else if (t && state.Query(p_prime + block_offset_diag, p.y)) {  // TR & BL
            // dir=1  dir=-1
            // _____  _____
            // | |x|  |x| |
            // |x|_|  |_|x|
            break;
        } else if (b && state.Query(p_prime + block_offset_diag, p.y - 1)) {  // BR & TL
            // dir=1  dir=-1
            // _____  _____   
            // |x| |  | |x|   
            // |_|x|  |x|_|   
            break;
        }
#endif
        p_prime += dir;
    }

    // I = { p (open), p_prime (closed) }
    Anya_Interval I;
    if (dir == 1) {
        // right
        I = { p.y, p.x + ANYA_EPSILON, p_prime };
    } else {
        // left
        I = { p.y, p_prime, p.x - ANYA_EPSILON };
    }

    if (I.HasInterval()) {
        if (r.y == p.y) {
            state.nodes.push_back({ parent, r, I });
        } else {
            state.nodes.push_back({ parent, p, I });
        }
    }
}

void Anya_GenConeSuccessors(Anya_Node &parent, Vector2 a, Vector2 b, Vector2 r)
{
    assert(a.y == b.y);  // if not.. then.. idk man.

    Anya_State &state = *parent.state;

    Anya_Interval I{};
    Vector2 r_prime{};
    Color dbgColor{};

    // Non-observable successors of a flat node
    if (a.y == b.y && b.y == r.y) {
        // r_prime == a or b, whichever is farther from r
        r_prime = fabsf(a.x - r.x) > fabsf(b.x - r.x) ? a : b;

        int dir = r_prime.x > r.x ? 1 : -1;

        // p = point from adjacent row, reached via right-hand turn at a
        // NOTE(dlb): Let's assume 'a' should be 'r_prime'.. turning at 'a' makes no sense...
        Vector2 p{ r_prime.x, r_prime.y + dir };
        
        int x_min = p.x;
        int x_max = p.x;
        
        // I = maximum closed interval beginning at p and observable from r_prime
#if 1
        int blocker_offset = 0;
        assert(dir == 1);
#else
        int blocker_offset = r_prime.x > r.x ? 0 : -1;
        while (!state.Query(x_min - 1, p.y + blocker_offset)) {
            x_min--;
        }
#endif
        while (!state.Query(x_max, p.y + blocker_offset)) {
            x_max++;
        }

        I.y = p.y;
        I.x_min = x_min;
        I.x_max = x_max;
        dbgColor = RED;

    // Non-observable successors of a cone node
    } else if (Vector2Equals(a, b)) {
        r_prime = a;
        
        // HACK(dlb): Hard-coded map width!!
        // p = point from adjacent row, computed via linear projection from r through a
        int dir = a.y - r.y > 0 ? 1 : -1;
        Vector2 p{};
        assert(ClosestPointLineSegmentToRay({ 0, a.y + dir }, { 64, a.y + dir }, r, a, p));

        // I = maximum closed interval beginning at p and observable from r_prime
        int y = p.y;
        int x_min = p.x;
        int x_max = p.x;

        int block_offset = dir == 1 ? -1 : 0;
        while (!state.Query(x_min - 1, y + block_offset)) {
            x_min--;
        }
        while (!state.Query(x_max, y + block_offset)) {
            x_max++;
        }

        I.y = y;
        I.x_min = x_min;
        I.x_max = x_max;
        dbgColor = WHITE;

    // Observable successors of a cone node
    } else {
        r_prime = r;

        // HACK(dlb): Hard-coded map width!!
        // p = point from adjacent row, computed via linear projection from r through a
        int dir_a = a.y - r.y > 0 ? 1 : -1;
        int dir_b = b.y - r.y > 0 ? 1 : -1;
        assert(dir_b == dir_a);

        Vector2 p{};
        assert(ClosestPointLineSegmentToRay({ 0, a.y + dir_a }, { 64, a.y + dir_a }, r, a, p));
        
        // p' = point from adjacent row, computed via linear projection from r through b
        // HACK(dlb): Hard-coded map width!!
        Vector2 p_prime{};
        assert(ClosestPointLineSegmentToRay({ 0, b.y + dir_b }, { 64, b.y + dir_b }, r, b, p_prime));

        int y = p.y;
        int x_min = r_prime.x;
        int x_max = r_prime.x;

        int block_offset = dir_a == 1 ? -1 : 0;
        while (x_min > p.x && !state.Query(x_min - 1, y + block_offset)) {
            x_min--;
        }
        while (x_max < p_prime.x && !state.Query(x_max, y + block_offset)) {
            x_max++;
        }

        x_min = MAX(p.x, x_min);
        x_max = MIN(p_prime.x, x_max);

        // I = maximum closed interval with endpoints p and p' (which is implicitly observable from r)
        I.y = p.y;
        I.x_min = x_min;
        I.x_max = x_max;
        dbgColor = BLUE;
    }

    if (I.HasInterval()) {
        std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(*parent.state, I);
        for (Anya_Interval &II : intervals) {
            assert(II.HasInterval());
            Anya_Node n_prime{ parent, r_prime, II };
            n_prime.dbgColor = dbgColor;
            state.nodes.push_back(n_prime);
        }
    }
}

void Anya_Successors(Anya_Node &node)
{
    if (node.IsStart()) {
        Anya_GenStartSuccessors(node);
    }

    Anya_State &state = *node.state;
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

        //assert(floorf(p.x) == p.x);  // expecting this to be a corner
        Anya_GenFlatSuccessors(node, p, r);

        if (Anya_IsTurningPoint(state, p, r)) {
            Anya_GenConeSuccessors(node, p, p, r);
        }
    } else {
        Vector2 a = { I.x_min, I.y };
        Vector2 b = { I.x_max, I.y };

        Anya_GenConeSuccessors(node, a, b, r);

        if (Anya_IsTurningPoint(state, a, r)) {
            Anya_GenFlatSuccessors(node, a, r);
            Anya_GenConeSuccessors(node, a, a, r);
        }

        if (Anya_IsTurningPoint(state, b, r)) {
            Anya_GenFlatSuccessors(node, b, r);
            Anya_GenConeSuccessors(node, b, b, r);
        }
    }
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
    std::vector<Anya_Node> nodes{};

    struct PrioNode {
        int id{};
        float cost{};
        float y{};

        PrioNode(int id, float cost, float y) : id(id), cost(cost), y(y) {}

        bool operator<(const PrioNode &rhs) const
        {
            // NOTE: Backwards on purpose, less distance = better heuristic and priority_queue is a max heap -_-
            return cost > rhs.cost;
        }
    };
    std::priority_queue<PrioNode> open{};

    Anya_Node start = Anya_Node::StartNode(state);
    //Anya_Node start{ state, { 44, 25, 26 }, { 25, 45 } };

    open.push({ start.id, start.cost, start.interval.y });
    state.nodes.push_back(start);

    const int maxIters = 1000;
    int iters = 0;

#if 0
    struct PathNode {
        float cost{};
        Vector2 root{};
    };

    auto Vec2Hash = [](const Vector2 &v) { return hash_combine(v.x, v.y); };
    auto Vec2Equal = [](const Vector2 &l, const Vector2 &r) { return l.x == r.x && l.y == r.y; };
    std::unordered_map<Vector2, PathNode, decltype(Vec2Hash), decltype(Vec2Equal)> path_cache{};
#endif

    Anya_Node *last_node = 0;

    while (!open.empty() && iters < maxIters) {
        const PrioNode &prioNode = open.top();
        Anya_Node node = state.nodes[prioNode.id];
        assert(node.interval.y == prioNode.y);
        open.pop();
        state.nodeSearchOrder.push_back(node);

        if (node.id == 12) {
            printf("");
        }

        if (node.interval.Contains(state.target)) {
            last_node = &node;
            break;
        }

        int successorStart = state.nodes.size();
        Anya_Successors(node);
        int successorCount = state.nodes.size();
        for (int i = successorStart; i < successorCount; i++) {
            Anya_Node &successor = state.nodes[i];
            if (!Anya_ShouldPrune(successor)) {
                open.push({ successor.id, successor.cost, successor.interval.y });

                if (Vector2Equals(successor.root, node.root)) {
                    successor.parent = node.parent;
                    successor.depth = node.depth;
                } else {
                    successor.parent = node.id;
                    successor.depth = node.depth + 1;
                }
            }
        }

        iters++;
    }

    if (last_node) {
        std::stack<Vector2> grid_path{};
        grid_path.push(state.target);

        Anya_Node *node = last_node;
        while (node->parent >= 0) {
            grid_path.push(node->root);
            node = &state.nodes[node->parent];
        }

        while (!grid_path.empty()) {
            state.path.push_back(Vector2Scale(grid_path.top(), TILE_W));
            grid_path.pop();
        }
    }
}