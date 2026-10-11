#ifndef VISUALIZATION_HPP
#define VISUALIZATION_HPP

#include <CGAL/IO/Color.h>
#include <CGAL/Graphics_scene.h>
#include <CGAL/draw_polygon_2.h>
#include <CGAL/Graphics_scene_options.h>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

using Color = CGAL::IO::Color;
using Scene = CGAL::Graphics_scene;
using PolygonSceneOptions = CGAL::Graphics_scene_options<Polygon, Polygon::Vertex_const_iterator, Polygon::Vertex_const_iterator, void*>;

void visualize(const HoledPolygon& wall_space, const Graph& graph);

#endif
