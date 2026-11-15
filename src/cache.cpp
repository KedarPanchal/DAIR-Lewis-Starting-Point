#include "cache.hpp"

#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ranges>

#include <sqlite3.h>

#include "cgal_types.hpp"

void Cache::setup_table() {
    // Create the coverage_areas table if it doesn't exist
    sqlite3_stmt* create_table_statement;
    std::string create_table = "CREATE TABLE IF NOT EXISTS coverage_areas ("
                                    "id INTEGER PRIMARY KEY,"
                                    "psetstring TEXT NOT NULL"
                                    "pcount INTEGER NOT NULL"
                                ");";
    int create_table_result = sqlite3_prepare_v2(
        this->db, 
        create_table.c_str(), 
        -1, 
        &create_table_statement, 
        nullptr
    );
    if (create_table_result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare create table statement: " + std::string(sqlite3_errmsg(db)));
    }
    int step_result = sqlite3_step(create_table_statement);
    if (step_result != SQLITE_DONE) {
        throw std::runtime_error("Failed to create table: " + std::string(sqlite3_errmsg(db)));
    }
    int finalize_result = sqlite3_finalize(create_table_statement);
    if (finalize_result != SQLITE_OK) {
        throw std::runtime_error("Failed to finalize create table statement: " + std::string(sqlite3_errmsg(db)));
    }
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
    if (insert_result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare insert statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    // Prepare the select statement
    std::string select_polygon_set = "SELECT psetstring FROM coverage_areas WHERE id = ?;";
    int select_result = sqlite3_prepare_v2(
        this->db,
        select_polygon_set.c_str(),
        -1,
        &this->select_statement,
        nullptr
    );
    if (select_result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db)));
    }
}

Cache::Cache(const std::string& name) {
    // Open the SQLite database
    int open_result = sqlite3_open_v2(
        name.c_str(), 
        &(this->db), 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
        nullptr
    );
    if (open_result != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db)));
    }

    this->setup_table();
    this->prepare_statements();    
}

void Cache::insert_coverage_area(const PolygonSet& polygon_set) {
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
    if (bind_result != SQLITE_OK) {
        throw std::runtime_error("Failed to bind polygon set string: " + std::string(sqlite3_errmsg(db)));
    }
    bind_result = sqlite3_bind_int(
        this->insert_statement,
        2,
        static_cast<int>(polygons_with_holes.size())
    );
    if (bind_result != SQLITE_OK) {
        throw std::runtime_error("Failed to bind polygon count: " + std::string(sqlite3_errmsg(db)));
    }
    
    // Execute the insert statement
    int step_result = sqlite3_step(this->insert_statement);
    if (step_result != SQLITE_DONE) {
        throw std::runtime_error("Failed to execute insert statement: " + std::string(sqlite3_errmsg(db)));
    }
}

PolygonSet Cache::get_coverage_area(size_t id) {
    // TODO: Implement
}

Cache::~Cache() {
    int close_result = sqlite3_close(this->db);
    if (close_result != SQLITE_OK) {
        std::cerr << "Failed to close database: " << sqlite3_errmsg(db) << std::endl;
    }

    int finalize_insert_result = sqlite3_finalize(this->insert_statement);
    if (finalize_insert_result != SQLITE_OK) {
        std::cerr << "Failed to finalize insert statement: " << sqlite3_errmsg(db) << std::endl;
    }

    int finalize_select_result = sqlite3_finalize(this->select_statement);
    if (finalize_select_result != SQLITE_OK) {
        std::cerr << "Failed to finalize select statement: " << sqlite3_errmsg(db) << std::endl;
    }
}
