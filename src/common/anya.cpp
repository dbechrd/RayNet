#include "anya.h"

#define ANYA_EPSILON 0.0f //0.0001f

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
    interval(interval)
{
    CalcCost();
}

Anya_Node::Anya_Node(Anya_State &state, Vector2 start) :
    state(&state),
    id(state.GetId()),
    parent(-1),
    root({ start.x, start.y }),
    interval({ start.y, start.x, start.x })
{
    CalcCost();
}

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

void Anya_Node::CalcCost(void)
{
    float cost = 0;
    if (parent >= 0) {
        // cost of start -> parent root
        const auto &parent_node = state->nodes[parent];
        cost += parent_node.rootCost;
        // cost of parent root -> root
        cost += Vector2Distance(parent_node.root, root);
    }
    rootCost = cost;

    Vector2 p = ClosestPointToTarget();
    // cost of root -> p
    cost += Vector2Distance(root, p);
    // approx. cost of p -> target
    cost += Vector2Distance(p, state->target);
    totalCost = cost;
}

Anya_State::Anya_State(Vector2 start, Vector2 target, Anya_SolidQuery solid_query, void *userdata)
    : start(start), target(target), solid_query(solid_query), userdata(userdata)
{}

bool Anya_State::Query(int x, int y)
{
    return solid_query(x, y, userdata);
}

#define CORNER_NW 0b0001
#define CORNER_NE 0b0010
#define CORNER_SW 0b0100
#define CORNER_SE 0b1000

bool Anya_IsCorner(Anya_State &state, float x, float y, int *flags = 0)
{
    assert(floorf(y) == y);
    if (floorf(x) != x) {
        return false;
    }

    const bool nw = state.Query(x - 1, y - 1);
    const bool ne = state.Query(x    , y - 1);
    const bool sw = state.Query(x - 1, y);
    const bool se = state.Query(x    , y);

    if (flags) {
        if (nw) *flags |= CORNER_NW;
        if (ne) *flags |= CORNER_NE;
        if (sw) *flags |= CORNER_SW;
        if (se) *flags |= CORNER_SE;
    }

    return nw + ne + sw + se == 1;
}

bool Anya_IsTurningPoint(Anya_State &state, Vector2 p, Vector2 r, int *flags = 0, bool *is_corner = 0)
{
    int corner_flags = 0;
    
    bool isCorner = Anya_IsCorner(state, p.x, p.y, &corner_flags);
    
    if (flags) *flags = corner_flags;
    if (is_corner) *is_corner = isCorner;
    
    if (!isCorner) {
        return false;
    }

    bool nw = corner_flags & CORNER_NW;
    bool ne = corner_flags & CORNER_NE;
    bool sw = corner_flags & CORNER_SW;
    bool se = corner_flags & CORNER_SE;

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

std::vector<Anya_Interval> Anya_SplitAtCorners(Anya_State &state, Vector2 r, Anya_Interval &I)
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

        if (Anya_IsCorner(state, (float)x, I.y)) {
        //if (Anya_IsTurningPoint(state, { (float)x, I.y }, r)) {
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
        auto split_intervals = Anya_SplitAtCorners(state, s, max_interval);
        for (auto &split_interval : split_intervals) {
            assert(split_interval.HasInterval());

            Anya_Node node{ start, s, split_interval };
            state.nodes.push_back(node);
        }
    }
}

void Anya_GenFlatSuccessors(Anya_Node &parent, Vector2 p, Vector2 r, int flags)
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
            if (flags & CORNER_SW) {
                // If BL, go left to remain taut
                // _____
                // | | |
                // |x|_|
                dir = -1;
            } else if (flags & CORNER_SE) {
                // If BR, go right to remain taut
                // _____
                // | | |
                // |_|x|
                dir = 1;
            } else {
                return;
                assert(!"cannot create flat successors on row above if not a corner!");
            }
        } else if (p.y > r.y) {
            // Searching down
            if (flags & CORNER_NW) {
                // If TL, go left to remain taut
                // _____
                // |x| |
                // |_|_|
                dir = -1;
            } else if (flags & CORNER_NE) {
                // If TR, go right to remain taut
                // _____
                // | |x|
                // |_|_|
                dir = 1;
            } else {
                return;
                assert(!"cannot create flat successors on row below if not a corner!");
            }
        } else {
            assert(!"p cannot equal r!");
        }
    }
    assert(dir);
