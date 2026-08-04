#pragma once
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <utility>

// One row taught inside a Slot: a course, resolved to the specific
// program/semester/faculty combination it's taught to.
struct Assignment {
    std::string courseCode;
    std::string program;
    int semester = 0;
    std::string faculty;
    std::string type;
};

// A Slot ("M1", "M2", ...) is a group of courses that always meet at the same
// time. Several courses share a slot when no program/semester or faculty
// member needs to be in two places at once -- that's what keeps the eventual
// weekly grid clash-free. Originally this used five fixed-size parallel
// arrays (code[100], pg[100], sem[100], ...) plus one bool[8] per hardcoded
// program name (ICTA/ICTB/CS/MNC/EVD) to track what's already in the slot.
// A vector<Assignment> plus two small STL sets do the same job, without the
// 100-row cap and without assuming which program names exist.
class Slot {
public:
    Slot(int slotNumber, int lecturesPerWeek)
        : slotNumber_(slotNumber), lecturesPerWeek_(lecturesPerWeek) {}

    int slotNumber() const noexcept { return slotNumber_; }
    int lecturesPerWeek() const noexcept { return lecturesPerWeek_; }
    const std::vector<Assignment>& assignments() const noexcept { return assignments_; }

    bool hasProgramSemester(const std::string& program, int semester) const {
        return programSemesterUsed_.count({program, semester}) != 0;
    }
    bool hasFaculty(const std::string& faculty) const {
        return facultyUsed_.count(faculty) != 0;
    }

    void addAssignment(Assignment a) {
        programSemesterUsed_.insert({a.program, a.semester});
        facultyUsed_.insert(a.faculty);
        assignments_.push_back(std::move(a));
    }

    // Resolves this slot to the single course a given program/semester sees
    // in it (or nullptr if that program/semester isn't taught here).
    const Assignment* forProgramSemester(const std::string& program, int semester) const {
        for (const auto& a : assignments_) {
            if (a.program == program && a.semester == semester) return &a;
        }
        for (const auto& a : assignments_) {
            if (a.semester == semester && a.program.rfind(program, 0) == 0) return &a;
        }
        return nullptr;
    }

    // Resolves this slot to the single course a given faculty member teaches
    // in it (or nullptr if they don't teach in this slot).
    const Assignment* forFaculty(const std::string& faculty) const {
        for (const auto& a : assignments_) {
            if (a.faculty == faculty) return &a;
        }
        return nullptr;
    }

private:
    int slotNumber_;
    int lecturesPerWeek_;
    std::vector<Assignment> assignments_;
    std::set<std::pair<std::string, int>> programSemesterUsed_;
    std::unordered_set<std::string> facultyUsed_;
};

// True if the two slots share at least one faculty member -- used when
// placing slots on the weekly grid so nobody is booked back-to-back.
inline bool shareFaculty(const Slot& a, const Slot& b) {
    for (const auto& assignment : a.assignments()) {
        if (b.hasFaculty(assignment.faculty)) return true;
    }
    return false;
}
