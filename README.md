# 📅 Time-Table Generator

**An automatic academic time-table generator built in modern C++ — clean CSVs in, Excel workbooks out, no manual scheduling.**

> Originally built as a Data Structures & Algorithms capstone project by **intMinds: a byte per member**.
> This version reworks the internals onto the STL and splits the single 1150-line file into a small,
> testable module set, while keeping the scheduling algorithm and constraints unchanged.

---

## 🧠 What it does

Feed it a semester's course list — codes, weekly lecture counts, faculty, programs, and semesters — and it will:

1. Read and validate every course from a CSV file.
2. Group identical courses (same code, same lecture count) taught to multiple programs together, so a
   shared lecture isn't scheduled twice.
3. Work out how many weekly **slots** are needed per lecture-count, based on the busiest program/semester
   combination.
4. Pack courses into slots while respecting real scheduling constraints (see below).
5. Arrange those slots into a **5-day × 5-period weekly grid**, placing heavier (more frequent) courses first.
6. Export the result as Excel workbooks (`.xlsx`) — the master time table, the raw slot breakdown, or
  filtered views **per faculty member** and **per program/semester**.

No two clashing classes. No professor double-booked. No manual spreadsheet wrangling.

---

## ⚙️ How the scheduling works

**Slot packing**
- Only *Core* courses are auto-scheduled.
- A slot only ever holds courses with the same weekly lecture count.
- A course is skipped for a slot if its program+semester is already represented there (no double class
  for the same batch), or if one of its faculty members is already teaching in that slot.

**Weekly placement**
- Each slot starts with a priority equal to its required lecture count per week.
- Slots are placed day-by-day, highest remaining priority first, decrementing after each placement — so a
  3-lecture course is guaranteed 3 placements across the week.
- A slot is never repeated on the same day.
- The same faculty member is never scheduled in two **consecutive** periods.
- Anything that can't be placed without breaking a rule is left as a free period.

**Known scope limits (inherited from the original)**
- Courses whose weekly lecture count is 0 (lab-only rows, e.g. an L-T-P-C code of `0-0-2-1`) are out of
  scope for auto-scheduling, same as before — the program prints which course codes were skipped so this
  is visible instead of silent.
- Electives / non-Core rows are read but never auto-slotted.

---

## 🔧 What changed from the original, and why

The scheduling *rules* above are unchanged — same constraints, same priority-based placement idea. What
changed is the implementation:

| Original | Now | Why |
|---|---|---|
| Hand-rolled singly linked list + manual pointer walk (`class node`, `class PriorityQ`) for weekly placement | `PriorityScheduler`, a thin wrapper over `std::multimap<int,int,std::greater<int>>` | Same behaviour, no manual list surgery; insert/scan are `O(log n)`/`O(n)` on a container that's already sorted for you |
| Hand-rolled open-chaining hash table (`class Hash` + `list<Course>` chain), built with a linear `repeat_course` dedup scan | `CourseCatalog`, a `std::map<string, CourseGroup>` | One pass to group courses by code; deterministic (alphabetical) iteration means the same input always packs into slots the same way |
| Five fixed-size parallel arrays per slot (`code[100]`, `pg[100]`, `sem[100]`, `fac[100]`, `type[100]`) plus one hardcoded `bool[8]` per program name (`ICTA`, `ICTB`, `CS`, `MNC`, `EVD`) | `Slot` holding `vector<Assignment>` + `set<pair<program,sem>>` + `unordered_set<faculty>` | No 100-row cap, no assumption about which program names exist |
| `no_of_slots()` hardcoded exactly the program names `{ICTA, CS, MNC, EVD}` — **`ICTB` was never counted**, so if it ever had the heaviest load for some lecture-count, its courses could silently run out of slots | `computeRequiredSlots()` discovers programs/semesters from the data itself, one pass | Genuine correctness bug fix; also makes the tool work on any program-name set, not just this one |
| Raw C-style VLAs everywhere (`Course arr[count]`, `slot sl[tot_slot]`, ...) — non-standard C++, a GCC extension that isn't portable | `std::vector` | Standard C++17, works with any conforming compiler |
| No input validation — a malformed CSV row silently produced a garbage `Course` (undefined `>>` behaviour) | `loadCoursesFromCsv` throws with a line number on a malformed row | Fails loudly and immediately instead of scheduling garbage |
| Silent data loss — a course that never found a slot just never appeared anywhere in the output | `assignCoursesToSlots` returns the codes it couldn't place; `main` prints them | Same packing behaviour, but nothing disappears without a trace |
| One filtered (program- or faculty-wise) export per run | CLI loops until you choose to stop | Minor usability improvement, same export logic |
| — | `validateGrid()` re-checks every constraint after the grid is built and reports any violation | Cheap regression check; confirms the rewrite still produces a clash-free table |

