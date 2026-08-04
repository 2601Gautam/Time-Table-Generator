#include <iostream>
#include <fstream>
#include <limits>
#include "CourseLoader.hpp"
#include "CourseCatalog.hpp"
#include "TimetableGenerator.hpp"

using namespace std;

namespace {

string resolveInputPath(const string& raw) {
    {
        ifstream direct(raw.c_str());
        if (direct.is_open()) return raw;
    }

    string dataPath = string("data/") + raw;
    {
        ifstream dataFile(dataPath.c_str());
        if (dataFile.is_open()) return dataPath;
    }

    return raw;
}

string prompt(const string& message) {
    cout << message;
    string value;
    cin >> value;
    return value;
}

int promptInt(const string& message) {
    cout << message;
    int value;
    while (!(cin >> value)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Please enter a number: ";
    }
    return value;
}

} // namespace

int main() {
    vector<Course> courses;
    try {
        string inputPath = resolveInputPath(prompt("Enter the name of the input CSV file : "));
        courses = loadCoursesFromCsv(inputPath);
    } catch (const exception& e) {
        cerr << "Could not read input: " << e.what() << "\n";
        return 1;
    }

    if (courses.empty()) {
        cerr << "Input file has no course rows -- nothing to schedule.\n";
        return 1;
    }

    SlotRequirement requirement = computeRequiredSlots(courses);
    vector<Slot> slots = buildSlots(requirement);
    CourseCatalog catalog(courses);

    vector<string> skippedOutOfScope;
    for (const auto& entry : catalog.groups()) {
        if (entry.second.lecturesPerWeek == 0) {
            skippedOutOfScope.push_back(entry.first);
        }
    }

    if (!skippedOutOfScope.empty()) {
        cerr << "Skipped " << skippedOutOfScope.size()
                  << " out-of-scope 0-lecture course(s): ";
        for (size_t i = 0; i < skippedOutOfScope.size(); ++i) {
            cerr << skippedOutOfScope[i] << (i + 1 < skippedOutOfScope.size() ? ", " : "\n");
        }
    }

    vector<string> unplaced = assignCoursesToSlots(catalog, slots);
    vector<string> realUnplaced;
    for (const auto& code : unplaced) {
        bool isSkipped = false;
        for (const auto& skipped : skippedOutOfScope) {
            if (code == skipped) {
                isSkipped = true;
                break;
            }
        }
        if (!isSkipped) realUnplaced.push_back(code);
    }
    if (!realUnplaced.empty()) {
        cerr << "Warning: " << realUnplaced.size()
                  << " course(s) could not be placed in any slot: ";
        for (size_t i = 0; i < realUnplaced.size(); ++i) {
            cerr << realUnplaced[i] << (i + 1 < realUnplaced.size() ? ", " : "\n");
        }
    }

    try {
        string slotFile = prompt("Enter the name of the xlsx file to save the slots : ");
        writeSlotBreakdownXlsx(slotFile, slots);

        Grid grid = buildWeeklyGrid(slots);

        auto violations = validateGrid(grid, slots);
        if (!violations.empty()) {
            cerr << "Warning: the generated grid has " << violations.size()
                      << " constraint violation(s):\n";
            for (const auto& v : violations) cerr << "  - " << v << "\n";
        }

        string ttFile = prompt("Enter the name of the xlsx file for time-table : ");
        writeMasterGridXlsx(ttFile, grid);

        while (true) {
            cout << "\nEnter 1. For Branch/Program wise time-table.\n"
                       "Enter 2. For Faculty's time-table.\n"
                       "Enter 0. To finish.\n";
            int choice = promptInt("> ");

            if (choice == 1) {
                string program = prompt("Enter the program : ");
                int sem = promptInt("Enter the sem : ");
                string fname =
                    prompt("Enter the xlsx file name to store time table of " + program + " Sem-" +
                           to_string(sem) + " : ");
                writeProgramGridXlsx(fname, grid, slots, program, sem);
            } else if (choice == 2) {
                string faculty = prompt("Enter short name of Faculty : ");
                string fname =
                    prompt("Enter the xlsx file name to store time table of " + faculty + " : ");
                writeFacultyGridXlsx(fname, grid, slots, faculty);
            } else if (choice == 0) {
                break;
            } else {
                cout << "Invalid option.\n";
            }
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    cout << "Done.\n";
    return 0;
}
