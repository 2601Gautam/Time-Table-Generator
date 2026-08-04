#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>

// Small, dependency-free helpers for the simple (unquoted) CSV files this
// project reads and writes. Good enough for the course-list format described
// in the README; not a general-purpose RFC-4180 parser.
namespace csv {

inline std::string trim(const std::string& s) {
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

// Splits a single CSV line on commas and trims whitespace/'\r' from each field.
inline std::vector<std::string> splitLine(const std::string& rawLine) {
    std::string line = rawLine;
    if (!line.empty() && line.back() == '\r') line.pop_back(); // tolerate CRLF input

    std::vector<std::string> fields;
    std::string field;
    for (char c : line) {
        if (c == ',') {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    fields.push_back(trim(field));
    return fields;
}

// The "Lecture" column is stored as an L-T-P-C string, e.g. "3-0-2-4".
// Only the leading L (lectures/week) value is meaningful to the scheduler.
inline int parseLeadingInt(const std::string& field) {
    size_t i = 0;
    while (i < field.size() && std::isspace(static_cast<unsigned char>(field[i]))) ++i;
    size_t start = i;
    while (i < field.size() && std::isdigit(static_cast<unsigned char>(field[i]))) ++i;
    if (i == start) {
        throw std::runtime_error("expected a leading number in field \"" + field + "\"");
    }
    return std::stoi(field.substr(start, i - start));
}

} // namespace csv
