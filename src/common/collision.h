#pragma once
#include "common.h"

struct Circle2 {
    Vector2 center{};
    float radius{};
};

struct LineSegment2 {
    Vector2 start{};
    Vector2 end{};

    LineSegment2(void) {}
    LineSegment2(Vector2 start, Vector2 end) : start(start), end(end) {}
};

struct Edge {
    struct Key {
        Vector2 vertex{};
        Vector2 normal{};

        Key(Vector2 vertex, Vector2 normal) : vertex(vertex), normal(normal) {};

        bool operator==(const Key &other) const
        {
            return vertex.x == other.vertex.x
                && vertex.y == other.vertex.y
                && normal.x == other.normal.x
                && normal.y == other.normal.y;
        }

        struct Hasher {
            size_t operator()(const Key &key) const
            {
                size_t hash = 0;
                hash_combine(hash, key.vertex.x, key.vertex.y, key.normal.x, key.normal.y);
                return hash;
            }
        };
    };

    bool operator==(const Edge &other) const
    {
        return line.start.x == other.line.start.x
            && line.start.y == other.line.start.y
            && line.end.x == other.line.end.x
            && line.end.y == other.line.end.y;
    }

    struct Hasher {
        size_t operator()(const Edge &edge) const
        {
            size_t hash = 0;
            hash_combine(hash, edge.line.start.x, edge.line.start.y, edge.line.end.x, edge.line.end.y);
            return hash;
        }
    };

    typedef std::unordered_map<Edge::Key, size_t, Edge::Key::Hasher> Map;
    typedef std::unordered_set<Edge, Edge::Hasher> Set;
    typedef std::vector<Edge> Array;

    LineSegment2 line{};
    Vector2 normal{};

    Edge(void) {}

    Edge(LineSegment2 line) : line(line) {
        const Vector2 edge = Vector2Subtract(line.end, line.start);
        const Vector2 normal{ edge.y, -edge.x };
        this->normal = Vector2Normalize(normal);
    }

    Key KeyStart(void) const {
        return Key{ line.start, normal };
    }

    Key KeyEnd(void) const {
        return Key{ line.end, normal };
    }

    Vector2 Midpoint(void) const {
        const Vector2 edge_vec = Vector2Subtract(line.end, line.start);
        const Vector2 edge_mid = Vector2Add(line.start, Vector2Scale(edge_vec, 0.5f));
        return edge_mid;
    }

    void Add(Edge other) {
        if (Vector2Equals(normal, other.normal)) {
            if (Vector2Equals(line.end, other.line.start)) {
                // Add other edge to end
                line.end = other.line.end;
            } else if (Vector2Equals(line.start, other.line.end)) {
                // Add other edge to beginning
                line.start = other.line.start;
            } else {
                assert(!"Expected edges to have a matching end/start or start/end vertex");
            }
        } else {
            assert(!"Why are you trying to add edges with differet normals!?");
        }
    }

    void Draw(Color color, float thickness = 1.0f, bool withNormal = true) const {
        // Draw edge
        DrawLineEx(line.start, line.end, thickness, color);

        // Draw normal
        if (withNormal) {
            const Vector2 edge_mid = Midpoint();
            const Vector2 normal_scaled = Vector2Scale(normal, 8);
            DrawLineEx(edge_mid, Vector2Add(edge_mid, normal_scaled), thickness, SKYBLUE);
        }
    }
};

struct Manifold {
    Vector2 contact;
    Vector2 normal;
    float depth;
};

struct Collision {
    float dist_sq{};  // distance squared from POC
    float dot_vel{};  // dot product with velocity
    Rectangle rect{};
    Edge *edge{};
    Manifold manifold{};
    Color col{};

    bool operator<(const Collision& rhs) const {
        //return dist_sq < rhs.dist_sq;
        //return fabsf(dot_vel) < fabsf(rhs.dot_vel);
        return manifold.depth < rhs.manifold.depth;
    }

    bool operator>(const Collision& rhs) const {
        //return dist_sq < rhs.dist_sq;
        //return fabsf(dot_vel) < fabsf(rhs.dot_vel);
        return manifold.depth > rhs.manifold.depth;
    }
};

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec);
bool dlb_CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec, Manifold *manifold);
bool dlb_CheckCollisionCircleRec2(Vector2 center, float radius, Rectangle rec, Manifold *manifold);
bool dlb_CheckCollisionCircleEdge(Vector2 center, float radius, Edge edge, Manifold *manifold);
