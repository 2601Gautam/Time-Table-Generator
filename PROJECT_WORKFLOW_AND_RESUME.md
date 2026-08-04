# TimeTableGenerator: Complete Project Walkthrough

## 1. What This Project Does

TimeTableGenerator is a C++ academic scheduling tool that reads course data from CSV, groups related course rows, computes how many weekly slots are needed, places courses into slots while avoiding clashes, and exports the final timetable as Excel workbooks.

It supports:

- raw slot breakdown output
- master weekly timetable output
- program-wise semester timetable output
- faculty-wise timetable output

The current implementation writes real `.xlsx` files and allows the user to type only the input filename, such as `autumn.csv`, while the program automatically reads from `data/`.

## 2. End-to-End Flow

### Step 1: Load the input CSV

The application starts in `src/main.cpp`.

- The user enters a filename like `autumn.csv`.
- The program first checks whether the file exists as typed.
- If not, it automatically tries `data/autumn.csv`.
- `loadCoursesFromCsv()` reads the CSV and converts every valid row into a `Course` object.

If a row is malformed, the loader throws an error with the line number.

### Step 2: Build the course catalog

After loading, the code creates a `CourseCatalog`.

- Courses are grouped by course code.
- All rows for the same course code are treated as one shared lecture group.
- This avoids duplicating the same lecture for multiple program/semester combinations.

### Step 3: Compute slot requirements

`computeRequiredSlots()` scans all Core courses.

- Only lecture counts of 1, 2, or 3 are used for auto-scheduling.
- Courses with 0 lectures are out of scope for placement and are reported separately.
- For every `(program, semester)` pair, the code counts how many courses need each lecture count.
- The final slot count is the worst-case requirement across all program/semester combinations.

This ensures the schedule has enough slots for the busiest batch.

### Step 4: Create slots

`buildSlots()` creates empty `Slot` objects.

- Slots are numbered `M1`, `M2`, and so on.
- 3-lecture slots are created first, then 2-lecture slots, then 1-lecture slots.

This preserves the slot ordering expected by the rest of the algorithm.

### Step 5: Pack courses into slots

`assignCoursesToSlots()` tries to place each course group into a compatible slot.

A course group can go into a slot only if:

- the lecture count matches the slot
- the same program/semester is not already present in that slot
- none of the assigned faculty members already exist in that slot

If a course cannot be placed, it is returned as unplaced so the program can report it.

### Step 6: Build the weekly grid

`buildWeeklyGrid()` places the slots into a 5-day × 5-period matrix.

- higher-frequency slots are prioritized first
- a slot is never repeated on the same day
- the same faculty member is not placed in consecutive periods
- free periods are inserted if there is extra capacity

If the total load is larger than the available grid cells, the function throws an error.

### Step 7: Validate the schedule

`validateGrid()` re-checks the result after the grid is built.

It looks for:

- repeated slots on the same day
- faculty clashes in adjacent periods

This is a safety check that helps confirm the generated timetable is consistent.

### Step 8: Export to Excel

The project now writes `.xlsx` workbooks directly.

Exports include:

- slot breakdown workbook
- master timetable workbook
- program-wise workbook for a selected program and semester
- faculty-wise workbook for a selected faculty member

The workbook generator creates valid Excel package files with the required XML parts.

## 3. How The Algorithm Works

This is the part the code is actually doing under the hood.

### 3.1 Build a frequency map of course demand

`computeRequiredSlots()` scans every loaded course row and builds a map keyed by:

- program
- semester
- lecture count

For each `(program, semester)` pair, it counts how many 1-lecture, 2-lecture, and 3-lecture Core courses exist. The required number of slots for each lecture size is then taken from the largest count seen across all program/semester groups.

In simple terms:

```text
required_1 = max(count of 1-lecture Core courses for every batch)
required_2 = max(count of 2-lecture Core courses for every batch)
required_3 = max(count of 3-lecture Core courses for every batch)
```

This is why the timetable always has enough slot groups for the busiest batch.

### 3.2 Group identical course codes together

`CourseCatalog` turns the flat CSV list into course groups.

- same course code → one group
- each group stores all program/semester rows for that code
- this allows shared lectures to be scheduled once instead of duplicated

Example:

- if `HM106` appears for `ICTA-2`, `ICTB-2`, and `CS-2`, they are treated as one shared lecture group
- the algorithm then places that group one time in the slot system, and all matching program rows point to it

### 3.3 Sort slots by weekly need

`buildSlots()` creates empty slot objects with lecture counts like 3, 2, or 1.

`PriorityScheduler` then keeps those slots ordered by remaining weekly demand.

The rule is:

- a 3-lecture slot must appear 3 times in the week
- a 2-lecture slot must appear 2 times
- a 1-lecture slot must appear 1 time

The scheduler always picks the highest remaining priority first, which means heavier courses are placed before lighter ones.

### 3.4 Place courses into slots

`assignCoursesToSlots()` is a greedy packing step.

For each slot, it scans the grouped courses and checks whether the group fits.

A group fits only if all of these are true:

- its lecture count matches the slot lecture count
- the same program/semester is not already inside the slot
- none of the group’s faculty members are already inside the slot

If it fits, all rows in that course group are added to the same slot.

This is the main anti-clash rule at the slot level.

### 3.5 Place slots into the weekly grid

