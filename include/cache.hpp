#ifndef CACHE_HPP
#define CACHE_HPP

#include <string>
#include <functional>

#include <sqlite3.h>

#include "cgal_types.hpp"

class Cache {
private:
    struct done_flag {
        explicit done_flag() = default;
    };
    inline constexpr static done_flag done{};

    sqlite3* db;
    sqlite3_stmt* insert_statement;
    sqlite3_stmt* select_statement;
    
    void teardown();
    void handle_sqlite_error(int result_code, const std::string& error_message);
    void handle_sqlite_error(const std::string& error_message, std::function<bool()> condition);
    void setup_table();
    void prepare_statements();

public:
    Cache(const std::string& name);
    Cache(const Cache&) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache& operator=(Cache&&) = delete;
    ~Cache();

    sqlite3_int64 insert_coverage_area(const PolygonSet& polygon_set);
    PolygonSet get_coverage_area(size_t id);
};

#endif
