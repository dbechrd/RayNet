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

inline bool Anya_Node::IsStart(void) const
{
    return interval.x_min == interval.x_max;
}

inline bool Anya_Node::IsFlat(void) const
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

bool Anya_State::AddNodeAndCheckTarget(Anya_Node &node)
{
    if (node.interval.Contains(target)) {
        node.dbgColor = ORANGE;
        target_found = true;
    }
    nodes.push_back(node);
    return target_found;
}

#define CORNER_NW 0b0001
#define CORNER_NE 0b0010
#define CORNER_SW 0b0100
#define CORNER_SE 0b1000

inline bool Anya_IsCorner(int flags)
{
    // If exactly 1 flag is set, then it's a corner
    return flags && ((flags & flags - 1) == 0);
}

inline bool Anya_IsFlatTurningPoint(int flags, Vector2 r, Vector2 p)
{
    if (!Anya_IsCorner(flags)) {
        return false;
    }

    if (p.x < r.x) {
        // | NE
        // *-----
        // p <- r  // flat node west
        // *-----
        // | SE
        return flags & CORNER_NE || flags & CORNER_SE;
    } else {
        //  NW  |
        // -----*
        // r -> p  // flat node east
        // -----*
        //  SW  |
        return flags & CORNER_NW || flags & CORNER_SW;
    }
    return false;
}

inline bool Anya_IsConeTurningPoint(int flags, Vector2 r, Vector2 p)
{
    if (!Anya_IsCorner(flags)) {
        return false;
    }

    if (p.y < r.y) {
        // cone north
        if (p.x < r.x) {
            // north west
            //        | NE
            //  ____  |____
            //   SW | p
            //      |  \
            //          r
            return flags & CORNER_NE || flags & CORNER_SW;
        } else if (p.x > r.x) {
            // north east
            //   NW |  
            //  ____|  ____
            //      p | SE
            //     /  |
            //    r    
            return flags & CORNER_NW || flags & CORNER_SE;
        } else {
            // north
            //  ___  p  ___
            //  SW | | | SE
            //     | r |
            //
            return flags & CORNER_SW || flags & CORNER_SE;
        }
    } else if (p.y > r.y) {
        // cone south
        if (p.x < r.x) {
            // south west
            //         r
            //  NW |  /
            //  ___| p ___
            //        | SE
            //        |
            //
            return flags & CORNER_NW || flags & CORNER_SE;
        } else if (p.x > r.x) {
            // south east
            //   r
            //    \  | NE
            // ___ p |___
            // SW |
            //    |
            //
            return flags & CORNER_NE || flags & CORNER_SW;
        } else {
            // south
            //         
            //  NW | r | NE
            //  ___| | |___
            //       p
            //
            return flags & CORNER_NW || flags & CORNER_NE;
        }
    }
    return false;
}

int Anya_QueryCornerFlags(Anya_State &state, Vector2 p)
{
    assert(floorf(p.y) == p.y);
    if (fabsf(floorf(p.x) - p.x) > ANYA_EPSILON_COMPARE) {
        return false;
    }

    int flags = 0;
    if (state.Query_NW(p.x, p.y)) flags |= CORNER_NW;
    if (state.Query_NE(p.x, p.y)) flags |= CORNER_NE;
    if (state.Query_SW(p.x, p.y)) flags |= CORNER_SW;
    if (state.Query_SE(p.x, p.y)) flags |= CORNER_SE;
    return flags;
}

// search west until corner or diagonal
int Anya_FlatBlockerWest(Anya_State &state, int x, int y, int flags)
{
    bool nw = flags & CORNER_NW;
    bool sw = flags & CORNER_SW;

    // Check if we're starting at a flat blocker
    if (nw && sw) {
        // _____
        // |x| |
        // |x|_|
        return x;
    } else if (nw && flags & CORNER_SE) {
        // _____
        // |x| |
        // |_|x|
        return x;
    } else if (sw && flags & CORNER_NE) {
        // _____
        // | |x|
        // |x|_|
        return x;
    }

    // Assumes x,y is a corner
    assert(Anya_IsCorner(flags));

    int x_min = x - 1;
    while (state.Query_NW(x_min, y) == nw &&
           state.Query_SW(x_min, y) == sw) {
        x_min--;
    }

    return x_min;
}

