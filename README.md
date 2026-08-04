# Time-Table Generator

An academic timetable generator written in modern C++.

The project reads course data from CSV files, groups shared lecture offerings, computes the number of weekly slots required, assigns courses while avoiding clashes, places those slots into a weekly grid, validates the result, and exports the timetable as Excel workbooks.

## Overview

The generator supports these outputs:

- slot breakdown workbook
- master weekly timetable workbook
- program-wise semester workbook
- faculty-wise workbook

Input filenames can be entered without a path. If the file is not found in the current directory, the program automatically tries the `data/` directory.

## Scheduling Model

### Slot Assignment

- Only Core courses are auto-scheduled.
- A slot contains only courses with the same weekly lecture count.
- A course group is rejected for a slot if the same program/semester is already present or if one of its faculty members is already present.

### Weekly Placement

- Each slot is assigned a priority equal to its weekly lecture count.
- Slots are placed day by day in descending priority order.
- A slot is not repeated on the same day.
- The same faculty member is not scheduled in consecutive periods.
- If no valid placement exists for a cell, the cell is left free.

### Scope Limitations

- Core courses with a weekly lecture count of 0 are reported as out of scope for auto-scheduling.
- Non-Core courses are read from the input but are not auto-slotted.

## Implementation Changes

The current implementation keeps the scheduling rules from the original project but replaces the internal data structures with STL-based components.

| Original implementation | Current implementation | Reason |
|---|---|---|
| Manual linked list for priority handling | `PriorityScheduler` built on `std::multimap` | Deterministic ordering with simpler code |
| Manual open-chaining hash table for grouping | `CourseCatalog` built on `std::map` | One-pass grouping with stable ordering |
| Fixed-size per-slot arrays | `Slot` with `std::vector`, `std::set`, and `std::unordered_set` | Removes hardcoded size limits |
| Hardcoded program-name checks | Data-driven slot computation | Works with any program naming pattern |
| Raw C-style arrays and VLAs | `std::vector` | Standard C++ and safer memory handling |
| Silent invalid CSV rows | Explicit CSV validation with line numbers | Faster debugging and safer input handling |

## Project Structure

```
TimeTableGenerator/
├── CMakeLists.txt
├── Makefile
├── README.md
├── include/
│   ├── Course.hpp
│   ├── Csv.hpp
│   ├── CourseLoader.hpp
│   ├── CourseCatalog.hpp
│   ├── Slot.hpp
│   ├── PriorityScheduler.hpp
│   ├── Constants.hpp
│   └── TimetableGenerator.hpp
├── src/
│   ├── CourseLoader.cpp
│   ├── TimetableGenerator.cpp
│   └── main.cpp
└── data/
    ├── autumn.csv
    └── winter.csv
```

## Input Format

Input data is read from a CSV file with the following columns:

```text
Code, Course Name, Lecture(L-T-P-C), Type, Faculty, Program, Sem
```

| Field | Example | Notes |
|---|---|---|
| Code | `IT205` | Course code shared across multiple program offerings |
| Course Name | `Data Structures` | Descriptive course title |
| Lecture | `3-0-2-4` | Only the first value, lectures per week, is used |
| Type | `Core` | Only Core courses are auto-scheduled |
| Faculty | `JG` | Faculty identifier |
| Program | `ICTA` | Program name |
| Sem | `4` | Semester number |

Sample input files are provided in [data/autumn.csv](data/autumn.csv) and [data/winter.csv](data/winter.csv).

## Algorithm Summary

1. Read and validate CSV input.
2. Group rows by course code.
3. Count how many 1-, 2-, and 3-lecture Core courses exist for each program and semester.
4. Compute the required number of slots from the busiest program/semester combination.
5. Build slot groups in descending lecture-count order.
6. Pack compatible course groups into slots.
7. Place slots into a 5-day by 5-period weekly grid using priority order.
8. Validate the final timetable for repeated slots and faculty clashes.
9. Export workbook outputs as `.xlsx` files.

## Build and Run

### Prerequisites

- C++17 compiler such as `g++` 9+ or `clang++` 10+
- CMake 3.10+ or the included Makefile

### Build

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src\main.cpp src\CourseLoader.cpp src\TimetableGenerator.cpp -o timetable.exe
```

### Run

```powershell
.\timetable.exe
```

When prompted for the input file, enter the filename only, for example:

```text
autumn.csv
```

The program will resolve it from the current directory or from `data/`.

## Usage Flow

1. Enter the input CSV filename.
2. Enter the output filename for the slot breakdown workbook.
3. Enter the output filename for the master timetable workbook.
4. Optionally generate program-wise or faculty-wise workbooks.

## Sample Output

```text
Time,Monday,Tuesday,Wednesday,Thursday,Friday
8:00 - 8:50 , Free , M3 , M1 , M6 , M3 ,
9:00 - 9:50 , Free , M5 , M2 , Free , M5 ,
```

Program-wise output resolves each slot to the course code visible to that program and semester.

## Future Improvements

- Configurable number of working days and periods
- Room and lab allocation
- Optional web or GUI front-end
- Support for elective and 0-lecture scheduling
