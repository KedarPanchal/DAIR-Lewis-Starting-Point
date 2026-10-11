#ifndef VISUALIZATION_HPP
#define VISUALIZATION_HPP

#include <ranges>
#include <concepts>

#include <CGAL/IO/Color.h>
#include <CGAL/Graphics_scene.h>
#include <CGAL/draw_polygon_2.h>
#include <CGAL/Graphics_scene_options.h>

#include "cgal_types.hpp"
#include "utilities.hpp"
#include "start_nodes.hpp"

using Color = CGAL::IO::Color;
using Scene = CGAL::Graphics_scene;
using PolygonSceneOptions = CGAL::Graphics_scene_options<Polygon, Polygon::Vertex_const_iterator, Polygon::Vertex_const_iterator, void*>;
using PolygonSetSceneOptions = CGAL::Graphics_scene_options<
    PolygonSet::Arrangement_2, 
    PolygonSet::Arrangement_2::Vertex_const_handle, 
    PolygonSet::Arrangement_2::Halfedge_const_handle,
    PolygonSet::Arrangement_2::Face_const_handle>;

namespace detail {
    Color random_color(size_t seed);
    void visualize_holed_polygon(
            const HoledPolygon& hole_polygon, 
            Scene& scene, 
            Color boundary_color,
            Color hole_color
            );
    void visualize_polygon_set(
            const PolygonSet& polygon_set,
            Scene& scene,
            Color coverage_color
            );
}

template <typename R>
requires std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, SCC>
void visualize(const HoledPolygon& wall_space, const R& sccs) {
    Scene scene;
    detail::visualize_holed_polygon(wall_space, scene, Color(255, 255, 255), Color(0, 0, 0));

    for (const auto& [scc_graph, scc_coverage, scc_area] : sccs) {
        Color coverage_color = detail::random_color(convert<size_t>(scc_area));
        detail::visualize_polygon_set(scc_coverage, scene, coverage_color);
    }

    CGAL::draw_graphics_scene(scene);
}

#endif
