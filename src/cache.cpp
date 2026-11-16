#include "cache.hpp"

#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ranges>
#include <functional>

#include <sqlite3.h>

#include <boost/container/small_vector.hpp>

#include <CGAL/Circle_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/enum.h>

#include "cgal_types.hpp"
#include "utilities.hpp"

void Cache::teardown() {
    int finalize_insert_result = sqlite3_finalize(this->insert_statement);
    if (finalize_insert_result != SQLITE_OK) {
        std::cerr 
            << "Failed to finalize insert statement: " 
            << sqlite3_errmsg(this->db) 
            << std::endl;
    }

    int finalize_select_result = sqlite3_finalize(this->select_statement);
    if (finalize_select_result != SQLITE_OK) {
        std::cerr 
            << "Failed to finalize select statement: " 
            << sqlite3_errmsg(this->db) 
            << std::endl;
    }

    int close_result = sqlite3_close(this->db);
    if (close_result != SQLITE_OK) {
        std::cerr 
            << "Failed to close database: " 
            << sqlite3_errmsg(this->db) 
            << std::endl;
    }
}

void Cache::handle_sqlite_error(
        int result_code, 
        const std::string& error_message
        ) {
    if (result_code != SQLITE_OK) {
        std::string error_msg = error_message 
            + ": " 
            + std::string(sqlite3_errmsg(this->db));
        this->teardown();
        throw std::runtime_error(error_msg);
    }
}

void Cache::handle_sqlite_error(
        const std::string& error_message, 
        std::function<bool()> condition
        ) {
    if (condition()) {
        std::string error_msg = error_message 
            + ": " 
            + std::string(sqlite3_errmsg(this->db));
        this->teardown();
        throw std::runtime_error(error_msg);
    }
}

void Cache::setup_table() {
    // Create the coverage_areas table if it doesn't exist
    sqlite3_stmt* create_table_statement;
    std::string create_table = "CREATE TABLE IF NOT EXISTS coverage_areas ("
                                    "id INTEGER PRIMARY KEY,"
                                    "source_segment TEXT NOT NULL,"
                                    "target_segment TEXT NOT NULL,"
                                    "radius TEXT NOT NULL,"
                                ");";
    int create_table_result = sqlite3_prepare_v2(
        this->db, 
        create_table.c_str(), 
        -1, 
        &create_table_statement, 
        nullptr
    );
    this->handle_sqlite_error(
        create_table_result, 
        "Failed to prepare create table statement"
    );
    int step_result = sqlite3_step(create_table_statement);
    this->handle_sqlite_error(step_result, "Failed to create table");
    int finalize_result = sqlite3_finalize(create_table_statement);
    this->handle_sqlite_error(
        finalize_result, 
        "Failed to finalize create table statement"
    );
}

void Cache::prepare_statements() {
    // Prepare the insert statement
    std::string insert_polygon_set = "INSERT INTO coverage_areas (source_segment, target_segment, radius) "
        "VALUES (?, ?, ?);";
    int insert_result = sqlite3_prepare_v2(
        this->db,
        insert_polygon_set.c_str(),
        -1,
        &this->insert_statement,
        nullptr
    );
    this->handle_sqlite_error(
        insert_result, 
        "Failed to prepare insert statement"
    );
    
    // Prepare the select statement
    std::string select_polygon_set = "SELECT source_segment, target_segment, radius FROM coverage_areas WHERE id = ?;";
    int select_result = sqlite3_prepare_v2(
        this->db,
        select_polygon_set.c_str(),
        -1,
        &this->select_statement,
        nullptr
    );
    this->handle_sqlite_error(
        select_result, 
        "Failed to prepare select statement"
    );
}

void Cache::prepare_segment(std::ostringstream& stream, const Segment& seg) {
    stream << std::setprecision(HP_PRECISION);
    hpscalar x1 = convert<hpscalar>(seg.source().x());
    hpscalar y1 = convert<hpscalar>(seg.source().y());
    hpscalar x2 = convert<hpscalar>(seg.target().x());
    hpscalar y2 = convert<hpscalar>(seg.target().y());
    stream << x1 << " " << y1 << " " << x2 << " " << y2;
}

Cache::Cache(const std::string& name) {
    // Open the SQLite database
    int open_result = sqlite3_open_v2(
        name.c_str(), 
        &(this->db), 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
        nullptr
    );
    this->handle_sqlite_error(open_result, "Failed to open database");

    this->setup_table();
    this->prepare_statements();    
}

