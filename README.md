# DAIR: Lewis Starting Point
This repository contains an algorithm to find optimal starting points for a
a group of circular robots operating using Jeremy S. Lewis's 
[Guaranteed Coverage with a Blind Unreliable Robot algorithm](https://jokane.net/pubs/LewFesOKa18.pdf).

## Building the Program
The program to run the algorithm is built using CMake 4.2.3. Its only 
dependency is CGAL, which has instructions for installation on Linux, MacOS,
and Windows on their [installation page](https://www.cgal.org/download.html).

After installing CMake and CGAL, build the Makefiles with:
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
Where `<input_file>` is a text file containing the input parameters for the algorithm. The input file should be formatted as follows:
```
<number of boundary vertices> <x1> <y1> <x2> <y2> ... <xn> <yn>  <number of holes> <number of vertices in hole 1> <x1> <y1> ... <xn> <yn>  ...  <number of vertices in hole m> <x1> <y1> ... <xn> <yn>
<number of parameter pairs> <layer length 1> <layer overlap 1> <layer length 2> <layer overlap 2> ... <layer length k> <layer overlap k>
<robot turning error> <robot radius>
```

Note that there are two spaces between the last vertex of the boundary and the 
number of holes, as well as between the last vertex of each hole and the number
of vertices in the next hole. If a polygon has no holes, the number of holes 
should be 0 and there should be no additional data after the boundary vertices.