---

## 📂 Project structure

```
TimeTableGenerator/
├── CMakeLists.txt
├── Makefile
├── README.md
├── include/
│   ├── Course.hpp             # one course-program-semester row
│   ├── Csv.hpp                 # line splitting / trimming / L-T-P-C parsing
│   ├── CourseLoader.hpp        # CSV -> vector<Course>
│   ├── CourseCatalog.hpp       # groups rows back together by course code
│   ├── Slot.hpp                 # a weekly time slot ("M1", "M2", ...)
│   ├── PriorityScheduler.hpp   # STL-backed weekly placement queue
│   ├── Constants.hpp            # grid size, free-slot sentinel
│   └── TimetableGenerator.hpp  # the scheduling pipeline + Excel export, declared
├── src/
│   ├── CourseLoader.cpp
│   ├── TimetableGenerator.cpp  # sizing, packing, grid placement, validation, export
│   └── main.cpp                 # CLI
└── data/
    ├── autumn.csv               # sample input
    └── winter.csv                # sample input
```

---

## 📥 Input format

Course data is read from a CSV file with the following columns, in order:

```
Code, Course Name, Lecture(L-T-P-C), Type, Faculty, Program, Sem
```

| Field | Example | Notes |
|---|---|---|
| `Code` | `IT205` | Course code — shared across programs to mark a joint lecture |
| `Course Name` | `Data Structures` | Full name |
| `Lecture` | `3-0-2-4` | `L-T-P-C`; only the leading `L` (lectures/week) is used |
| `Type` | `Core` | Only `Core` courses are auto-scheduled |
| `Faculty` | `JG` | Short faculty identifier |
| `Program` | `ICTA` | Any program name — no longer hardcoded |
| `Sem` | `4` | Semester number |

Two ready-to-use sample files are included: [`data/autumn.csv`](./data/autumn.csv) and
[`data/winter.csv`](./data/winter.csv).

---

## 🚀 Getting started

### Prerequisites
A C++17 compiler (`g++` 9+ or `clang++` 10+). CMake 3.10+ is optional — a plain `Makefile` is included too.

### Build & run — all in one go

**Option A — CMake**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/timetable
```

**Option B — Makefile**
```bash
make run
```
(`make` alone builds `bin/timetable` without running it.)

### Usage flow

Once running, the program will prompt you to:

1. Enter the input course CSV filename only (e.g. `autumn.csv`); the program reads it from `data/`.
2. Enter a filename for the raw slot breakdown `.xlsx` file.
3. Enter a filename for the master weekly time table `.xlsx` file.
4. Repeatedly choose `1` (program-wise), `2` (faculty-wise), or `0` (finish) to export as many filtered
   views as you need.

Each export is a standalone `.xlsx` workbook you can open directly in Excel, Google Sheets, or any
spreadsheet tool.

---

## 🖥️ Sample output

```
Time,Monday,Tuesday,Wednesday,Thursday,Friday
8:00 - 8:50 , Free , M3 , M1 , M6 , M3 ,
9:00 - 9:50 , Free , M5 , M2 , Free , M5 ,
...
```

Filtered per-program output resolves each slot to the actual course code taught to that program in that
period, so a student or faculty member only ever sees their own schedule.

---

## 💡 Possible future improvements

- Configurable number of working days / periods (grid size now lives in one place, `Constants.hpp`, but
  isn't yet exposed as a CLI/config option)
- Room/lab allocation alongside faculty and time slot
- A simple web or GUI front-end over the existing C++ core
- Support for elective/non-core and 0-lecture (lab-only) course scheduling
