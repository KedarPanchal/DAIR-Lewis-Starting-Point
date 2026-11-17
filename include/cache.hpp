#ifndef CACHE_HPP
#define CACHE_HPP

#include <functional>
#include <sstream>
#include <string>

#include <sqlite3.h>

#include <boost/container/small_vector.hpp>

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
    void handle_sqlite_error(
        int result_code,
        const std::string& error_message);
    void handle_sqlite_error(
        const std::string& error_message,
        std::function<bool()> condition);
    void setup_table();
    void prepare_statements();
    void prepare_segment(std::ostringstream& stream, const Segment& seg);
    Segment parse_segment(const char* segment_string);
    boost::container::small_vector<Curve, 4> build_standard_stadium(
        const Point& source_center,
        const Point& target_center,
        const fscalar& radius,
        const Vector& direction);
    boost::container::small_vector<Curve, 4> build_circular_stadium(
        const Point& center,
        const fscalar& radius);
    CurvedPolygon build_stadium(
        const Point& source,
        const Point& target,
        const fscalar& radius);
    PolygonSet compute_coverage(
        const Segment& source,
        const Segment& target,
        const fscalar& radius);

public:
    Cache(const std::string& name);
    Cache(const Cache&) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache& operator=(Cache&&) = delete;
    ~Cache();

    sqlite3_int64 insert_coverage_area(
        const Segment& source,
        const Segment& target,
        fscalar radius);
    PolygonSet get_coverage_area(size_t id);
};

#endif