`buildWeeklyGrid()` uses a 5 × 5 grid:

- rows = periods
- columns = days

It repeatedly takes the best available slot from `PriorityScheduler` and tries to place it into the next valid cell.

The placement rules are:

- do not repeat the same slot twice on the same day
- do not place the same faculty in consecutive periods
- if a choice would violate a rule, skip it and try the next candidate

If no valid slot is available for a cell, the cell stays free.

So the algorithm is greedy, not backtracking. It makes the best valid local choice at each cell instead of searching all possibilities.

### 3.6 Validate after placement

After the grid is built, `validateGrid()` checks that the result still obeys the intended rules.

It reports:

- repeated slot numbers on the same day
- faculty clashes in adjacent periods

This is a guardrail so bugs in placement logic do not silently produce a bad timetable.

### 3.7 Export the final layout

The Excel export step does not change the algorithm. It only formats the final results into sheets:

- raw slot assignment view
- master timetable view
- program/semester timetable view
- faculty timetable view

## 4. Important Design Choices

- The project uses standard C++ containers instead of manual linked lists and raw arrays.
- The code keeps the scheduling logic separate from file I/O.
- Input filenames are resolved automatically from `data/` for easier use.
- Semester-wise program export supports program prefixes such as `ICT`, which match `ICTA` and `ICTB`.

## 5. How To Run It

Build:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src\main.cpp src\CourseLoader.cpp src\TimetableGenerator.cpp -o timetable.exe
```

Run:

```powershell
.\timetable.exe
```

At the prompt, enter only the filename:

```text
autumn.csv
```

The program will automatically load it from `data/`.

## 6. Resume-Ready Description For Obsidian Capital

### Short Summary

Built a performance-conscious timetable generator in modern C++ that loads academic course data, groups shared lectures, computes slot requirements, resolves scheduling constraints, and exports clash-free timetables as Excel workbooks.

### Resume Bullets

- Designed and implemented a modular C++17 timetable generation system using STL containers, clear separation of concerns, and deterministic scheduling logic.
- Replaced manual list and array-based scheduling structures with `std::map`, `std::vector`, `std::set`, and `std::unordered_set` to improve maintainability and correctness.
- Built a constraint-based scheduling pipeline that groups shared lectures, calculates slot requirements per program and semester, and prevents faculty/program clashes.
- Added validation checks to detect repeated slots and adjacent faculty conflicts after timetable construction.
- Implemented Excel workbook export directly from C++, producing `.xlsx` output for slot breakdowns, master timetables, program-wise timetables, and faculty-wise timetables.
- Improved usability by auto-resolving input CSV files from the `data/` directory and supporting program-prefix semester exports such as `ICT` → `ICTA` / `ICTB`.

### JD-Aligned Technical Description

This project demonstrates the same kind of engineering discipline valued in the Obsidian Capital role: careful systems design, low-level correctness, deterministic behavior, and production-grade C++ implementation. It required reasoning about data layout, algorithmic constraints, validation, and clean modular code.

## 7. Pseudocode View Of The Core Algorithm

This is the shortest code-level explanation of how the project works.

### 7.1 Load and normalize input

```text
read input filename from user
if file exists as typed:
	use it
else if data/filename exists:
	use that
else:
	fail with error

for each CSV row:
	parse code, name, lecture count, type, faculty, program, semester
	create Course object
```

### 7.2 Group by course code

```text
for each Course:
	if course is Core:
		add it to CourseCatalog under its course code
```

### 7.3 Compute how many slots are needed

```text
for each Course:
	ignore if not Core
	ignore if lecture count is not 1, 2, or 3
	increase count for (program, semester, lecture count)

for each (program, semester):
	required_1 = max(required_1, count of 1-lecture courses)
	required_2 = max(required_2, count of 2-lecture courses)
	required_3 = max(required_3, count of 3-lecture courses)
```

### 7.4 Build slots

```text
create slots for lecture-3 courses first
create slots for lecture-2 courses next
create slots for lecture-1 courses last
assign slot numbers M1, M2, M3, ...
```

### 7.5 Pack course groups into slots

```text
for each slot:
	for each course group:
		if lecture count matches slot lecture count
		   and program/semester is not already used in slot
		   and faculty is not already used in slot:
			   place entire group in slot
			   mark group as placed
```

### 7.6 Fill the weekly grid

```text
create 5 x 5 grid
put all slots into a priority queue by remaining weekly count

for each day:
	for each period:
		pick the highest-priority slot that:
			is not already used on this day
			does not create back-to-back faculty clash
		place it in the cell
		decrement its remaining count
		if nothing fits:
			leave cell free
```

### 7.7 Validate result

```text
scan every day and period
if same slot repeats on same day:
	report issue
if faculty appears in adjacent periods:
	report issue
```

### 7.8 Export Excel

```text
convert rows to XML worksheet data
package workbook parts into .xlsx structure
write slot sheet, master timetable sheet, program sheet, or faculty sheet
```

## 8. Resume Summary Paragraph

Modern C++ developer with hands-on experience building a modular timetable generation system that loads and validates CSV data, groups shared lecture offerings, computes schedule capacity, resolves clash-free slot allocation, and exports Excel-based timetables. Strong focus on clean C++17 design, STL-based data structures, deterministic behavior, and reliable output generation.
