#pragma once

// Weekly grid shape. Originally hardcoded as a literal `tt[6][6]` (1-indexed,
// so effectively 5x5) scattered across several functions; named here so the
// grid size is a single, obvious place to change (the original README lists
// "configurable number of working days/periods" as a future improvement).
constexpr int NUM_DAYS = 5;      // Monday..Friday
constexpr int NUM_PERIODS = 5;   // periods per day
constexpr int START_HOUR = 8;    // first period starts at 8:00

// Slot number 0 is reserved to mean "nothing scheduled" / Free period.
constexpr int FREE_SLOT = 0;
