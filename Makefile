CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
SRC      := src/main.cpp src/CourseLoader.cpp src/TimetableGenerator.cpp
BIN      := bin/timetable

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: all
	./$(BIN)

clean:
	rm -rf bin
