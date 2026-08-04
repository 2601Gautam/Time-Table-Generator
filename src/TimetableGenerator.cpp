#include "TimetableGenerator.hpp"
#include "PriorityScheduler.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using namespace std;

SlotRequirement computeRequiredSlots(const vector<Course>& courses) {
    // One pass, tallying 1/2/3-lecture Core course counts per (program, semester).
    map<pair<string, int>, array<int, 4>> counts;
    for (const auto& c : courses) {
        if (!c.isCore()) continue;
        int lec = c.lecturesPerWeek();
        if (lec < 1 || lec > 3) continue; // matches original scope: only 1/2/3-lecture courses are auto-slotted
        counts[{c.program(), c.semester()}][lec]++;
    }

    SlotRequirement req;
    for (map<pair<string, int>, array<int, 4>>::const_iterator it = counts.begin();
         it != counts.end(); ++it) {
        const array<int, 4>& tally = it->second;
        req.forLecture1 = max(req.forLecture1, tally[1]);
        req.forLecture2 = max(req.forLecture2, tally[2]);
        req.forLecture3 = max(req.forLecture3, tally[3]);
    }
    return req;
}

vector<Slot> buildSlots(const SlotRequirement& requirement) {
    vector<Slot> slots;
    slots.reserve(requirement.total());
    int slotNumber = 1;
    for (int i = 0; i < requirement.forLecture3; ++i) slots.emplace_back(slotNumber++, 3);
    for (int i = 0; i < requirement.forLecture2; ++i) slots.emplace_back(slotNumber++, 2);
    for (int i = 0; i < requirement.forLecture1; ++i) slots.emplace_back(slotNumber++, 1);
    return slots;
}

vector<string> assignCoursesToSlots(CourseCatalog& catalog, vector<Slot>& slots) {
    for (auto& slot : slots) {
        for (map<string, CourseGroup>::iterator it = catalog.groups().begin();
             it != catalog.groups().end(); ++it) {
            CourseGroup& group = it->second;
            if (group.placed) continue;
            if (group.lecturesPerWeek != slot.lecturesPerWeek()) continue;

            bool fits = true;
            for (const auto& section : group.sections) {
                if (slot.hasProgramSemester(section.program(), section.semester()) ||
                    slot.hasFaculty(section.faculty())) {
                    fits = false;
                    break;
                }
            }
            if (!fits) continue;

            for (const auto& section : group.sections) {
                slot.addAssignment(Assignment{section.code(), section.program(), section.semester(),
                                               section.faculty(), section.type()});
            }
            group.placed = true;
        }
    }

    vector<string> unplaced;
    for (map<string, CourseGroup>::const_iterator it = catalog.groups().begin();
         it != catalog.groups().end(); ++it) {
        const string& code = it->first;
        const CourseGroup& group = it->second;
        if (!group.placed) unplaced.push_back(code);
    }
    return unplaced;
}

