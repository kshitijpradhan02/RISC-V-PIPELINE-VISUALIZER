![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

RISC-V 5-Stage Pipelined Processor Simulator
Project Overview
The RISC-V Instruction Set Architecture (ISA) is an open, extensible, and modular ISA based on reduced instruction set computing (RISC) principles. It was designed to provide a clean, simple, and flexible specification that supports a wide range of computing systems, from embedded devices to high-performance processors. RISC-V defines a small base integer instruction set with optional standard and custom extensions, enabling scalability without architectural complexity. Its openness allows academic, industrial, and commercial adoption without licensing restrictions. By separating the ISA from implementation details, RISC-V promotes innovation, portability, and long-term architectural stability across software and hardware ecosystems. The RISC-V pipeline is an execution approach that overlaps multiple instructions to improve processor throughput and overall performance. Instead of completing one instruction before starting the next, pipelining allows different instructions to be processed concurrently at different points in execution, thereby increasing instruction-level parallelism. This technique significantly reduces the average cycles per instruction and enables higher clock utilization without increasing instruction complexity. In RISC-V, pipelining aligns naturally with the simplicity and regularity of the ISA, making it easier to implement efficient and predictable execution flows. Pipelining also supports higher instruction bandwidth, allowing the processor to sustain continuous instruction execution under normal conditions. However, overlapping execution introduces challenges such as data dependencies and control flow changes, which must be managed to maintain correctness.

Visual Tour
The Control Center:
This is where the user writes assembly code and interacts with the ‘Step Pipeline’ button. Control Center

Pipeline Dynamics:
Notice how each stage (IF, ID, EX, MEM, WB) and hazard resolution(STALL) has a unique color, helping the user track an instruction from start to finish. Pipeline Dynamics

The Hardware State:
A live view of the "Registers" (internal high-speed storage) and "Data Memory" (long-term storage). Hardware State

The Architecture
A classic RISC-V processor is commonly organized as a five-stage pipeline to enable efficient instruction execution while maintaining architectural simplicity.

Instruction Fetch (IF):
This stage is responsible for reading the next instruction from instruction memory using the program counter (PC). The PC is then updated, typically to the next sequential address, while supporting redirection in case of control flow changes.

Instruction Decode (ID):
This stage interprets the fetched instruction. During this stage, the opcode and fields are decoded, the register file is read, and immediate values are generated. Control signals required for later stages are also produced, and basic hazard detection is performed.

Execute (EX):
In this stage arithmetic and logical operations are carried out by the ALU. This stage also evaluates branch conditions and computes effective addresses for load and store instructions.

Memory Access (MEM):
This stage interacts with data memory. Load instructions read data from memory, while store instructions write data to memory. Instructions that do not require memory access simply pass through this stage.

Write-Back (WB):
This stage updates the destination register with results from the ALU or data memory, completing instruction execution.

Pipeline Hazards
In an ideal world, one instruction would enter the pipeline every clock cycle. However, because instructions overlap, they often "bump" into each other. These are called Hazards. This simulator handles the two most critical types:

1. Data Hazards (Read-After-Write)
A data hazard occurs when an instruction needs the result of a previous instruction that hasn't finished yet. The Example: ADD x1, x2, x3 (Calculates x1, but doesn't save it until Stage 5), SUB x4, x1, x5 (Needs x1 immediately in Stage 3). The Resolution (Forwarding): Instead of waiting for Stage 5, this simulator implements Forwarding. It "snatches" the result from the end of the EX or MEM stage and wires it directly back to the ALU input, allowing the CPU to keep running at full speed.

2. Load-Use Hazards
This is a "hard" hazard. When you load data from memory (LW), the data isn't available until the very end of the MEM stage. If the next instruction needs that data immediately, even forwarding can't save us. The Example: LW x1, 0(x2) (Data only ready at the end of Stage 4). ADD x3, x1, x4 (Needs data at the start of Stage 3). The Resolution (Stalling): The simulator detects this conflict and performs a Stall. It pauses the first two stages and injects a "Bubble" (a NOP or No-Operation) into the pipeline. This acts as a "wait" command, pushing the second instruction back by one cycle until the data is ready. Forwarding

Instruction Set Support
The simulator supports the core RISC-V instructions required for basic computation and memory manipulation:

R-Type (Register):
ADD, SUB, AND, OR. These perform math using only the 32 internal registers.

I-Type (Immediate/Load):
ADDI (Add a constant) and LW (Load from memory).

S-Type (Store):
SW (Store to memory).

Future Plans
1. Expanded Instruction Set Architecture (ISA)
While the current version supports the base RV32I instructions, expanding the supported modules will allow the simulator to run more complex software: M-Extension (Multiplication/Division): Implementing hardware-level integer multiplication and division logic. C-Extension (Compressed Instructions): Adding support for 16-bit instructions to demonstrate how code density affects fetch bandwidth and cache performance. Floating-Point Support (F/D Extensions): Integrating a separate Floating-Point Unit (FPU) and a set of 32 floating-point registers.

2. Advanced Microarchitecture Features
To move closer to modern performance standards, several hardware-level optimizations are on the roadmap: Branch Prediction: Moving beyond simple stalls to implement Dynamic Branch Prediction (using a Branch Target Buffer) to minimize the "control hazard" penalty. Cache Hierarchy: Introducing a Level 1 (L1) Instruction and Data Cache. This will allow users to visualize Cache Misses and the resulting pipeline stalls. Out-of-Order Execution: Implementing a "Scoreboard" or "Tomasulo’s Algorithm" to allow instructions to bypass each other when they don't have data dependencies.

3. Enhanced Visualization & Debugging
The user interface will be updated to provide deeper insights into the "soul" of the machine: Waveform Viewer: Integrating a logic-analyzer style view (similar to GTKWave) to show signal transitions over time. Pipeline Gantt Chart: A visual timeline showing exactly where every instruction is across multiple clock cycles. Performance Metrics Dashboard: Real-time calculation of CPI (Cycles Per Instruction), IPC (Instructions Per Cycle), and branch accuracy percentages.

4. Software Ecosystem Integration
ELF Loader: Currently, the simulator uses manual assembly entry. A planned update will allow the loading of compiled .elf files directly from standard RISC-V GCC toolchains. Virtual Peripherals: Adding a memory-mapped UART (for text output) or a simple VGA buffer to allow the simulated CPU to "talk" to the outside world. Why C++ and Qt? C++17: This was chosen for the "Hardware Logic." C++ allows for low-level bit manipulation and precise control over data structures, which is essential when trying to mimic the physical gates and wires of a processor. Qt 6: This powers the "Visuals." Qt is a professional-grade framework that allows us to turn complex C++ variables into a modern, responsive GUI. It uses a "Signal and Slot" system to ensure that when a register changes in the C++ backend, the UI updates instantly.

Installation & Usage
1. Requirements
Qt Creator: The Integrated Development Environment (IDE) used to build the project. C++ Compiler: (GCC, Clang, or MSVC) to turn the code into an executable.
