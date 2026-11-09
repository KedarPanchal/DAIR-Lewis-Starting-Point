---
name: generating-test-cases
description: Generates .dat test case files for 2-D polygonal environments a 
circular robot is meant to navigate in. Use when the user asks to "generate 
test cases" , "create test cases" or "generate .dat files". Works from a 
description of the environment. If none is provided, it will generate a random 
environment.
---

# Generating Test Cases

## Overview

Generate a `.dat` test case file for a 2-D polygonal environment that a 
circular robot is meant to navigate in. The environment can be described by the 
user, or if no description is provided, a random environment will be generated. 
The output will be a `.dat` file containing three lines: 

1. The polygonal environment itself, 
2. The pairs of parameters discretizing the environment
3. The robot's maximum rotational error and its radius.

## Step 1: Describe the Environment

The polygonal environment is a polygon with a number of polygonal holes. 
Neither the polygon nor its holes have curvature in their edges or have any
self-intersections, but both may be convex or non-convex. No three vertices
of the polygon or its holes may be collinear.

Polygons are described using the Well-Known Text (WKT) format. A WKT 
representation of a polygon with holes is formatted as follows:

```
POLYGON((x1 y1, x2 y2, ..., xn yn), (h1_x1 h1_y1, h1_x2 h1_y2, ..., h1_xn h1_yn), ..., (hm_x1 hm_y1, hm_x2 hm_y2, ..., hm_xn hm_yn))
```

Where `n` is the number of vertices in the polygon, `m` is the number of holes, 
and `h1`, `h2`, ..., `hm` are the holes of the polygon. The outer boundary
is described by a list of its vertices in counter-clockwise order, and the 
holes are described by a list of their vertices in clockwise order. The first 
and last vertex of the boundary and holes are the same to close the polygon. 
Each vertex is represented by its x and y coordinates, separated by a space.

## Step 2: Discretize the Environment

The polygonal environment is discretized into a set of layers. Each layer 
contains line segments that run along the boundary of the polygon and its 
holes. These segments are described by their length and maximum overlap 
amongst other segments in the same layer.

The second line of the `.dat` file contains these pairs of parameters, formatted 
as follows:

```
<number of parameter pairs> <layer length 1> <layer overlap 1> <layer length 2> <layer overlap 2> ... <layer length k> <layer overlap k>
```

Note that the spacing restrictions are not present in this line. Parameters and
the parameter count at the start of the line should be separated by a single 
space.

When determining the layer length and overlap, the following restrictions must
be followed:
- The layer length must be greater than 0 and less than the length of the 
  shortest edge in the polygon or any of its holes.
- The layer overlap must be greater than 0 and less than the layer length.

Sufficient layers of increasing length and overlap must be provided to ensure
the environment is thoroughly discretized. Target anywhere from 5 to 20 layers
depending on the size and complexity of the environment.

## Step 3: Specify the Robot

The robot is defined by its maximum rotational error and its radius. The third 
line of the `.dat` file contains these two parameters, formatted as follows:

```
<robot turning error> <robot radius>
```

Just like in step 2, each parameter should be separated by a single space. The 
robot turning error must be greater than 0 and less than 180 degrees and is 
specified in degrees. The robot radius must be greater than 0 and less than the 
length of the shortest edge in the polygon or any of its holes.

## Step 4: Generate the `.dat` File

Generated a `.dat` file containing the three lines described above in the 
specified order. Place the file in the `test_data` directory of the repository.
The file should be given a descriptive name that reflects the environment and
discretization layer count. This name should be no more than 7 words long, be
in all lowercase, and use underscores to separate words. 

For example, a file describing a polygonal environment with no holes and 10 
layers could be named `polygon_no_holes_10_layers.dat`.

## Step 5: Validate the `.dat` File

The `validate_parameters` tool can be used to validate the generated `.dat` 
file.

### Step 5a: Build the Makefiles

The Makefiles used to build the `validate_parameters` tool can be built with 
the following command:

```sh
cmake -S . -B build
```

### Step 5b: Compile the Tool

The `validate_parameters` tool can be compiled with the following command:

```sh
cmake --build build --target validate_parameters
```

### Step 5c: Run the Tool

The tool can be ran by routing the contents of the `.dat` file to its standard 
input with the following command:

```sh
./build/validate_parameters < <input_file>
```

Where `<input_file>` is the path to the `.dat` file to be validated. The tool
will output a string containing the line number and description of the first 
error it encounters in the file and return a non-zero exit code. If the file is
valid, the tool outputs nothing and returns a zero exit code.

### Step 6: Revise the `.dat` File

This step should ONLY be performed if the `validate_parameters` tool outputs an 
error. Revise the `.dat` file to correct the error and repeat steps 5a through 
5c until the file is valid.
