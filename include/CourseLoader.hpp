#pragma once
#include <string>
#include <vector>
#include "Course.hpp"

// Reads the input course list. Expected columns (see README):
//   Code, Course Name, Lecture(L-T-P-C), Type, Faculty, Program, Sem
// Throws std::runtime_error with a line number on a malformed row instead of
// silently producing a corrupt Course (the original read garbage on bad input).
std::vector<Course> loadCoursesFromCsv(const std::string& path);
