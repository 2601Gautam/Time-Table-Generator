#include "CourseLoader.hpp"
#include "Csv.hpp"
#include <fstream>
#include <sstream>

using namespace std;

vector<Course> loadCoursesFromCsv(const string& path) {
    ifstream in(path);
    if (!in.is_open()) {
        throw runtime_error("could not open input file: " + path);
    }

    vector<Course> courses;
    string line;
    int lineNo = 0;

    while (getline(in, line)) {
        ++lineNo;
        if (csv::trim(line).empty()) continue; // tolerate blank/trailing lines

        auto fields = csv::splitLine(line);
        if (fields.size() != 7) {
            ostringstream msg;
            msg << "line " << lineNo << ": expected 7 comma-separated fields "
                << "(Code,Name,Lecture,Type,Faculty,Program,Sem) but found " << fields.size();
            throw runtime_error(msg.str());
        }

        try {
            int lecturesPerWeek = csv::parseLeadingInt(fields[2]);
            int semester = csv::parseLeadingInt(fields[6]);
            courses.emplace_back(fields[0], fields[1], lecturesPerWeek, fields[3], fields[4],
                                  fields[5], semester);
        } catch (const exception& e) {
            ostringstream msg;
            msg << "line " << lineNo << ": " << e.what();
            throw runtime_error(msg.str());
        }
    }

    return courses;
}
