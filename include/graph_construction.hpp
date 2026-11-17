#ifndef GRAPH_CONSTRUCTION_HPP
#define GRAPH_CONSTRUCTION_HPP

#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <mutex>

#include <sqlite3.h>

#include <CGAL/squared_distance_2.h>

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"
#include "cache.hpp"

// Graph node, with numeric ID for Johnson's algorithm
class Node {
    size_t id;
    Segment seg;

public:
    Node() = default;
    Node(size_t id, const Segment& segment) : id{id}, seg{segment} {}

    size_t ID() const noexcept { return id; }
    const Segment& segment() const noexcept { return seg; }

    bool operator==(const Node& other) const noexcept {
        return id == other.id;
    }

    bool operator!=(const Node& other) const noexcept {
        return id != other.id;
    }

    operator size_t() const noexcept { return id; }
};

// Hash specialization that just uses the numeric ID of the node
namespace std {
    template <>
    struct hash<Node> {
        size_t operator()(const Node& node) const {
            return hash<size_t>()(node.ID());
        }
    };
}

size_t hash_value(const Node& node);

// Adjacency list representation of the graph
using Graph = std::unordered_map<
    Node,
    std::vector<std::tuple<Node, Vector, sqlite3_int64>>>;

using graph_iter = decltype(std::declval<Graph>().begin());
std::vector<std::ranges::subrange<graph_iter>> graph_views(
        Graph& g, 
        Cache& cache);
// -- GRAPH CONSTRUCTION ------------------------------------------------------

void add_layer_helper(const Polygon& w, Graph& g, fscalar l, fscalar o_max);

void add_layer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max);

std::optional<Point> shoot_ray(
    const HoledPolygon& w,
    const Point& source,
    const Vector& direction,
    const AABBTree& tree);

std::optional<Vector> has_edge(
    const HoledPolygon& w,
    const Segment& source,
    const Segment& target,
    hpscalar theta_max,
    const AABBTree& tree
    );

void process_view(
        const HoledPolygon& w, 
        std::ranges::subrange<graph_iter> graph,
        const fscalar& radius,
        const hpscalar& theta_max_hp,
        const AABBTree& tree,
        Cache& cache,
        std::mutex& graph_mutex
        );

Graph construct_graph(
    const HoledPolygon& w,
    const std::vector<std::pair<fscalar, fscalar>>& parameters,
    fscalar theta_max,
    fscalar radius,
    Cache& cache
    );

#endif