Grid buildWeeklyGrid(const vector<Slot>& slots) {
    int totalTeachingPeriods = 0;
    for (const auto& s : slots) totalTeachingPeriods += s.lecturesPerWeek();

    const int totalCells = NUM_DAYS * NUM_PERIODS;
    if (totalTeachingPeriods > totalCells) {
        ostringstream msg;
        msg << "the input needs " << totalTeachingPeriods << " teaching periods per week, but a "
            << NUM_DAYS << "x" << NUM_PERIODS << " grid only has " << totalCells
            << " available. Reduce the course load or widen NUM_DAYS/NUM_PERIODS in Constants.hpp.";
        throw runtime_error(msg.str());
    }

    PriorityScheduler scheduler;
    for (const auto& s : slots) scheduler.push(s.slotNumber(), s.lecturesPerWeek());
    const int freeNeeded = totalCells - totalTeachingPeriods;
    if (freeNeeded > 0) scheduler.push(FREE_SLOT, freeNeeded);

    Grid grid(NUM_PERIODS + 1, vector<int>(NUM_DAYS + 1, FREE_SLOT));
    auto byNumber = [&](int slotNumber) -> const Slot& { return slots[slotNumber - 1]; };

    for (int day = 1; day <= NUM_DAYS && !scheduler.empty(); ++day) {
        unordered_set<int> usedToday;
        for (int period = 1; period <= NUM_PERIODS && !scheduler.empty(); ++period) {
            const int prevSlotNumber = (period > 1) ? grid[period - 1][day] : FREE_SLOT;

            int chosen = scheduler.takeBestMatching([&](int id) {
                if (id == FREE_SLOT) return true; // a free period never clashes with anything
                if (usedToday.count(id)) return false; // no repeat of the same slot on the same day
                if (prevSlotNumber != FREE_SLOT && shareFaculty(byNumber(prevSlotNumber), byNumber(id))) {
                    return false; // no faculty member back-to-back across periods
                }
                return true;
            });

            if (chosen == -1) continue; // nothing schedulable here without a clash; leave it Free
            grid[period][day] = chosen;
            if (chosen != FREE_SLOT) usedToday.insert(chosen);
        }
    }
    return grid;
}

vector<string> validateGrid(const Grid& grid, const vector<Slot>& slots) {
    vector<string> violations;
    auto byNumber = [&](int slotNumber) -> const Slot& { return slots[slotNumber - 1]; };
    static const char* dayNames[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

    for (int day = 1; day <= NUM_DAYS; ++day) {
        unordered_set<int> seenToday;
        for (int period = 1; period <= NUM_PERIODS; ++period) {
            int cur = grid[period][day];
            if (cur == FREE_SLOT) continue;

            if (seenToday.count(cur)) {
                violations.push_back("M" + to_string(cur) + " repeats on " +
                                      string(dayNames[day]));
            }
            seenToday.insert(cur);

            if (period > 1) {
                int prev = grid[period - 1][day];
                if (prev != FREE_SLOT && shareFaculty(byNumber(prev), byNumber(cur))) {
                    violations.push_back("faculty clash between M" + to_string(prev) + " and M" +
                                          to_string(cur) + " on " + string(dayNames[day]) +
                                          ", periods " + to_string(period - 1) + "-" +
                                          to_string(period));
                }
            }
        }
    }
    return violations;
}

// --- Excel export -----------------------------------------------------------

namespace {

struct ZipEntry {
    string name;
    string data;
};

void writeU16(ostream& out, uint16_t value) {
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
}

void writeU32(ostream& out, uint32_t value) {
    writeU16(out, static_cast<uint16_t>(value & 0xFFFF));
    writeU16(out, static_cast<uint16_t>((value >> 16) & 0xFFFF));
}

uint32_t crc32(const string& data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

string xmlEscape(const string& text) {
    string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

string xmlCellRef(int row, int column) {
    string ref;
    int n = column;
    while (n > 0) {
        int remainder = (n - 1) % 26;
        ref.insert(ref.begin(), static_cast<char>('A' + remainder));
        n = (n - 1) / 26;
    }
    ref += to_string(row);
    return ref;
}

string makeInlineStringCell(int row, int column, const string& value) {
    ostringstream xml;
    xml << "<c r=\"" << xmlCellRef(row, column) << "\" t=\"inlineStr\"><is><t";
    if (!value.empty() && (value.front() == ' ' || value.back() == ' ')) {
        xml << " xml:space=\"preserve\"";
    }
    xml << ">" << xmlEscape(value) << "</t></is></c>";
    return xml.str();
}

string makeSheetXml(const vector<vector<string>>& rows) {
    ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    xml << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">";
    xml << "<sheetData>";

    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        const auto& row = rows[rowIndex];
        bool hasCells = false;
        for (const auto& cell : row) {
            if (!cell.empty()) {
                hasCells = true;
                break;
            }
        }
        if (!hasCells) continue;

        xml << "<row r=\"" << (rowIndex + 1) << "\">";
        for (size_t columnIndex = 0; columnIndex < row.size(); ++columnIndex) {
            if (row[columnIndex].empty()) continue;
            xml << makeInlineStringCell(static_cast<int>(rowIndex + 1), static_cast<int>(columnIndex + 1),
                                        row[columnIndex]);
        }
        xml << "</row>";
    }

    xml << "</sheetData></worksheet>";
    return xml.str();
}

string makeWorkbookXml(const string& sheetName) {
    ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    xml << "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">";
    xml << "<sheets><sheet name=\"" << xmlEscape(sheetName)
        << "\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>";
    return xml.str();
}

string makeWorkbookRelsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
           "</Relationships>";
}

string makeRootRelsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
           "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
           "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
           "</Relationships>";
}

string makeContentTypesXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
           "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
           "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
           "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
           "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
           "</Types>";
}

string makeCorePropsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
           "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
           "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
           "xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
           "<dc:creator>TimeTableGenerator</dc:creator>"
           "<cp:lastModifiedBy>TimeTableGenerator</cp:lastModifiedBy>"
           "</cp:coreProperties>";
}

string makeAppPropsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" "
           "xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\">"
           "<Application>TimeTableGenerator</Application>"
           "</Properties>";
}

void writeStoredZip(const string& path, const vector<ZipEntry>& entries) {
    ofstream out(path, ios::binary);
    if (!out.is_open()) throw runtime_error("could not open output file: " + path);

    struct CentralRecord {
        string name;
        uint32_t crc = 0;
        uint32_t size = 0;
        uint32_t offset = 0;
    };

    vector<CentralRecord> centralRecords;
    uint32_t offset = 0;

    for (const auto& entry : entries) {
        CentralRecord record;
        record.name = entry.name;
        record.crc = crc32(entry.data);
        record.size = static_cast<uint32_t>(entry.data.size());
        record.offset = offset;

        writeU32(out, 0x04034b50u);
        writeU16(out, 20);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU32(out, record.crc);
        writeU32(out, record.size);
        writeU32(out, record.size);
        writeU16(out, static_cast<uint16_t>(record.name.size()));
        writeU16(out, 0);
        out.write(record.name.data(), static_cast<streamsize>(record.name.size()));
        out.write(entry.data.data(), static_cast<streamsize>(entry.data.size()));

        offset += 30u + static_cast<uint32_t>(record.name.size()) + record.size;
        centralRecords.push_back(record);
    }

    const uint32_t centralDirectoryOffset = offset;
    for (const auto& record : centralRecords) {
        writeU32(out, 0x02014b50u);
        writeU16(out, 20);
        writeU16(out, 20);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU32(out, record.crc);
        writeU32(out, record.size);
        writeU32(out, record.size);
        writeU16(out, static_cast<uint16_t>(record.name.size()));
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU32(out, 0);
        writeU32(out, record.offset);
        out.write(record.name.data(), static_cast<streamsize>(record.name.size()));
        offset += 46u + static_cast<uint32_t>(record.name.size());
    }

    const uint32_t centralDirectorySize = offset - centralDirectoryOffset;
    writeU32(out, 0x06054b50u);
    writeU16(out, 0);
    writeU16(out, 0);
    writeU16(out, static_cast<uint16_t>(centralRecords.size()));
    writeU16(out, static_cast<uint16_t>(centralRecords.size()));
    writeU32(out, centralDirectorySize);
    writeU32(out, centralDirectoryOffset);
    writeU16(out, 0);
}

void writeWorkbookXlsx(const string& path, const string& sheetName,
                       const vector<vector<string>>& rows) {
    vector<ZipEntry> entries;
    entries.push_back({"[Content_Types].xml", makeContentTypesXml()});
    entries.push_back({"_rels/.rels", makeRootRelsXml()});
    entries.push_back({"docProps/core.xml", makeCorePropsXml()});
    entries.push_back({"docProps/app.xml", makeAppPropsXml()});
    entries.push_back({"xl/workbook.xml", makeWorkbookXml(sheetName)});
    entries.push_back({"xl/_rels/workbook.xml.rels", makeWorkbookRelsXml()});
    entries.push_back({"xl/worksheets/sheet1.xml", makeSheetXml(rows)});
    writeStoredZip(path, entries);
}