#endif

#if 0
    float p_prime = p.x;
    if (Anya_IsCorner(state, p.x, p.y)) {
        p_prime += dir;
    }

    // p_prime = first corner_flags point (or farthest vertex) on p.y row such that (r, p, p') is taut
    // HACK(dlb): Hard-coded map width!
    while (p_prime < 64 && !Anya_IsCorner(state, p_prime, p.y)) {
        p_prime += dir;
    }
#else
    float p_prime = p.x;
    
    int block_offset      = dir == 1 ? 0 : -1;  // if search right, offset x by 0, if search left, offset x by -1, with respect to top-left corner_flags of cell
    int block_offset_diag = dir == 1 ? -1 : 0;  // opposite block offset, for diagonal blocker checks

    bool initial_t = state.Query(p_prime + block_offset, p.y - 1);
    bool initial_b = state.Query(p_prime + block_offset, p.y);
    
    if (initial_t && initial_b) {
        // Wall
        return;
    } else if (initial_t && state.Query(p_prime + block_offset_diag, p.y)) {
        // Passing through diagonal
        return;
    } else if (initial_b && state.Query(p_prime + block_offset_diag, p.y - 1)) {
        // Passing through diagonal
        return;
    }

    p_prime += dir;

    // p_prime = first corner_flags point (or farthest vertex) on p.y row such that (r, p, p') is taut
    // HACK(dlb): Hard-coded map width
    while (p_prime >= 0 && p_prime <= 64) {
        // Either top/bottom left, or top/bottom right, depending on search direction
        bool t = state.Query(p_prime + block_offset, p.y - 1);
        bool b = state.Query(p_prime + block_offset, p.y);
#if 1
        // first corner_flags point in either direction (i.e. shortest possible flat successor)
        if (t != initial_t || b != initial_b) {
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
#endif

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

void Anya_GenConeSuccessors(Anya_Node &parent, Vector2 a, Vector2 b, Vector2 r, int flags)
{
    assert(a.y == b.y);  // if not.. then.. idk man.

    Anya_State &state = *parent.state;

    Anya_Interval I{};
    Vector2 r_prime{};
    Color dbgColor{};

    // Non-observable successors of a flat node
    if (Vector2Equals(a, b)) { //a.y == b.y && b.y == r.y) {
        // guaranteed to be a turning point?
        assert(flags);

#if 1
        // a and b are guaranteed to be the same!
        assert(a.x == b.x);
        r_prime = a;
#else
        // r_prime == a or b, whichever is farther from r
        r_prime = fabsf(a.x - r.x) > fabsf(b.x - r.x) ? a : b;
#endif

        // Furthest end of flat node should always be a closed interval at a turning point
        assert(floorf(r_prime.x) == r_prime.x);

        bool north = a.x == r.x && a.y < r.y;
        bool south = a.x == r.x && a.y > r.y;
        bool east = a.x > r.x && a.y == r.y;
        bool west = a.x < r.x && a.y == r.y;
        bool north_east = a.x > r.x && a.y < r.y;
        bool north_west = a.x < r.x && a.y < r.y;
        bool south_west = a.x < r.x && a.y > r.y;
        bool south_east = a.x > r.x && a.y > r.y;

        assert(north + south + east + west + north_east + north_west + south_west + south_east == 1);

        int search_dir_x = 0;
        int dir_y = 0;

        if (north) {
            dir_y = -1;
            if (flags & CORNER_SW) search_dir_x = -1;
            if (flags & CORNER_SE) search_dir_x =  1;
        } else if (south) {
            dir_y = 1;
            if (flags & CORNER_NW) search_dir_x = -1;
            if (flags & CORNER_NE) search_dir_x =  1;
        } else if (east) {
            // could be a cone successor of a flat node
            search_dir_x = 1;
            if (flags & CORNER_NW) dir_y = -1;
            if (flags & CORNER_SW) dir_y =  1;
        } else if (west) {
            // could be a cone successor of a flat node
            search_dir_x = -1;
            if (flags & CORNER_NE) dir_y = -1;
            if (flags & CORNER_SE) dir_y =  1;
        } else if (north_east) {
            if (flags & CORNER_NW) {
                // turn north and search west to find hidden cone
                dir_y = -1;
                search_dir_x = -1;
            }
            if (flags & CORNER_SE) {
                // turn east
                dir_y = -1;
                search_dir_x = 1;
            }
        } else if (north_west) {
            if (flags & CORNER_NE) {
                // turn north and search east to find hidden cone
                dir_y = -1;
                search_dir_x = 1;
            }
            if (flags & CORNER_SW) {
                // turn west
                dir_y = -1;
                search_dir_x = -1;
            }
        } else if (south_west) {
            if (flags & CORNER_NW) {
                // turn west
                dir_y = 1;
                search_dir_x = -1;
            }
            if (flags & CORNER_SE) {
                // turn south and search east for hidden cone
                dir_y = 1;
                search_dir_x = 1;
            }
        } else if (south_east) {
            if (flags & CORNER_NE) {
                // turn east
                dir_y = 1;
                search_dir_x = 1;
            }
            if (flags & CORNER_SW) {
                // turn south and search west for hidden cone
                dir_y = 1;
                search_dir_x = -1;
            }
        }

        //assert(dir_y);
        assert(search_dir_x);

        Vector2 p{};

        //int dir_y = r_prime.y < r.y ? -1 : r_prime.y > r.y ? 1 : 0;
        if (a.y == r.y) {
            // p = point from adjacent row, reached via right-hand turn at a
            // NOTE(dlb): Let's assume 'a' should be 'r_prime'.. turning at 'a' makes no sense...
            p = { r_prime.x, r_prime.y + dir_y };
        } else {
            assert(ClosestPointLineSegmentToRay({ 0, a.y + dir_y }, { 64, a.y + dir_y }, r, a, p));
        }
#if 0
        int x_min = p.x;
        int x_max = p.x;
#else
        // I = maximum closed interval beginning at p and observable from r_prime
        int y = p.y;
        int x_min = p.x;
        int x_max = p.x;
#endif

        int block_offset_y = dir_y == 1 ? -dir_y : 0;
        if (search_dir_x == 1) {
            while (!state.Query(x_max, p.y + block_offset_y)) {
                x_max++;
            }
        } else {
            while (!state.Query(x_min - 1, p.y + block_offset_y)) {
                x_min--;
            }
        }

        I.y = p.y;
        I.x_min = x_min;
        I.x_max = x_max;
        dbgColor = RED;

    // Non-observable successors of a cone node
    } else if (Vector2Equals(a, b)) {
        r_prime = a;

#if 0
        int dir_x = 0;
        int dir_y = 0;

        if (dir_x == 1) {
            // east
            if (state.Query(r_prime.x - 1, r_prime.y)) {
                // SW corner_flags, search down
                dir_y = 1;
            } else if (state.Query(r_prime.x - 1, r_prime.y - 1)) {
                // NW corner_flags, search up
                dir_y = -1;
            } else {
                assert(!"expected turning point!");
            }
        } else {
            // west
            if (state.Query(r_prime.x, r_prime.y - 1)) {
                // NE corner_flags, search up
                dir_y = -1;
            } else if (state.Query(r_prime.x, r_prime.y)) {
                // SE corner_flags, search down
                dir_y = 1;
            } else {
                assert(!"expected turning point!");
            }
        }
#endif

        // HACK(dlb): Hard-coded map width!!
        // p = point from adjacent row, computed via linear projection from r through a
        int dir_y = a.y - r.y > 0 ? 1 : -1;
        Vector2 p{};
        assert(ClosestPointLineSegmentToRay({ 0, a.y + dir_y }, { 64, a.y + dir_y }, r, a, p));

        // I = maximum closed interval beginning at p and observable from r_prime
        int y = p.y;
        int x_min = p.x;
        int x_max = p.x;

        int block_offset = dir_y == 1 ? -1 : 0;
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
#if 1
        int y = p.y;
        int x_min = floorf(p.x);
        int x_max = x_min;

        int block_offset = dir_a == 1 ? -1 : 0;
        //while (x_min > p.x && !state.Query(x_min - 1, y + block_offset)) {
        //    x_min--;
        //}
        bool is_blocked = false;
        while (!is_blocked && x_max < p_prime.x) {
            is_blocked = state.Query(x_max, y + block_offset);
            if (is_blocked) break;
            x_max++;
        }

        // I = maximum closed interval with endpoints p and p' (which is implicitly observable from r)
        if (!is_blocked) {
            I.y = p.y;
            I.x_min = p.x;
            I.x_max = p_prime.x;
            dbgColor = BLUE;
        } else {
            printf("");
        }
#else
        // I = maximum closed interval with endpoints p and p' (which is implicitly observable from r)
        I.y = p.y;
        I.x_min = p.x;
        I.x_max = p_prime.x;
        dbgColor = BLUE;
#endif
    }

    if (I.HasInterval()) {
        std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(*parent.state, r_prime, I);
        for (Anya_Interval &II : intervals) {
            assert(II.HasInterval());
            Anya_Node n_prime{ parent, r_prime, II };
            n_prime.dbgColor = dbgColor;
            state.nodes.push_back(n_prime);
        }
    }
}

void Anya_Flat_FlatSuccessors(Anya_Node &node)
{

}

void Anya_Flat_ConeSuccessors(Anya_Node &node)
{

}

void Anya_Successors(Anya_Node &node)
{
    if (node.IsStart()) {
        Anya_GenStartSuccessors(node);
        return;
    }

    Anya_State &state = *node.state;
    Anya_Interval I = node.interval;
    Vector2 r = node.root;

    std::vector<Anya_Node> successors{};

    if (state.next_id == 23) {
        printf("");
    }

    if (node.IsFlat()) {
        const Vector2 p0{ I.x_min, I.y };
        const Vector2 p1{ I.x_max, I.y };
        const float p0_dist = Vector2DistanceSqr(p0, r);
        const float p1_dist = Vector2DistanceSqr(p1, r);

        // p = endpoint of I farthest from r
        Vector2 p = p0_dist > p1_dist ? p0 : p1;

        int p_flags = 0;
        bool p_turn = Anya_IsTurningPoint(state, p, r, &p_flags);

        Anya_GenFlatSuccessors(node, p, r, p_flags);

        if (p_turn) {
            Anya_GenConeSuccessors(node, p, p, r, p_flags);
        }
    } else {
        Vector2 a = { I.x_min, I.y };
        Vector2 b = { I.x_max, I.y };

        int a_flags = 0;
        int b_flags = 0;
        bool a_corner = false;
        bool b_corner = false;
        bool a_turn = Anya_IsTurningPoint(state, a, r, &a_flags, &a_corner);
        bool b_turn = Anya_IsTurningPoint(state, b, r, &b_flags, &b_corner);

        Anya_GenConeSuccessors(node, a, b, r, 0);
#if 0
        if (a_corner) {
        }
        if (a_turn) {
            Anya_GenConeSuccessors(node, a, a, r, a_flags);
        }
#else
        if (a_turn) {
            Anya_GenFlatSuccessors(node, a, r, a_flags);
            Anya_GenConeSuccessors(node, a, a, r, a_flags);
        }
#endif
        if (b_turn) {
            Anya_GenFlatSuccessors(node, b, r, b_flags);
            Anya_GenConeSuccessors(node, b, b, r, b_flags);
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
        // if node.I has a closed endpoint that is also a corner_flags point {
        //     return false;
        // }
        Anya_Interval I_prime{};
        // TODO: I_prime = interval after projecting node.r through node.I
        // if I_prime contains any corner_flags points {
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
            return cost == rhs.cost ? id > rhs.id : cost > rhs.cost;
        }
    };
    std::priority_queue<PrioNode> open{};

    Anya_Node start = Anya_Node::StartNode(state);
    //Anya_Node start{ state, { 44, 25, 26 }, { 25, 45 } };

    open.push({ start.id, start.totalCost, start.interval.y });
    state.nodes.push_back(start);

    const int maxIters = 1000;
    int iters = 0;

    // TODO(dlb): This may still be necessary to prevent cycles:
    // Don't push nodes with equal or higher cost into the open queue.
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

        if (node.id == 18) {
            printf("");
        }

        if (node.interval.Contains(state.target)) {
            last_node = &node;
            break;
        }

        int successorsStart = state.nodes.size();
        Anya_Successors(node);
        int successorsEnd = state.nodes.size();
        for (int i = successorsStart; i < successorsEnd; i++) {
            Anya_Node &successor = state.nodes[i];
            if (!Anya_ShouldPrune(successor)) {
                open.push({ successor.id, successor.totalCost, successor.interval.y });

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
        grid_path.push(node->root);

        while (!grid_path.empty()) {
            state.path.push_back(Vector2Scale(grid_path.top(), TILE_W));
            grid_path.pop();
        }
    }
}