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

#include "cgal_types.hpp"

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
                                    "psetstring TEXT NOT NULL"
                                    "pcount INTEGER NOT NULL CHECK (pcount > 0)"
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
    std::string insert_polygon_set = "INSERT INTO coverage_areas (pssetstring, pcount) VALUES (?, ?);";
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
    std::string select_polygon_set = "SELECT psetstring, pcount FROM coverage_areas WHERE id = ?;";
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

sqlite3_int64 Cache::insert_coverage_area(const PolygonSet& polygon_set) {
    std::ostringstream pset_string;
    pset_string << std::setprecision(HP_PRECISION);
    
    // Convert the PolygonSet to a string representation
    std::vector<CurvedPolygon> polygons_with_holes;
    polygons_with_holes.reserve(polygon_set.number_of_polygons_with_holes());
    polygon_set.polygons_with_holes(std::back_inserter(polygons_with_holes));
    for (const auto& [i, p] : std::views::zip(std::views::iota(0), polygons_with_holes)) {
        pset_string << p;
        if (i != polygons_with_holes.size() - 1) pset_string << '\n';
    }
    
    // Bind the string representation and polygon count to the insert statement
    int bind_result = sqlite3_bind_text(
        this->insert_statement,
        1,
        pset_string.str().c_str(),
        -1,
        SQLITE_TRANSIENT
    );
    this->handle_sqlite_error(
        bind_result, 
        "Failed to bind polygon set string"
    );
    bind_result = sqlite3_bind_int(
        this->insert_statement,
        2,
        static_cast<int>(polygons_with_holes.size())
    );
    this->handle_sqlite_error(bind_result, "Failed to bind polygon count");
    
    // Execute the insert statement
    int step_result = sqlite3_step(this->insert_statement);
    this->handle_sqlite_error(
        "Failed to execute insert statement", 
        [step_result]() { return step_result != SQLITE_DONE; }
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
    this->handle_sqlite_error(step_result, "Failed to execute select statement");
    const char* pset_string = reinterpret_cast<const char*>(sqlite3_column_text(this->select_statement, 1));
    this->handle_sqlite_error(
        "Failed to retrieve polygon set string", 
        [pset_string]() { return pset_string == nullptr; }
    );
    
    int pcount = sqlite3_column_int(this->select_statement, 2);
    this->handle_sqlite_error(
        "Failed to retrieve polygon count", 
        [pcount]() { return pcount <= 0; }
    );
    
    // Process results
    std::istringstream pset_stream(pset_string);
    std::vector<CurvedPolygon> polygons_with_holes;
    polygons_with_holes.reserve(pcount);
    std::string line;
    while (std::getline(pset_stream, line)) {
        std::istringstream line_stream(line);
        CurvedPolygon polygon;
        this->handle_sqlite_error(
            "Failed to parse polygon from string: " + line, 
            [&line_stream, &polygon]() { return !(line_stream >> polygon); }
        );
        
        polygons_with_holes.push_back(polygon); 
    }

    PolygonSet result;
    result.join(polygons_with_holes.begin(), polygons_with_holes.end());

    // Step through the select statement again to allow for future reuse
    step_result = sqlite3_step(this->select_statement);
    this->handle_sqlite_error(
        "Failed to step through select statement", 
        [step_result]() { return step_result != SQLITE_DONE; }
    );

    return result;
}

Cache::~Cache() {
    this->teardown();
}
