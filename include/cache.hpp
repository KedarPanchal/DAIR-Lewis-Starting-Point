#ifndef CACHE_HPP
#define CACHE_HPP

#include <string>

#include <sqlite3.h>

#include "cgal_types.hpp"

class Cache {
private:
    sqlite3* db;
    sqlite3_stmt* insert_statement;
    sqlite3_stmt* select_statement;

public:
    Cache(const std::string& name);
    Cache(const Cache&) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache& operator=(Cache&&) = delete;
    ~Cache();

    void insert_coverage_area(const PolygonSet& polygon_set);
    PolygonSet get_coverage_area(size_t id);
};

#endif