// search east until corner or diagonal
int Anya_FlatBlockerEast(Anya_State &state, int x, int y, int flags)
{
    bool ne = flags & CORNER_NE;
    bool se = flags & CORNER_SE;

    // Check if starting we're at a flat blocker
    if (ne && se) {
        // _____
        // | |x|
        // |_|x|
        return x;
    } else if (ne && flags & CORNER_SW) {
        // _____
        // | |x|
        // |x|_|
        return x;
    } else if (se && flags & CORNER_NW) {
        // _____
        // |x| |
        // |_|x|
        return x;
    }

    // Assumes x,y is a corner
    assert(Anya_IsCorner(flags));

    int x_max = x + 1;
    while (state.Query_NE(x_max, y) == ne &&
           state.Query_SE(x_max, y) == se) {
        x_max++;
    }

    return x_max;
}

bool Anya_IsTurningPoint(Anya_State &state, Vector2 p, Vector2 r, int *flags = 0, bool *is_corner = 0)
{
    int corner_flags = Anya_QueryCornerFlags(state, p);
    bool isCorner = Anya_IsCorner(corner_flags);
    
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

void Anya_SplitAtCorners(Anya_Node &parent, Vector2 r, Anya_Interval &I, Color color)
{
    if (!I.HasInterval()) {
        return;
    }
    
    Anya_State &state = *parent.state;

    if (I.Contains(state.target)) {
        Anya_Node node{ parent, r, I };
        state.AddNodeAndCheckTarget(node);
        return;
    }

    int y = I.y;
    int x0 = floorf(I.x_min + 1);
    int x1 = floorf(I.x_max);

    bool cone_up = I.y < r.y;
    bool cone_down = I.y > r.y;
    Anya_Interval segment{ I.y, I.x_min, -1 };

    bool nw = state.Query_NW(x0, y);
    bool sw = state.Query_SW(x0, y);

    for (int x = x0; x <= x1; x++) {
        // TODO(perf): We are checking twice as many cells as we need to for cone nodes, perhaps split it up somehow?
        bool ne = state.Query_NE(x, y);
        bool se = state.Query_SE(x, y);

        if (ne != nw || se != sw) {
            segment.x_max = x;

            if (segment.HasInterval()) {
                bool culDeSacCone = (cone_up && nw) || (cone_down && sw);
                if (!culDeSacCone) {
                    Anya_Node node{ parent, r, segment };
                    node.dbgColor = color;
                    if (state.AddNodeAndCheckTarget(node)) {
                        return;
                    }
                }
            }
            
            segment.x_min = segment.x_max + ANYA_EPSILON;
            segment.x_max = -1;
        }

        nw = ne;
        sw = se;
    }

    segment.x_max = I.x_max;
    if (segment.HasInterval()) {
        bool culDeSacCone = (cone_up && nw) || (cone_down && sw);
        if (!culDeSacCone) {
            Anya_Node node{ parent, r, segment };
            node.dbgColor = color;
            state.AddNodeAndCheckTarget(node);
        }
    }
}

void Anya_GenStartSuccessors(Anya_Node &start)
{
    Anya_State &state = *start.state;
    const Vector2 s = start.state->start;
        
    int flags = Anya_QueryCornerFlags(state, s);

    // Max interval for all visible points left of s [)
    {
        float x_min = Anya_FlatBlockerWest(state, s.x, s.y, flags);

        Anya_Interval I_max_left{ s.y, x_min, s.x - ANYA_EPSILON };
        Anya_SplitAtCorners(start, s, I_max_left, LIME);
        if (start.state->target_found) {
            return;
        }
    }

    // Max interval for all visible points right of s (]
    {
        float x_max = Anya_FlatBlockerEast(state, s.x, s.y, flags);

        Anya_Interval I_max_right{ s.y, s.x + ANYA_EPSILON, x_max };
        Anya_SplitAtCorners(start, s, I_max_right, LIME);
        if (start.state->target_found) {
            return;
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

        Anya_Interval I_max_above{ y, x_min, x_max };
        Anya_SplitAtCorners(start, s, I_max_above, LIME);
        if (start.state->target_found) {
            return;
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

        Anya_Interval I_max_below{ y, x_min, x_max };
        Anya_SplitAtCorners(start, s, I_max_below, LIME);
        if (start.state->target_found) {
            return;
        }
    }
}

void Anya_GenFlatSuccessors(Anya_Node &parent, Vector2 p, Vector2 r, int flags)
{
    Anya_State &state = *parent.state;

    assert(Anya_IsCorner(flags));

    bool west = false;

    if (p.y == r.y) {
        // flat node
        if (p.x < r.x) {
            // west
            west = true;
        } else if (p.x > r.x) {
            // east
            west = false;
        } else {
            assert(!"p equals r, ambiguous search direction");
        }
    } else if (p.y > r.y) {
        // cone south
        if (flags & CORNER_NW && (p.x <= r.x)) {
            // hit NW corner, while going S or SW
            west = true;
        } else if (flags & CORNER_NE && (p.x >= r.x)) {
            // hit NE corner, while going S or SE
            west = false;
        } else {
            //printf("this is not a turning point for a south cone\n");
            return;
        }
    } else if (p.y < r.y) {
        // cone north
        if (p.x <= r.x && flags & CORNER_SW) {
            // hit SW corner, while going N or NW
            west = true;
        } else if (flags & CORNER_SE && (p.x >= r.x)) {
            // hit SE corner, while going N or NE
            west = false;
        } else {
            //printf("this is not a turning point for a north cone\n");
            return;
        }
    }

    Anya_Interval I{};
    I.y = p.y;

    if (west) {
        I.x_min = Anya_FlatBlockerWest(*parent.state, p.x, p.y, flags);
        I.x_max = p.x - ANYA_EPSILON;
    } else {
        I.x_min = p.x + ANYA_EPSILON;
        I.x_max = Anya_FlatBlockerEast(*parent.state, p.x, p.y, flags);
    }

    if (I.HasInterval()) {
        if (r.y == p.y) {
            Anya_Node node{ parent, r, I };
            state.AddNodeAndCheckTarget(node);
        } else {
            Anya_Node node{ parent, p, I };
            state.AddNodeAndCheckTarget(node);
        }
    }
}

void Anya_GenConeSuccessorsFromFlat(Anya_Node &parent, Vector2 p, Vector2 r, int flags)
{
    Anya_State &state = *parent.state;

    // Non-observable successors of a flat node
    assert(flags);
    assert(p.y == r.y);
    assert(p.x != r.x);  // we should be going right or left, p cannot equal the root
    assert(floorf(p.x) == p.x);  // Furthest end of flat node should always be a closed interval at a turning point

    // Corner flags should have exactly 1 corner set
    assert(Anya_IsCorner(flags));

    bool north = (flags & CORNER_NW) || (flags & CORNER_NE);
    bool west = p.x < r.x;

    // I = maximum closed interval beginning at p_prime and observable from r_prime
    int y = p.y + (north ? -1 : 1);
    int x_min = p.x;
    int x_max = p.x;

    if (north) {
        if (west) {
            // north west
            while (!state.Query_SW(x_min, y)) {
                x_min--;
            }
        } else {
            // north east
            while (!state.Query_SE(x_max, y)) {
                x_max++;
            }
        }
    } else {
        if (west) {
            // south west
            while (!state.Query_NW(x_min, y)) {
                x_min--;
            }
        } else {
            // south east
            while (!state.Query_NE(x_max, y)) {
                x_max++;
            }
        }
    }

    Anya_Interval I{};
    I.y = y;
    I.x_min = x_min;
    I.x_max = x_max;

    Anya_SplitAtCorners(parent, p, I, BLUE);
}

void Anya_GenConeSuccessorsFromCone(Anya_Node &parent, Vector2 a, Vector2 b, Vector2 r)
{
    Anya_State &state = *parent.state;

    // HACK(dlb): Hard-coded map width!!
    const float MAP_WIDTH = 64;
    const float MAP_HEIGHT = 64;

    // Observable successors of a cone node
    // a,b is a normal interval
    assert(!Vector2Equals(a, b));

    // Query current row if above, or previous row if below
    bool cone_up = a.y < r.y;
    if (cone_up && a.y == 0) {
        return;  // nothing above top of map ;)
    }
    if (!cone_up && a.y == MAP_HEIGHT) {
        return;  // nothing below bottom of map ;)
    }
    if (cone_up && state.Query_NE(a.x, a.y)) {
        return;  // cone staring at a north wall, no successors
    }
    if (!cone_up && state.Query_SE(a.x, a.y)) {
        return;  // cone staring at a south wall, no successors
    }

    Vector2 p_a{};
    Vector2 p_b{};
        
    float projection_y = a.y + (cone_up ? -1 : 1);
    assert(ClosestPointLineSegmentToRay({ 0, projection_y }, { 64, projection_y }, r, a, p_a));
    assert(ClosestPointLineSegmentToRay({ 0, projection_y }, { 64, projection_y }, r, b, p_b));

    // Keep the floating point error caused by projection in check
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
            while (x_min > p_a.x && !state.Query_NW(x_min, a.y)) {
                x_min--;
            }
        } else {
            while (x_min > p_a.x && !state.Query_SW(x_min, a.y)) {
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
            while (x_max < p_b.x && !state.Query_NE(x_max, a.y)) {
                x_max++;
            }
        } else {
            while (x_max < p_b.x && !state.Query_SE(x_max, a.y)) {
                x_max++;
            }
        }
        x_max = MIN(x_max, p_b.x);
    }

    {
        Anya_Interval I_middle{};
        I_middle.y = projection_y;
        I_middle.x_min = x_min;
        I_middle.x_max = x_max;

        Anya_SplitAtCorners(parent, r, I_middle, WHITE);
        if (state.target_found) {
            return;
        }
    }

    bool aTurningPoint = Anya_IsTurningPoint(*parent.state, a, r);
    if (aTurningPoint) {
        // Find non-observable left cone
        float x_min_non_observable = floorf(x_min);

        if (cone_up) {
            while (!state.Query_NW(x_min_non_observable, a.y)) {
                x_min_non_observable--;
            }
        } else {
            while (!state.Query_SW(x_min_non_observable, a.y)) {
                x_min_non_observable--;
            }
        }

        Anya_Interval I_left{};
        I_left.y = projection_y;
        I_left.x_min = x_min_non_observable;
        I_left.x_max = x_min;

        Anya_SplitAtCorners(parent, a, I_left, RED);
        if (state.target_found) {
            return;
        }
    }

    bool bTurningPoint = Anya_IsTurningPoint(*parent.state, b, r);
    if (bTurningPoint) {
        // Find non-observable right cone
        float x_max_non_observable = ceilf(x_max);

        if (cone_up) {
            while (!state.Query_NE(x_max_non_observable, a.y)) {
                x_max_non_observable++;
            }
        } else {
            while (!state.Query_SE(x_max_non_observable, a.y)) {
                x_max_non_observable++;
            }
        }

        Anya_Interval I_right{};
        I_right.y = projection_y;
        I_right.x_min = x_max;
        I_right.x_max = x_max_non_observable;

        Anya_SplitAtCorners(parent, b, I_right, RED);
        if (state.target_found) {
            return;
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

        int flags = Anya_QueryCornerFlags(state, p);
        if (Anya_IsCorner(flags)) {
            Anya_GenFlatSuccessors(node, p, r, flags);
            if (state.target_found) return;
        }
        
        if (Anya_IsFlatTurningPoint(flags, r, p)) {
            Anya_GenConeSuccessorsFromFlat(node, p, r, flags);
            if (state.target_found) return;
        }
    } else {
        Vector2 a = { I.x_min, I.y };
        Vector2 b = { I.x_max, I.y };

        int a_flags = Anya_QueryCornerFlags(state, a);
        int b_flags = Anya_QueryCornerFlags(state, b);

        Anya_GenConeSuccessorsFromCone(node, a, b, r);
        if (state.target_found) return;

        if (Anya_IsConeTurningPoint(a_flags, r, a)) {
            Anya_GenFlatSuccessors(node, a, r, a_flags);
            if (state.target_found) return;
        }

        if (Anya_IsConeTurningPoint(b_flags, r, b)) {
            Anya_GenFlatSuccessors(node, b, r, b_flags);
            if (state.target_found) return;
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

inline bool Anya_ShouldPrune(Anya_Node &node)
{
    assert(node.interval.HasInterval());
    return Anya_IsCulDeSac(node) || Anya_IsIntermediate(node);
}
#else
bool Anya_ShouldPrune(Anya_Node &node)
{
    return false;
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

    Anya_Node *target_node = 0;
    while (iters < maxIters && !target_node && !open.empty()) {
        const PrioNode &prioNode = open.top();
        Anya_Node node = state.nodes[prioNode.id];
        assert(node.interval.y == prioNode.y);
        state.nodeSearchOrder.push_back(node);
        open.pop();

        if (node.interval.Contains(state.target)) {
            target_node = &node;
            break;
        }

        int successorsStart = state.nodes.size();
        Anya_Successors(node);
        int successorsEnd = state.nodes.size();

        if (state.target_found) {
            target_node = &state.nodes[state.nodes.size() - 1];
            break;
        }

        for (int i = successorsStart; i < successorsEnd; i++) {
            Anya_Node &successor = state.nodes[i];
            if (successor.interval.Contains(state.target)) {
                // If successor contains target, we're done, don't bother doing more work.
                target_node = &successor;
                break;
            }

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

    if (target_node) {
        std::stack<Vector2> grid_path{};
        grid_path.push(state.target);

        Anya_Node *node = target_node;
        while (node->parent >= 0) {
            grid_path.push(node->root);
            node = &state.nodes[node->parent];
        }
        grid_path.push(node->root);

        const float nudge = radius / 1.4142135f;
        while (!grid_path.empty()) {
            Vector2 gridPos = grid_path.top();
            Vector2 worldPos = Vector2Scale(gridPos, TILE_W);
            int flags = Anya_QueryCornerFlags(state, gridPos);
            if (Anya_IsCorner(flags)) {
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