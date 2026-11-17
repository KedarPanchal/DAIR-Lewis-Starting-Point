# DAIR: Lewis Starting Point
This repository contains an algorithm to find optimal starting points for a
a group of circular robots operating using Jeremy S. Lewis's 
[Guaranteed Coverage with a Blind Unreliable Robot algorithm](https://jokane.net/pubs/LewFesOKa18.pdf).

## Building the Program
The program to run the algorithm is built using CMake 4.2.3. Its dependencies
are CGAL, which has instructions for installation on Linux, MacOS, and Windows
on their [installation page](https://www.cgal.org/download.html), and SQLite3,
which is used for the graph cache.

After installing CMake, CGAL, and SQLite3, build the Makefiles with:
```bash
cmake -S . -B build
```

After building the Makefiles, compile the program with:
```bash
cmake --build build
```

## Running the Program
The program can be run with the following command:
```bash
./build/robot_start_position < <input_file>
```
Where `<input_file>` is a `.dat` file describing the environment and the robot.
See [the test case generation skill](.agents/skills/generating-test-cases/SKILL.md)
for how to format one, and `test_data/` for examples.