Segment Cache::parse_segment(const char* segment_string) {
    std::istringstream stream(segment_string);
    hpscalar x1, y1, x2, y2;
    stream >> std::setprecision(HP_PRECISION) >> x1 >> y1 >> x2 >> y2;
    Point source{convert<fscalar>(x1), convert<fscalar>(y1)};
    Point target{convert<fscalar>(x2), convert<fscalar>(y2)};
    return Segment{source, target};
}

// Code to build the edges of a stadium polygon
boost::container::small_vector<Curve, 4> Cache::build_standard_stadium(
        const Point& source_center, 
        const Point& target_center, 
        const fscalar& radius, 
        const Vector& direction
        ) {
    CGAL::Circle_2<Kernel> source_circle(source_center, radius * radius, CGAL::COUNTERCLOCKWISE);
    CGAL::Circle_2<Kernel> target_circle(target_center, radius * radius, CGAL::COUNTERCLOCKWISE);

    // Construct the 4 points needed to construct the rectangle
    Point source1 = source_center + direction.perpendicular(CGAL::CLOCKWISE);
    Point source2 = source_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    Point target1 = target_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    Point target2 = target_center + direction.perpendicular(CGAL::CLOCKWISE);
    std::cout << "Stadium points: " << source1 << ", " << source2 << ", " << target1 << ", " << target2 << std::endl;
    // Place them all into a vector and sort them in counterclockwise order
    boost::container::small_vector<Point, 4> rectangle_points{source1, source2, target1, target2};
    boost::container::small_vector<Point, 4> sorted_rectangle_points;
    CGAL::convex_hull_2(rectangle_points.begin(), rectangle_points.end(), std::back_inserter(sorted_rectangle_points));

    // If both points are a source or target, construct an arc in counterclockwise order, otherwise construct a line segment
    boost::container::small_vector<Curve, 4> curves;
    for (size_t i = 0; i < 4; ++i) {
        size_t next = (i + 1)  % 4;
        if (
            (sorted_rectangle_points[i] == source1 || sorted_rectangle_points[i] == source2) &&
            (sorted_rectangle_points[next] == source1 || sorted_rectangle_points[next] == source2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            curves.emplace_back(source_circle, source_point, target_point);
        } else if (
            (sorted_rectangle_points[i] == target1 || sorted_rectangle_points[i] == target2) &&
            (sorted_rectangle_points[next] == target1 || sorted_rectangle_points[next] == target2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            curves.emplace_back(target_circle, source_point, target_point);
        } else {
            curves.emplace_back(sorted_rectangle_points[i], sorted_rectangle_points[next]);
        }
    }

    return curves;
}

boost::container::small_vector<Curve, 4> Cache::build_circular_stadium(
        const Point& center, 
        const fscalar& radius
        ) {
    CGAL::Circle_2<Kernel> circle(center, radius * radius, CGAL::COUNTERCLOCKWISE);
    return {Curve(circle)};
}

CurvedPolygon Cache::build_stadium(
        const Point& source, 
        const Point& target, 
        const fscalar& radius
        ) {
    std::cout << "Source: " << source << ", Target: " << target << ", Radius: " << radius << std::endl;
    // The circle centers are radius distance along the vector from source to target, in both directions
    Vector direction = Vector{source, target};
    direction = direction / CGAL::sqrt(direction.squared_length()) * radius;
    Point source_center = source + direction;
    Point target_center = target - direction;

    // If source and target centers are the same, then the stadium is just a circle
    boost::container::small_vector<Curve, 4> curves = source_center == target_center 
        ? this->build_circular_stadium(source_center, radius) 
        : this->build_standard_stadium(source_center, target_center, radius, direction);
    
    CGAL::Arrangement_2<CurvedTraits> arr;
    CGAL::insert(arr, curves.begin(), curves.end());
    
    // Find first bounded face in the arrangement and extract its boundary edges to construct a CurvedPolygon
    CGAL::Arrangement_2<CurvedTraits>::Face_const_iterator face = arr.unbounded_face();
    for (face = arr.faces_begin(); face != arr.faces_end(); ++face) {
        if (!face->is_unbounded()) break;
    }
    boost::container::small_vector<CurvedTraits::X_monotone_curve_2, 4> stadium_edges;
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator circ = face->outer_ccb();
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator start = circ;
    do {
        stadium_edges.push_back(circ->curve());
    } while (++circ != start);

    return CurvedPolygon(stadium_edges.begin(), stadium_edges.end());
}

PolygonSet Cache::compute_coverage(
        const Segment& source, 
        const Segment& target, 
        const fscalar& radius
        ) {
    auto stadium1 = this->build_stadium(source.source(), target.target(), radius);
    auto stadium2 = this->build_stadium(source.target(), target.source(), radius);
    
    PolygonSet coverage{std::move(stadium1)};
    coverage.intersection(std::move(stadium2));
    return coverage;
}

sqlite3_int64 Cache::insert_coverage_area(
        const Segment& source, 
        const Segment& target, 
        fscalar radius
        ) {
    std::ostringstream source_string;
    std::ostringstream target_string;
    std::ostringstream radius_string;
    this->prepare_segment(source_string, source);
    this->prepare_segment(target_string, target);

    hpscalar radius_hp = convert<hpscalar>(radius);
    radius_string << std::setprecision(HP_PRECISION) << radius;
    
    // Bind the string representation and polygon count to the insert statement
    int bind_result = sqlite3_bind_text(
        this->insert_statement,
        1,
        source_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT
    );
    this->handle_sqlite_error(
        bind_result, 
        "Failed to bind source string"
    );
    bind_result = sqlite3_bind_text(
        this->insert_statement,
        2,
        target_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT
    );
    this->handle_sqlite_error(
        bind_result, 
        "Failed to bind target string"
    );
    bind_result = sqlite3_bind_text(
        this->insert_statement,
        3,
        radius_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT
    );
    this->handle_sqlite_error(bind_result, "Failed to bind radius");
    
    // Execute the insert statement
    int step_result = sqlite3_step(this->insert_statement);
    this->handle_sqlite_error(
        "Failed to execute insert statement", 
        [step_result]() { return step_result != SQLITE_DONE; }
    );

    // Reset the insert statement to allow future reuse
    int reset_result = sqlite3_reset(this->insert_statement);
    this->handle_sqlite_error(
        reset_result, 
        "Failed to reset insert statement"
    );

    return sqlite3_last_insert_rowid(this->db);
}

PolygonSet Cache::get_coverage_area(size_t id) {
    // Prepare select statement
    int bind_result = sqlite3_bind_int(
        this->select_statement, 
        1, 
        static_cast<int>(id)
    );
    this->handle_sqlite_error(
        bind_result, 
        "Failed to bind id for select statement"
    );
    
    // Execute the select statement
    // There should only be one row returned, since id is a primary key
    int step_result = sqlite3_step(this->select_statement);
    this->handle_sqlite_error(
        "Failed to execute select statement", 
        [step_result]() { return step_result != SQLITE_ROW; }
    );
    const char* source_string = reinterpret_cast<const char*>(sqlite3_column_text(this->select_statement, 1));
    this->handle_sqlite_error(
        "Failed to retrieve polygon set string", 
        [source_string]() { return source_string == nullptr; }
    );
    const char* target_string = reinterpret_cast<const char*>(sqlite3_column_text(this->select_statement, 2));
    this->handle_sqlite_error(
        "Failed to retrieve polygon set string", 
        [target_string]() { return target_string == nullptr; }
    );
    const char* radius_string = reinterpret_cast<const char*>(sqlite3_column_text(this->select_statement, 3));
    this->handle_sqlite_error(
        "Failed to retrieve polygon set string", 
        [radius_string]() { return radius_string == nullptr; }
    );
    
    // Process results
    Segment source = this->parse_segment(source_string);
    Segment target = this->parse_segment(target_string);
    hpscalar radius_hp;
    std::istringstream radius_stream(radius_string);
    radius_stream >> std::setprecision(HP_PRECISION) >> radius_hp;
    fscalar radius = convert<fscalar>(radius_hp);
    PolygonSet result = this->compute_coverage(source, target, radius);

    // Step through and reset select statement again to allow future reuse
    step_result = sqlite3_step(this->select_statement);
    this->handle_sqlite_error(
        "Failed to step through select statement", 
        [step_result]() { return step_result != SQLITE_DONE; }
    );
    int reset_result = sqlite3_reset(this->select_statement);
    this->handle_sqlite_error(
        reset_result, 
        "Failed to reset select statement"
    );
    return result;
}

Cache::~Cache() {
    this->teardown();
}
