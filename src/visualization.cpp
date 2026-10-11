#include "visualization.hpp"

#include <CGAL/draw_polygon_2.h>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------
void visualize_wall_space(const HoledPolygon& wall_space, Scene& scene) {
    PolygonSceneOptions hole_options; 
    hole_options.colored_vertex = [](const Polygon&, Polygon::Vertex_const_iterator) { return false; };
    hole_options.colored_edge = [](const Polygon&, Polygon::Vertex_const_iterator) { return true; };
    hole_options.edge_color = [](const Polygon&, Polygon::Vertex_const_iterator) { return Color(0, 0, 0); };
    hole_options.colored_face = [](const Polygon&, void*) { return true; };
    hole_options.face_color = [](const Polygon&, void*) { return Color(0, 0, 0); };

    for (auto hole : wall_space.holes()) {
        hole.reverse_orientation();
        CGAL::add_to_graphics_scene(hole, scene, hole_options);
    }

    PolygonSceneOptions wall_boundary_options;
    wall_boundary_options.colored_vertex = [](const Polygon&, Polygon::Vertex_const_iterator) { return false; };
    wall_boundary_options.colored_edge = [](const Polygon&, Polygon::Vertex_const_iterator) { return true; };
    wall_boundary_options.edge_color = [](const Polygon&, Polygon::Vertex_const_iterator) { return Color(0, 0, 0); };
    wall_boundary_options.colored_face = [](const Polygon&, void*) { return true; };
    wall_boundary_options.face_color = [](const Polygon&, void*) { return Color(255, 255, 255); };

    CGAL::add_to_graphics_scene(wall_space.outer_boundary(), scene, wall_boundary_options);
}

void visualize(const HoledPolygon& wall_space, const Graph& graph) {
    Scene scene;
    visualize_wall_space(wall_space, scene);

    CGAL::draw_graphics_scene(scene);
}
