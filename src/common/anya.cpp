#include "anya.h"

#define ANYA_EPSILON 0.0f //0.0001f
#define ANYA_EPSILON_COMPARE 0.0001f

Anya_Interval::Anya_Interval()
{}

Anya_Interval::Anya_Interval(float y, float x_min, float x_max) : y(y), x_min(x_min), x_max(x_max)
{}

bool Anya_Interval::HasInterval(void) const
{
    return (x_max - x_min) > ANYA_EPSILON_COMPARE;
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
    return p.y == y
        && p.x >= x_min - ANYA_EPSILON_COMPARE
        && p.x <= x_max + ANYA_EPSILON_COMPARE;
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

//inline bool Anya_State::Query(int x, int y)
//{
//    return solid_query(x, y, userdata);
//}

inline bool Anya_State::Query_NW(int x, int y)
{
    return solid_query(x - 1, y - 1, userdata);
}

inline bool Anya_State::Query_NE(int x, int y)
{
    return solid_query(x, y - 1, userdata);
}

inline bool Anya_State::Query_SW(int x, int y)
{
    return solid_query(x - 1, y, userdata);
}

inline bool Anya_State::Query_SE(int x, int y)
{
    return solid_query(x, y, userdata);
}

#define CORNER_NW 0b0001
#define CORNER_NE 0b0010
#define CORNER_SW 0b0100
#define CORNER_SE 0b1000

bool Anya_FlagsIsCorner(int flags)
{
    return flags && ((flags & flags - 1) == 0);
}

bool Anya_IsCorner(Anya_State &state, float x, float y, int *flags = 0)
{
    assert(floorf(y) == y);
    if (fabsf(floorf(x) - x) > ANYA_EPSILON_COMPARE) {
        return false;
    }

    bool nw = state.Query_NW(x, y);
    bool ne = state.Query_NE(x, y);
    bool sw = state.Query_SW(x, y);
    bool se = state.Query_SE(x, y);

    if (flags) {
        if (nw) *flags |= CORNER_NW;
        if (ne) *flags |= CORNER_NE;
        if (sw) *flags |= CORNER_SW;
        if (se) *flags |= CORNER_SE;
    }

    // If only 1 flag is set, then it's a corner
    return (nw + ne + sw + se) == 1;
}

// search west until corner or diagonal
int Anya_FlatBlockerWest(Anya_State &state, int x, int y, int flags)
{
    // Assumes x,y is a corner
    assert(Anya_IsCorner(state, x, y));

    bool top = flags & CORNER_NW;
    bool bot = flags & CORNER_SW;

    int x_min = x - 1;
    while (state.Query_NW(x_min, y) == top &&
           state.Query_SW(x_min, y) == bot) {
        x_min--;
    }

    return x_min;
}

// search east until corner or diagonal
int Anya_FlatBlockerEast(Anya_State &state, int x, int y, int flags)
{
    // Assumes x,y is a corner
    assert(Anya_IsCorner(state, x, y));

    bool top = flags & CORNER_NE;
    bool bot = flags & CORNER_SE;

    int x_max = x + 1;
    while (state.Query_NE(x_max, y) == top &&
           state.Query_SE(x_max, y) == bot) {
        x_max++;
    }

    return x_max;
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

    const bool nw = corner_flags & CORNER_NW;
    const bool ne = corner_flags & CORNER_NE;
    const bool sw = corner_flags & CORNER_SW;
    const bool se = corner_flags & CORNER_SE;

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

    // TODO(perf): We are double-checking cells many times with IsCorner, make it directional to speed it up.
    for (int x = x0; x <= x1; x++) {
        if (Anya_IsCorner(state, (float)x, I.y)) {
            segment.x_max = x;

            if (segment.HasInterval()) {
                intervals.push_back(segment);
            }
            segment.x_min = segment.x_max + ANYA_EPSILON;
            segment.x_max = -1;
        }
    }

    segment.x_max = I.x_max;
    if (segment.HasInterval()) {
        intervals.push_back(segment);
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

    // Max interval for all visible points left of s [)
    {
        const float y = s.y;
        float x_min = s.x;

        for (;;) {
            bool nw = state.Query_NW(x_min, y);
            bool sw = state.Query_SW(x_min, y);
            if (nw && sw) {
                // _____
                // |x| |
                // |x|_|
                break;
            } else if (nw && state.Query_SE(x_min, y)) {
                // _____
                // |x| |
                // |_|x|
                break;
            } else if (sw && state.Query_NE(x_min, y)) {
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
        float x_max = s.x;

        for (;;) {
            bool ne = state.Query_NE(x_max, s.y);
            bool se = state.Query_SE(x_max, s.y);
            if (ne && se) {
                // _____
                // | |x|
                // |_|x|
                break;
            } else if (ne && state.Query_SW(x_max, s.y)) {
                // _____
                // | |x|
                // |x|_|
                break;
            } else if (se && state.Query_NW(x_max, s.y)) {
                // _____
                // |x| |
                // |_|x|
                break;
            }
            x_max += 1;
        }

        Anya_Interval I_max_right{ s.y, s.x + ANYA_EPSILON, x_max };
        if (I_max_right.HasInterval()) {
            max_intervals.push_back(I_max_right);
        }
    }

    // Max interval for all visible points in row above s []
    {
        const float y = s.y - 1;
        float x_min = s.x;
        float x_max = s.x;

        while (!state.Query_SW(x_min, y)) {
            // _____
            // | | |
            // |x|_|
            x_min--;
        }
        while (!state.Query_SE(x_max, y)) {
            // _____
            // | | |
            // |_|x|
            x_max++;
        }

        Anya_Interval I_max_up{ y, x_min, x_max };
        if (I_max_up.HasInterval()) {
            max_intervals.push_back(I_max_up);
        }
    }

    // Max interval for all visible points in row below s []
    {
        const float y = s.y + 1;
        float x_min = s.x;
        float x_max = s.x;

        while (!state.Query_NW(x_min, y)) {
            // _____
            // |x| |
            // |_|_|
            x_min--;
        }
        while (!state.Query_NE(x_max, y)) {
            // _____
            // | |x|
            // |_|_|
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
            Anya_Node node{ start, s, split_interval };
            state.nodes.push_back(node);
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
            while (!state.Query_SE(x_max, p.y + block_offset_y)) {
                x_max++;
            }
        } else {
            while (!state.Query_SW(x_min, p.y + block_offset_y)) {
                x_min--;
            }
        }

        I.y = p.y;
        I.x_min = x_min;
        I.x_max = x_max;
        dbgColor = BLUE;
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

void Anya_GenFlatSuccessors(Anya_Node &parent, Vector2 p, Vector2 r, int flags)
{
    if (!Anya_FlagsIsCorner(flags)) {
        return;
    }

    int x_dir = 0;

    if (p.y == r.y) {
        // flat node
        if (p.x < r.x) {
            // west
            x_dir = -1;
        } else if (p.x > r.x) {
            // east
            x_dir = 1;
        }
    } else if (p.y > r.y) {
        // cone south
        if (flags & CORNER_NW && (p.x <= r.x)) {
            // hit NW corner, while going S or SW
            x_dir = -1;
        } else if (flags & CORNER_NE && (p.x >= r.x)) {
            // hit NE corner, while going S or SE
            x_dir = 1;
        }
    } else if (p.y < r.y) {
        // cone north
        if (flags & CORNER_SW && (p.x <= r.x)) {
            // hit SW corner, while going N or NW
            x_dir = -1;
        } else if (flags & CORNER_SE && (p.x >= r.x)) {
            // hit SE corner, while going N or NE
            x_dir = 1;
        }
    }

    Anya_Interval I{};

    if (x_dir == -1) {
        I.y = p.y;
        I.x_min = Anya_FlatBlockerWest(*parent.state, p.x, p.y, flags);
        I.x_max = p.x - ANYA_EPSILON;
    } else if (x_dir == 1) {
        I.y = p.y;
        I.x_min = p.x + ANYA_EPSILON;
        I.x_max = Anya_FlatBlockerEast(*parent.state, p.x, p.y, flags);
    }

    if (I.HasInterval()) {
        if (r.y == p.y) {
            parent.state->nodes.push_back({ parent, r, I });
        } else {
            parent.state->nodes.push_back({ parent, p, I });
        }
    }
}

void Anya_GenConeSuccessorsNew(Anya_Node &parent, Vector2 a, Vector2 b, Vector2 r, int flags)
{
    std::vector<Vector2> roots{};
    std::vector<Anya_Interval> max_intervals{};
    std::vector<Color> colors{};

    const float MAP_WIDTH = 64;
    const float MAP_HEIGHT = 64;

    // Observable successors of a cone node
    // a,b is a normal interval
    if (!Vector2Equals(a, b)) {
        // Query current row if above, or previous row if below
        bool cone_up = a.y < r.y;
        if (cone_up && a.y == 0) {
            return;  // nothing above top of map ;)
        }
        if (!cone_up && a.y == MAP_HEIGHT) {
            return;  // nothing below bottom of map ;)
        }
        if (cone_up && parent.state->Query_NE(a.x, a.y)) {
            return;  // cone staring at a north wall, no successors
        }
        if (!cone_up && parent.state->Query_SE(a.x, a.y)) {
            return;  // cone staring at a south wall, no successors
        }

        Vector2 p_a{};
        Vector2 p_b{};
        
        // HACK(dlb): Hard-coded map width!!
        float projection_y = a.y + (cone_up ? -1 : 1);
        assert(ClosestPointLineSegmentToRay({ 0, projection_y }, { 64, projection_y }, r, a, p_a));
        assert(ClosestPointLineSegmentToRay({ 0, projection_y }, { 64, projection_y }, r, b, p_b));

        p_a.x = roundf(p_a.x * 10000) / 10000;
        p_b.x = roundf(p_b.x * 10000) / 10000;

        float x_min{};
        float x_max{};

        if (p_a.x >= a.x) {
            x_min = p_a.x;
        } else {
            // Search left until you hit p_a or a wall
            x_min = ceilf(a.x);
            if (cone_up) {
                while (x_min > p_a.x && !parent.state->Query_NW(x_min, a.y)) {
                    x_min--;
                }
            } else {
                while (x_min > p_a.x && !parent.state->Query_SW(x_min, a.y)) {
                    x_min--;
                }
            }
            x_min = MAX(x_min, p_a.x);
        }

        if (p_b.x <= b.x) {
            x_max = p_b.x;
        } else {
            // Search right until you hit p_b or a wall
            x_max = floorf(b.x);
            if (cone_up) {
                while (x_max < p_b.x && !parent.state->Query_NE(x_max, a.y)) {
                    x_max++;
                }
            } else {
                while (x_max < p_b.x && !parent.state->Query_SE(x_max, a.y)) {
                    x_max++;
                }
            }
            x_max = MIN(x_max, p_b.x);
        }

        Anya_Interval I_middle{};
        I_middle.y = projection_y;
        I_middle.x_min = x_min;
        I_middle.x_max = x_max;

        if (I_middle.HasInterval()) {
            roots.push_back(r);
            max_intervals.push_back(I_middle);
            colors.push_back(WHITE);
        }

        bool aTurningPoint = Anya_IsTurningPoint(*parent.state, a, r);
        if (aTurningPoint) {
            // Find non-observable left cone
            float x_min_non_observable = floorf(x_min);

            if (cone_up) {
                while (!parent.state->Query_NW(x_min_non_observable, a.y)) {
                    x_min_non_observable--;
                }
            } else {
                while (!parent.state->Query_SW(x_min_non_observable, a.y)) {
                    x_min_non_observable--;
                }
            }

            Anya_Interval I_left{};
            I_left.y = projection_y;
            I_left.x_min = x_min_non_observable;
            I_left.x_max = x_min;

            if (I_left.HasInterval()) {
                roots.push_back(a);
                max_intervals.push_back(I_left);
                colors.push_back(RED);
            }
        }

        bool bTurningPoint = Anya_IsTurningPoint(*parent.state, b, r);
        if (bTurningPoint) {
            // Find non-observable right cone
            float x_max_non_observable = ceilf(x_max);

            if (cone_up) {
                while (!parent.state->Query_NE(x_max_non_observable, a.y)) {
                    x_max_non_observable++;
                }
            } else {
                while (!parent.state->Query_SE(x_max_non_observable, a.y)) {
                    x_max_non_observable++;
                }
            }

            Anya_Interval I_right{};
            I_right.y = projection_y;
            I_right.x_min = x_max;
            I_right.x_max = x_max_non_observable;

            if (I_right.HasInterval()) {
                roots.push_back(b);
                max_intervals.push_back(I_right);
                colors.push_back(RED);
            }
        }
#if 0
    } else if (a.y != r.y) {
        // Non-observable successors of a cone node
        // a,a of cone, or b,b of cone
        r_prime = a;
        color = RED;

        // Query current row if above, or previous row if below
        bool cone_up = a.y < r.y;
        if (cone_up && a.y == 0) {
            return;  // nothing above top of map ;)
        }
        if (!cone_up && a.y == MAP_HEIGHT) {
            return;  // nothing below bottom of map ;)
        }

        Vector2 p{};

        // HACK(dlb): Hard-coded map width!!
        float projection_y = a.y + (cone_up ? -1 : 1);
        assert(ClosestPointLineSegmentToRay({ 0, projection_y }, { 64, projection_y }, r, a, p));

        float x_min{};
        float x_max{};

        if (cone_up) {
            // north
            if (flags & CORNER_NE) {
                // inside corner
                I.y = projection_y;
                I.x_min = p.x;
                I.x_max = a.y + projection_y;
            } else if (flags & CORNER_SW) {

            }
        } else {
            // south
        }

        if (a.x < r.x) {
            I.y = projection_y;
            I.x_min = p.x;
            I.x_max = a.y + projection_y;
        } else if (a.x > r.x) {
            I.y = projection_y;
            I.x_min = a.y + projection_y;
            I.x_max = p.x;
        }
#endif
    } else {
        // TODO: Non-observable successors of a flat node
        assert(a.x == b.x);
        assert(a.y == b.y);
        assert(a.y == r.y);

        //r_prime = a;
        //color = BLUE;
    }
    
    assert(roots.size() == max_intervals.size());
    assert(max_intervals.size() == colors.size());

    for (int i = 0; i < roots.size(); i++) {
        if (max_intervals[i].HasInterval()) {
            std::vector<Anya_Interval> intervals = Anya_SplitAtCorners(*parent.state, roots[i], max_intervals[i]);
#if 0
            std::vector<Anya_Interval> smaller_intervals{};
            for (Anya_Interval &II : intervals) {
                if (II.x_max - II.x_min > 8) {
                    float split_at = II.x_min + (II.x_max - II.x_min) * 0.5f;
                    Anya_Interval a{ II.y, II.x_min, split_at };
                    Anya_Interval b{ II.y, split_at, II.x_max };
                    smaller_intervals.push_back(a);
                    smaller_intervals.push_back(b);
                } else {
                    smaller_intervals.push_back(II);
                }
            }
            for (Anya_Interval &II : smaller_intervals) {
                assert(II.HasInterval());
                Anya_Node node{ parent, roots[i], II };
                node.dbgColor = colors[i];
                parent.state->nodes.push_back(node);
            }
#else
            for (Anya_Interval &II : intervals) {
                assert(II.HasInterval());
                Anya_Node node{ parent, roots[i], II };
                node.dbgColor = colors[i];
                parent.state->nodes.push_back(node);
            }
#endif
        }
    }
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
        bool a_turn = Anya_IsTurningPoint(state, a, r, &a_flags); //, &a_corner);
        bool b_turn = Anya_IsTurningPoint(state, b, r, &b_flags); //, &b_corner);

        Anya_GenConeSuccessorsNew(node, a, b, r, 0);
        if (a_turn) {
            Anya_GenFlatSuccessors(node, a, r, a_flags);
            //Anya_GenConeSuccessorsNew(node, a, a, r, a_flags);
        }
        if (b_turn) {
            Anya_GenFlatSuccessors(node, b, r, b_flags);
            //Anya_GenConeSuccessorsNew(node, b, b, r, b_flags);
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
        // TODO: p = endpoint of node.I farthest from node.root

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

void Anya(Anya_State &state, float radius)
{
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

    // TODO(dlb): This may still be necessary to prevent cycles:
    // Don't push nodes with equal or higher cost into the open queue.
#if 1
    auto Vec2Hash = [](const Vector2 &v) { return hash_combine(v.x, v.y); };
    auto Vec2Equal = [](const Vector2 &l, const Vector2 &r) { return l.x == r.x && l.y == r.y; };
    std::unordered_map<Vector2, float, decltype(Vec2Hash), decltype(Vec2Equal)> root_costs{};
#endif

    const int maxIters = 10000;
    int iters = 0;
    state.nodes.reserve(maxIters);

    Anya_Node start = Anya_Node::StartNode(state);
    open.push({ start.id, start.totalCost, start.interval.y });
    state.nodes.push_back(start);

    Anya_Node *last_node = 0;
    while (!open.empty() && iters < maxIters) {
        const PrioNode &prioNode = open.top();
        Anya_Node node = state.nodes[prioNode.id];
        assert(node.interval.y == prioNode.y);
        open.pop();
        state.nodeSearchOrder.push_back(node);

        if (node.interval.Contains(state.target)) {
            last_node = &node;
            break;
        }

        int successorsStart = state.nodes.size();
        Anya_Successors(node);
        int successorsEnd = state.nodes.size();
        for (int i = successorsStart; i < successorsEnd; i++) {
            Anya_Node &successor = state.nodes[i];
            const auto &root_cost = root_costs.find(successor.root);
            if (root_cost == root_costs.end() || root_cost->second == 0 || successor.rootCost <= root_cost->second) {
                if (!Anya_ShouldPrune(successor)) {
                    open.push({ successor.id, successor.totalCost, successor.interval.y });

                    successor.orig_parent = successor.parent;
                    if (Vector2Equals(successor.root, node.root)) {
                        successor.parent = node.parent;
                    }

                    root_costs[successor.root] = successor.rootCost;
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

        const float nudge = radius / 1.4142135f;
        while (!grid_path.empty()) {
            Vector2 gridPos = grid_path.top();
            Vector2 worldPos = Vector2Scale(gridPos, TILE_W);
            int flags = 0;
            if (Anya_IsCorner(state, gridPos.x, gridPos.y, &flags)) {
                if (flags & CORNER_NW) {
                    worldPos.x += nudge;
                    worldPos.y += nudge;
                } else if (flags & CORNER_NE) {
                    worldPos.x -= nudge;
                    worldPos.y += nudge;
                } else if (flags & CORNER_SE) {
                    worldPos.x -= nudge;
                    worldPos.y -= nudge;
                } else if (flags & CORNER_SW) {
                    worldPos.x += nudge;
                    worldPos.y -= nudge;
                }
            }
            state.path.push_back(worldPos);
            grid_path.pop();
        }
    }
}