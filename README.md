![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

# RISC-V-PIPELINE-VISUALIZER-USING-C++
Interactive Qt-based RISC-V 5-stage pipeline simulator with real-time visualization of instruction flow, register/memory state, and load-use hazard detection.
RISC-V Pipeline Visualizer is a Qt-based interactive simulator for a classic 5-stage RISC-V pipeline. It allows you to step through execution cycle by cycle and visually observe how instructions move through the pipeline, how registers and memory change, and how hazards occur and are resolved.

This tool is intended for students, educators, and anyone learning computer architecture who wants a concrete way to understand pipeline behavior.

Overview

Understanding pipelined processors can be difficult when everything happens conceptually at once. This simulator breaks execution into clock cycles and shows exactly what happens in each pipeline stage:

Instruction Fetch (IF)

Instruction Decode (ID)

Execute (EX)

Memory Access (MEM)

Write Back (WB)

You write simple RISC-V assembly code, press a STEP button, and watch instructions advance through the pipeline one cycle at a time.

Features

Interactive, cycle-by-cycle simulation

Visual display of all five pipeline stages

Live register file view (x0 through x31)

Live memory view

Automatic detection of load-use hazards

Stall insertion with clear hazard messages

Program counter and cycle counter tracking

Simple and beginner-friendly instruction set

Supported Instructions

The simulator currently supports the following RISC-V instructions:

Load Word

lw xd, imm(xs1)
Loads a word from memory at address xs1 + imm into register xd.

Example:

lw x1, 0(x0)

Store Word

sw xs2, imm(xs1)
Stores the value in xs2 to memory at address xs1 + imm.
Example:
sw x2, 4(x1)
Add
add xd, xs1, xs2
Adds the values in xs1 and xs2 and stores the result in xd.
Example:
add x3, x1, x2

These instructions are sufficient to demonstrate basic pipeline execution and data hazards.
Getting Started
Requirements
CMake 3.16 or newer
Qt 6 (Core, Gui, and Widgets modules)

A C++17 compatible compiler (GCC, Clang, or MSVC)

Windows, macOS, or Linux

Building the Project

Clone the repository and build it using CMake:

git clone https://github.com/your-username/your-repo.git
cd your-repo

mkdir build
cd build

cmake ..
cmake --build . --config Release


On Linux or macOS, you may also use:

make -j$(nproc)


After building, the executable will be named RISCVPipeline (or RISCVPipeline.exe on Windows).

Running the Simulator

From the build directory:

./RISCVPipeline


A graphical window will open containing:

A text editor for RISC-V assembly code

A STEP button

Tables showing pipeline stages, registers, and memory

How to Use

Enter your RISC-V program in the text editor. Example:

lw x1, 0(x0)
lw x2, 4(x0)
add x3, x1, x2
sw x3, 8(x0)


Click the STEP button to advance the simulation by one clock cycle.

Observe:

Instructions moving through pipeline stages

Register values updating during write-back

Memory updates during store instructions

The program counter and cycle counter changing

Continue stepping until all pipeline stages show nop. At this point, the program has fully executed.

Hazard Handling

The simulator automatically detects load-use hazards.

Example:

lw x1, 0(x0)
add x2, x1, x1


In this case, the add instruction depends on the result of the lw. The simulator inserts a stall, displays a hazard warning, and shows the bubble in the pipeline.

This behavior matches how a real pipelined processor handles such hazards.

Internal Design

The simulator maintains:

32 general-purpose registers (x0 to x31, with x0 hardwired to zero)

A memory array with predefined initial values

Five pipeline stage registers, each with an instruction and valid flag

A program counter pointing to the next instruction

On each cycle:

Write-back updates registers

Memory stage performs loads and stores

Hazard detection checks for load-use conflicts

Instructions advance through the pipeline

A new instruction is fetched if possible

Project Structure
.
├── CMakeLists.txt       Build configuration
├── main.cpp             Application entry point
├── mainwindow.h/.cpp    Qt GUI implementation
├── pipeline.h/.cpp      Pipeline simulation logic
└── README.md            Documentation


The pipeline module handles instruction decoding, execution, and hazard detection.
The main window module handles visualization and user interaction.

Future Improvements

Planned or possible enhancements include:

Additional instructions (sub, and, or, branches, jumps)

Forwarding support

Configurable initial register and memory values

Automatic run mode

Execution trace export

Unit tests for pipeline behavior

Contributing

Contributions are welcome.

Fork the repository

Create a feature branch

Make and test your changes

Submit a pull request

License

This project is licensed under the MIT License. See the LICENSE file for details.
