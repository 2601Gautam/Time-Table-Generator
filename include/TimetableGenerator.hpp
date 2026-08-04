#pragma once
#include <string>
#include <vector>
#include "Course.hpp"
#include "CourseCatalog.hpp"
#include "Slot.hpp"

// tt[period][day], 1-indexed (row/col 0 unused) to match the printed
// "period 1..5 x Monday..Friday" grid. Holds slot numbers, or FREE_SLOT.
using Grid = std::vector<std::vector<int>>;

// How many slots of each weekly-lecture-count are needed, i.e. how many
// distinct 1-lecture / 2-lecture / 3-lecture time slots must exist so that
// the busiest single program/semester still gets every one of its Core
// courses scheduled without two of its own classes landing in the same slot.
struct SlotRequirement {
    int forLecture1 = 0;
    int forLecture2 = 0;
    int forLecture3 = 0;
    int total() const noexcept { return forLecture1 + forLecture2 + forLecture3; }
};

// Scans every Core course once and, per (program, semester), counts how many
// 1/2/3-lecture courses it has; the requirement is the worst case across all
// of them. Replaces the original `no_of_slots`, which hardcoded exactly the
// four program names {ICTA, CS, MNC, EVD} and silently skipped ICTB (a real
// bug -- if ICTB ever had the heaviest load for some lecture count, its
// courses would run out of slots to be placed into and drop out of the
// output). This version discovers programs and semesters from the data
// itself, so it's correct for any input, not just the sample CSVs.
SlotRequirement computeRequiredSlots(const std::vector<Course>& courses);

// Creates `requirement.total()` empty slots, 3-lecture ones first, matching
// the original's slot-numbering order (M1..Mn).
std::vector<Slot> buildSlots(const SlotRequirement& requirement);

// Greedily packs each course group into the first compatible slot with
// matching weekly-lecture-count that doesn't already teach that
// program/semester or clash with a faculty member already in the slot.
// Returns the course codes that could not be placed (should be empty when
// computeRequiredSlots sized things correctly; surfaced so it's never a
// silent data loss the way the original was).
std::vector<std::string> assignCoursesToSlots(CourseCatalog& catalog, std::vector<Slot>& slots);

// Places every slot onto the weekly grid, highest remaining weekly-frequency
// first, so a 3-lecture course is guaranteed 3 placements across the week,
// never twice on the same day, and never back-to-back with another slot that
// shares a faculty member. Throws std::runtime_error if the total weekly
// teaching load exceeds the number of periods available in the grid.
Grid buildWeeklyGrid(const std::vector<Slot>& slots);

// Re-checks every constraint the grid is supposed to satisfy and returns a
// human-readable description of each violation found (empty = grid is
// clash-free). Cheap to run and a good regression check after any change to
// buildWeeklyGrid.
std::vector<std::string> validateGrid(const Grid& grid, const std::vector<Slot>& slots);

// --- Excel export ---------------------------------------------------------

void writeSlotBreakdownXlsx(const std::string& path, const std::vector<Slot>& slots);
void writeMasterGridXlsx(const std::string& path, const Grid& grid);
void writeProgramGridXlsx(const std::string& path, const Grid& grid, const std::vector<Slot>& slots,
                          const std::string& program, int semester);
void writeFacultyGridXlsx(const std::string& path, const Grid& grid, const std::vector<Slot>& slots,
                          const std::string& faculty);
