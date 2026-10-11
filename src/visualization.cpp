#include "visualization.hpp"

#include <CGAL/draw_polygon_2.h>
#include <CGAL/draw_arrangement_2.h>
#include <CGAL/Random.h>

#include "cgal_types.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------
namespace detail {
    Color random_color(size_t seed) {
        CGAL::Random rand(seed);
        return CGAL::get_random_color(rand);
    }

    void visualize_holed_polygon(
            const HoledPolygon& hole_polygon, 
            Scene& scene, 
            Color boundary_color, 
            Color hole_color) {
        PolygonSceneOptions hole_options; 
        hole_options.colored_vertex = [](const Polygon&, Polygon::Vertex_const_iterator) { return false; };
        hole_options.colored_edge = [](const Polygon&, Polygon::Vertex_const_iterator) { return true; };
        hole_options.edge_color = [](const Polygon&, Polygon::Vertex_const_iterator) { return Color(0, 0, 0); };
        hole_options.colored_face = [](const Polygon&, void*) { return true; };
        hole_options.face_color = [&](const Polygon&, void*) { return hole_color; };
    
        for (auto hole : hole_polygon.holes()) {
            hole.reverse_orientation();
            CGAL::add_to_graphics_scene(hole, scene, hole_options);
        }
    
        PolygonSceneOptions wall_boundary_options;
        wall_boundary_options.colored_vertex = [](const Polygon&, Polygon::Vertex_const_iterator) { return false; };
        wall_boundary_options.colored_edge = [](const Polygon&, Polygon::Vertex_const_iterator) { return true; };
        wall_boundary_options.edge_color = [](const Polygon&, Polygon::Vertex_const_iterator) { return Color(0, 0, 0); };
        wall_boundary_options.colored_face = [](const Polygon&, void*) { return true; };
        wall_boundary_options.face_color = [&](const Polygon&, void*) { return boundary_color; };
    
        CGAL::add_to_graphics_scene(hole_polygon.outer_boundary(), scene, wall_boundary_options);
    }

    void visualize_polygon_set(
            const PolygonSet& polygon_set,
            Scene& scene,
            Color coverage_color) {
        PolygonSetSceneOptions coverage_options;
        coverage_options.colored_vertex = [](const PolygonSet::Arrangement_2&, PolygonSet::Arrangement_2::Vertex_const_handle) { return false; };
        coverage_options.colored_edge = [](const PolygonSet::Arrangement_2&, PolygonSet::Arrangement_2::Halfedge_const_handle) { return true; };
        coverage_options.edge_color = [](const PolygonSet::Arrangement_2&, PolygonSet::Arrangement_2::Halfedge_const_handle) { return Color(0, 0, 0); };
        coverage_options.colored_face = [](const PolygonSet::Arrangement_2&, PolygonSet::Arrangement_2::Face_const_handle) { return true; };
        coverage_options.face_color = [&](const PolygonSet::Arrangement_2&, PolygonSet::Arrangement_2::Face_const_handle) { return coverage_color; };

        CGAL::add_to_graphics_scene(polygon_set.arrangement(), scene, coverage_options);
    }
}
