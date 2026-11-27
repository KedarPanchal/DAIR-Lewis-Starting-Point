#include "cache.hpp"

#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

#include <boost/container/small_vector.hpp>

#include <CGAL/Circle_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/enum.h>

#include "cgal_types.hpp"
#include "constructions.hpp"

// -- LIFETIME ----------------------------------------------------------------

Cache::Cache(const std::string& name, const HoledPolygon& wall_space) {
    // Open the SQLite database
    int open_result = sqlite3_open_v2(
        name.c_str(),
        &(this->db),
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
        nullptr);
    this->handle_sqlite_error(open_result, "Failed to open database");

    this->setup_table();
    this->prepare_statements();

    this->wall_space = convert_polygon_with_holes<EHoledPolygon>(wall_space);
}

Cache::~Cache() {
    this->teardown();
}

// -- DATABASE HELPERS --------------------------------------------------------

void Cache::teardown() {
    int finalize_insert_result = sqlite3_finalize(this->insert_statement);
    if (finalize_insert_result != SQLITE_OK) {
        std::cerr
            << "Failed to finalize insert statement: "
            << sqlite3_errmsg(this->db)
            << '\n';
    }
    this->insert_statement = nullptr;

    int finalize_select_result = sqlite3_finalize(this->select_statement);
    if (finalize_select_result != SQLITE_OK) {
        std::cerr
            << "Failed to finalize select statement: "
            << sqlite3_errmsg(this->db)
            << '\n';
    }
    this->select_statement = nullptr;

    int close_result = sqlite3_close(this->db);
    if (close_result != SQLITE_OK) {
        std::cerr
            << "Failed to close database: "
            << sqlite3_errmsg(this->db)
            << '\n';
    }
    this->db = nullptr;
}

void Cache::handle_sqlite_error(
    int result_code,
    const std::string& error_message) {
    if (result_code != SQLITE_OK) {
        std::string error_msg =
            error_message + ": " + std::string(sqlite3_errmsg(this->db));
        this->teardown();
        throw std::runtime_error(error_msg);
    }
}

void Cache::handle_sqlite_error(
        const std::string& error_message,
        std::function<bool()> condition) {
    if (condition()) {
        std::string error_msg =
            error_message + ": " + std::string(sqlite3_errmsg(this->db));
        this->teardown();
        throw std::runtime_error(error_msg);
    }
}

void Cache::setup_table() {
    // Create the coverage_areas table if it doesn't exist
    sqlite3_stmt* drop_table_statement;
    std::string drop_table = "DROP TABLE IF EXISTS coverage_areas;";
    int drop_table_result = sqlite3_prepare_v2(
        this->db,
        drop_table.c_str(),
        -1,
        &drop_table_statement,
        nullptr);
    this->handle_sqlite_error(
        drop_table_result,
        "Failed to prepare drop table statement");

    int step_result = sqlite3_step(drop_table_statement);
    this->handle_sqlite_error(
        "Failed to drop table",
        [step_result]() { return step_result != SQLITE_DONE; });

    int finalize_result = sqlite3_finalize(drop_table_statement);
    this->handle_sqlite_error(
        finalize_result,
        "Failed to finalize drop table statement");

    sqlite3_stmt* create_table_statement;
    std::string create_table =
        "CREATE TABLE IF NOT EXISTS coverage_areas ("
        "id INTEGER PRIMARY KEY,"
        "source_segment TEXT NOT NULL,"
        "target_segment TEXT NOT NULL,"
        "radius TEXT NOT NULL"
        ");";
    int create_table_result = sqlite3_prepare_v2(
        this->db,
        create_table.c_str(),
        -1,
        &create_table_statement,
        nullptr);
    this->handle_sqlite_error(
        create_table_result,
        "Failed to prepare create table statement");

    step_result = sqlite3_step(create_table_statement);
    this->handle_sqlite_error(
        "Failed to create table",
        [step_result]() { return step_result != SQLITE_DONE; });

    finalize_result = sqlite3_finalize(create_table_statement);
    this->handle_sqlite_error(
        finalize_result,
        "Failed to finalize create table statement");
}

void Cache::prepare_statements() {
    // Prepare the insert statement
    std::string insert_polygon_set =
        "INSERT INTO coverage_areas "
        "(source_segment, target_segment, radius) "
        "VALUES (?, ?, ?);";
    int insert_result = sqlite3_prepare_v2(
        this->db,
        insert_polygon_set.c_str(),
        -1,
        &this->insert_statement,
        nullptr);
    this->handle_sqlite_error(
        insert_result,
        "Failed to prepare insert statement");

    // Prepare the select statement
    std::string select_polygon_set =
        "SELECT source_segment, target_segment, radius "
        "FROM coverage_areas WHERE id = ?;";
    int select_result = sqlite3_prepare_v2(
        this->db,
        select_polygon_set.c_str(),
        -1,
        &this->select_statement,
        nullptr);
    this->handle_sqlite_error(
        select_result,
        "Failed to prepare select statement");
}