vector<vector<string>> slotRows(const vector<Slot>& slots) {
    vector<vector<string>> rows;
    for (const auto& slot : slots) {
        rows.push_back({"Slot : M" + to_string(slot.slotNumber())});
        rows.push_back({"SEM", "PROGRAM", "CODE", "LECTURE", "TYPE", "FACULTY"});
        for (const auto& a : slot.assignments()) {
            rows.push_back({"Sem-" + to_string(a.semester), a.program, a.courseCode,
                            to_string(slot.lecturesPerWeek()), a.type, a.faculty});
        }
        rows.push_back({});
    }
    return rows;
}

string periodLabel(int period) {
    int startHour = START_HOUR + (period - 1);
    ostringstream label;
    label << startHour << ":00 - " << startHour << ":50";
    return label.str();
}

vector<vector<string>> masterGridRows(const Grid& grid) {
    vector<vector<string>> rows;
    rows.push_back({"Time", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"});
    for (int period = 1; period <= NUM_PERIODS; ++period) {
        vector<string> row;
        row.push_back(periodLabel(period));
        for (int day = 1; day <= NUM_DAYS; ++day) {
            int slotNumber = grid[period][day];
            row.push_back(slotNumber == FREE_SLOT ? "Free" : "M" + to_string(slotNumber));
        }
        rows.push_back(move(row));
    }
    return rows;
}

vector<vector<string>> programGridRows(const Grid& grid, const vector<Slot>& slots,
                                                      const string& program, int semester) {
    vector<vector<string>> rows;
    rows.push_back({"Time-Table For " + program + " Sem-" + to_string(semester)});
    rows.push_back({"Time", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"});
    for (int period = 1; period <= NUM_PERIODS; ++period) {
        vector<string> row;
        row.push_back(periodLabel(period));
        for (int day = 1; day <= NUM_DAYS; ++day) {
            int slotNumber = grid[period][day];
            const Assignment* a = slotNumber == FREE_SLOT
                                       ? nullptr
                                       : slots[slotNumber - 1].forProgramSemester(program, semester);
            row.push_back(a ? a->courseCode : "Free");
        }
        rows.push_back(move(row));
    }
    return rows;
}

vector<vector<string>> facultyGridRows(const Grid& grid, const vector<Slot>& slots,
                                                      const string& faculty) {
    vector<vector<string>> rows;
    rows.push_back({"Time-Table For " + faculty});
    rows.push_back({"Time", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"});
    for (int period = 1; period <= NUM_PERIODS; ++period) {
        vector<string> row;
        row.push_back(periodLabel(period));
        for (int day = 1; day <= NUM_DAYS; ++day) {
            int slotNumber = grid[period][day];
            const Assignment* a = slotNumber == FREE_SLOT ? nullptr : slots[slotNumber - 1].forFaculty(faculty);
            row.push_back(a ? a->courseCode : "Free");
        }
        rows.push_back(move(row));
    }
    return rows;
}

} // namespace

void writeSlotBreakdownXlsx(const string& path, const vector<Slot>& slots) {
    writeWorkbookXlsx(path, "Slots", slotRows(slots));
}

void writeMasterGridXlsx(const string& path, const Grid& grid) {
    writeWorkbookXlsx(path, "Timetable", masterGridRows(grid));
}

void writeProgramGridXlsx(const string& path, const Grid& grid, const vector<Slot>& slots,
                          const string& program, int semester) {
    writeWorkbookXlsx(path, program + " Sem-" + to_string(semester),
                       programGridRows(grid, slots, program, semester));
}

void writeFacultyGridXlsx(const string& path, const Grid& grid, const vector<Slot>& slots,
                          const string& faculty) {
    writeWorkbookXlsx(path, faculty, facultyGridRows(grid, slots, faculty));
}
