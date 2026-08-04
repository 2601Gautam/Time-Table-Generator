#pragma once
#include <string>

// One row of the input CSV: a single course as offered to one program/semester.
// If the same course code is taught to several programs (a shared lecture),
// each program/semester gets its own Course row -- CourseCatalog is what groups
// those rows back together by code.
class Course {
public:
    Course() = default;
    Course(std::string code, std::string name, int lecturesPerWeek, std::string type,
           std::string faculty, std::string program, int semester)
        : code_(std::move(code)), name_(std::move(name)), lecturesPerWeek_(lecturesPerWeek),
          type_(std::move(type)), faculty_(std::move(faculty)), program_(std::move(program)),
          semester_(semester) {}

    const std::string& code() const noexcept { return code_; }
    const std::string& name() const noexcept { return name_; }
    int lecturesPerWeek() const noexcept { return lecturesPerWeek_; }
    const std::string& type() const noexcept { return type_; }
    const std::string& faculty() const noexcept { return faculty_; }
    const std::string& program() const noexcept { return program_; }
    int semester() const noexcept { return semester_; }

    // Only "Core" courses are auto-scheduled (electives are out of scope, same as before).
    bool isCore() const noexcept { return type_ == "Core"; }

private:
    std::string code_;
    std::string name_;
    int lecturesPerWeek_ = 0;
    std::string type_;
    std::string faculty_;
    std::string program_;
    int semester_ = 0;
};