// -- SEGMENT SERIALIZATION ---------------------------------------------------

void Cache::prepare_segment(std::ostringstream& stream, const Segment& seg) {
    stream << std::setprecision(HP_PRECISION);
    hpscalar x1 = convert<hpscalar>(seg.source().x());
    hpscalar y1 = convert<hpscalar>(seg.source().y());
    hpscalar x2 = convert<hpscalar>(seg.target().x());
    hpscalar y2 = convert<hpscalar>(seg.target().y());
    stream << x1 << " " << y1 << " " << x2 << " " << y2;
}

Segment Cache::parse_segment(const char* segment_string) {
    std::istringstream stream(segment_string);
    hpscalar x1, y1, x2, y2;
    stream >> std::setprecision(HP_PRECISION) >> x1 >> y1 >> x2 >> y2;
    Point source{convert<fscalar>(x1), convert<fscalar>(y1)};
    Point target{convert<fscalar>(x2), convert<fscalar>(y2)};
    return Segment{source, target};
}

// -- CACHE API ---------------------------------------------------------------

sqlite3_int64 Cache::insert_coverage_area(
    const Segment& source,
    const Segment& target,
    fscalar radius) {
    std::ostringstream source_string;
    std::ostringstream target_string;
    std::ostringstream radius_string;
    this->prepare_segment(source_string, source);
    this->prepare_segment(target_string, target);

    hpscalar radius_hp = convert<hpscalar>(radius);
    radius_string << std::setprecision(HP_PRECISION) << radius;

    // Bind the string representation and polygon count to the insert
    // statement
    int bind_result = sqlite3_bind_text(
        this->insert_statement,
        1,
        source_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT);
    this->handle_sqlite_error(
        bind_result,
        "Failed to bind source string");

    bind_result = sqlite3_bind_text(
        this->insert_statement,
        2,
        target_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT);
    this->handle_sqlite_error(
        bind_result,
        "Failed to bind target string");

    bind_result = sqlite3_bind_text(
        this->insert_statement,
        3,
        radius_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT);
    this->handle_sqlite_error(bind_result, "Failed to bind radius");

    // Execute the insert statement
    int step_result = sqlite3_step(this->insert_statement);
    this->handle_sqlite_error(
        "Failed to execute insert statement",
        [step_result]() { return step_result != SQLITE_DONE; });

    // Reset the insert statement to allow future reuse
    int reset_result = sqlite3_reset(this->insert_statement);
    this->handle_sqlite_error(
        reset_result,
        "Failed to reset insert statement");

    return sqlite3_last_insert_rowid(this->db);
}

PolygonSet Cache::get_coverage_area(size_t id) {
    // Prepare select statement
    int bind_result = sqlite3_bind_int(
        this->select_statement,
        1,
        static_cast<int>(id));
    this->handle_sqlite_error(
        bind_result,
        "Failed to bind id for select statement");

    // Execute the select statement
    // There should only be one row returned, since id is a primary key
    int step_result = sqlite3_step(this->select_statement);
    this->handle_sqlite_error(
        "Failed to execute select statement",
        [step_result]() { return step_result != SQLITE_ROW; });

    const char* source_string = reinterpret_cast<const char*>(
        sqlite3_column_text(this->select_statement, 0));
    this->handle_sqlite_error(
        "Failed to retrieve source segment string",
        [source_string]() { return source_string == nullptr; });

    const char* target_string = reinterpret_cast<const char*>(
        sqlite3_column_text(this->select_statement, 1));
    this->handle_sqlite_error(
        "Failed to retrieve target segment string",
        [target_string]() { return target_string == nullptr; });

    const char* radius_string = reinterpret_cast<const char*>(
        sqlite3_column_text(this->select_statement, 2));
    this->handle_sqlite_error(
        "Failed to retrieve radius string",
        [radius_string]() { return radius_string == nullptr; });

    // Process results
    Segment source = this->parse_segment(source_string);
    Segment target = this->parse_segment(target_string);
    hpscalar radius_hp;
    std::istringstream radius_stream(radius_string);
    radius_stream >> std::setprecision(HP_PRECISION) >> radius_hp;
    fscalar radius = convert<fscalar>(radius_hp);
    PolygonSet result = compute_coverage(source, target, radius, this->wall_space);

    // Step through and reset select statement again to allow future reuse
    step_result = sqlite3_step(this->select_statement);
    this->handle_sqlite_error(
        "Failed to step through select statement",
        [step_result]() { return step_result != SQLITE_DONE; });

    int reset_result = sqlite3_reset(this->select_statement);
    this->handle_sqlite_error(
        reset_result,
        "Failed to reset select statement");

    return result;
}
