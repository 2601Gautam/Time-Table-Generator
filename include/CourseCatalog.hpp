#pragma once
#include <map>
#include <string>
#include <vector>
#include "Course.hpp"

// A course code taught to one or more program/semester combinations, e.g.
// "IC101" taught to both ICTA-1 and ICTB-1 by the same or different faculty.
// All of those rows are scheduled together, as one shared lecture.
struct CourseGroup {
    int lecturesPerWeek = 0;
    std::vector<Course> sections; // one entry per program/semester offering
    bool placed = false;          // true once assigned to a Slot
};

// Groups the flat course list back together by course code. This replaces
// the original hand-rolled open-chaining hash table (`class Hash` + a
// `list<Course>` chain, built by a manual dedup-by-linear-scan pass). A
// std::map<string, CourseGroup> does the same grouping in one pass and, as a
// bonus, iterates in a fixed (alphabetical) order, so a given input always
// packs into slots the same way -- useful when you need to trust that
// re-running the generator on the same file reproduces the same timetable.
class CourseCatalog {
public:
    explicit CourseCatalog(const std::vector<Course>& courses) {
        for (const auto& c : courses) {
            if (!c.isCore()) continue; // only Core courses are auto-scheduled
            auto& group = groups_[c.code()];
            group.lecturesPerWeek = c.lecturesPerWeek();
            group.sections.push_back(c);
        }
    }

    std::map<std::string, CourseGroup>& groups() noexcept { return groups_; }
    const std::map<std::string, CourseGroup>& groups() const noexcept { return groups_; }

private:
    std::map<std::string, CourseGroup> groups_;
};
