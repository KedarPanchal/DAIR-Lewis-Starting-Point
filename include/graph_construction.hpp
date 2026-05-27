#ifndef GRAPH_CONSTRUCTION_HPP
#define GRAPH_CONSTRUCTION_HPP

#include <list>
#include <unordered_map>
#include <optional>
#include <tuple>

#include <CGAL/squared_distance_2.h>

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

// Graph node, with numeric ID for Johnson's algorithm
class Node {
    size_t id;
    Segment seg;

public:
    Node(size_t id, const Segment& segment) : id{id}, seg{segment} {}

    size_t ID() const { return id; }
    Segment segment() const { return seg; }

    bool operator==(const Node& other) const { return id == other.id; }
    bool operator!=(const Node& other) const { return id != other.id; }
    operator size_t() const { return id; }
};

// Hash specialization that just uses the numeric ID of the node
namespace std {
    template<>
    struct hash<Node> {
        size_t operator()(const Node& node) const {
            return hash<size_t>()(node.ID());
        }
    };
}

size_t hash_value(const Node& node);

// Adjacency list representation of the graph
using Graph = std::unordered_map<Node, std::list<std::tuple<Node, Vector, PolygonSet>>>;

void addLayerHelper(const Polygon& w, Graph& g, fscalar l, fscalar o_max);

void addLayer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max);

std::optional<Point> shootRay(const HoledPolygon& w, const Point& source, const Vector& direction, const AABBTree& tree);

std::optional<Vector> hasEdge(const HoledPolygon& w, const Segment& source, const Segment& target, hpscalar theta_max, const AABBTree& tree);

Graph construct_graph(const HoledPolygon& w, const std::list<std::pair<fscalar, fscalar>>& parameters, fscalar theta_max, fscalar radius);

#endif
