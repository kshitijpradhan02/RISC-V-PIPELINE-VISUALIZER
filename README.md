
---

# **RISC-V 5-Stage Pipelined Processor Simulator**

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## **Project Overview**

The **RISC-V Instruction Set Architecture (ISA)** is an open, extensible, and modular ISA based on Reduced Instruction Set Computing (RISC) principles. It was designed to provide a clean and flexible specification capable of scaling from embedded systems to high-performance processors. RISC-V defines a compact base integer instruction set with optional standard and custom extensions, enabling architectural growth without unnecessary complexity.

By separating the ISA from implementation details, RISC-V encourages innovation, portability, and long-term stability across software and hardware ecosystems.

This project implements a **cycle-accurate simulation of a classic 5-stage RISC-V pipelined processor**, focusing on instruction-level parallelism, hazard handling, and real-time visualization.

---

## **Pipeline Concept**

Pipelining overlaps instruction execution to improve throughput and hardware utilization. Instead of executing one instruction at a time, multiple instructions are processed simultaneously across different stages of the pipeline. This reduces the average **Cycles Per Instruction (CPI)** and increases performance without increasing instruction complexity.

While pipelining improves efficiency, it introduces **data and control hazards**, which must be resolved to maintain correctness. This simulator visualizes and resolves those hazards dynamically.

---

## **Visual Tour**



### **Control Center**
<img width="1920" height="1080" alt="control_centre" src="https://github.com/user-attachments/assets/ea667941-c998-48e7-83cb-bea8d76467ec"<img width="1920" height="1080" alt="pipeline_dynamics" src="https://github.com/user-attachments/assets/1550ce2b-1728-4bb1-8c78-192f069ead8a" />
 />
The interface where users write RISC-V assembly code and advance execution using the **Step Pipeline** button.

### **Pipeline Dynamics**
![Uploading pipeline_dynamics.png…]()


Each pipeline stage—**IF, ID, EX, MEM, WB**—is displayed with a unique color. Hazards such as **STALLs** are clearly highlighted, allowing users to follow instructions as they flow through the pipeline.

### **Hardware State**

A real-time view of:

* **Register File** (32 general-purpose registers)
* **Data Memory**

<img width="1920" height="1080" alt="hardware_state" src="https://github.com/user-attachments/assets/ffcdde71-606d-44cb-b39d-51e051b46f37" />

---

## **Processor Architecture**

The simulator models a traditional **five-stage RISC-V pipeline**:

### **1. Instruction Fetch (IF)**

* Fetches the instruction from instruction memory using the Program Counter (PC)
* Updates the PC for sequential execution or control flow changes

### **2. Instruction Decode (ID)**

* Decodes the instruction fields and opcode
* Reads source registers
* Generates immediate values
* Produces control signals
* Performs basic hazard detection

### **3. Execute (EX)**

* Performs arithmetic and logical operations
* Evaluates branch conditions
* Computes effective addresses for memory access

### **4. Memory Access (MEM)**

* Reads from or writes to data memory
* Non-memory instructions pass through unchanged

### **5. Write-Back (WB)**

* Writes ALU or memory results back to the destination register

---

## **Pipeline Hazards**

In an ideal pipeline, one instruction enters every cycle. In practice, overlapping execution causes **hazards**.

### **1. Data Hazards (Read-After-Write)**

**Example**

```
ADD x1, x2, x3
SUB x4, x1, x5
```

The `SUB` instruction requires `x1` before the `ADD` instruction reaches the write-back stage.

**Resolution — Forwarding**
The simulator implements **data forwarding**, routing results directly from the EX or MEM stage back to the ALU input, eliminating unnecessary stalls.

---



### **2. Load-Use Hazards**

**Example**

```
LW  x1, 0(x2)
ADD x3, x1, x4
```

The loaded value is only available at the end of the MEM stage, making forwarding insufficient.

<img width="1920" height="1080" alt="stalling" src="https://github.com/user-attachments/assets/3f0fb29e-0965-4f03-8ee6-98d63c82467e" />
**Resolution — Stalling**
The simulator detects the conflict and:

* Freezes the IF and ID stages
* Injects a **pipeline bubble (NOP)**
* Resumes execution once the data becomes available

---

## **Instruction Set Support**

The simulator currently supports a subset of **RV32I** instructions:

### **R-Type**

* `ADD`
* `SUB`
* `AND`
* `OR`

### **I-Type**

* `ADDI`
* `LW`

### **S-Type**

* `SW`

---

## **Future Plans**

### **1. Expanded ISA Support**

* **M Extension**: Integer multiplication and division
* **C Extension**: Compressed 16-bit instructions
* **F/D Extensions**: Floating-point execution and registers

### **2. Advanced Microarchitecture**

* Dynamic **Branch Prediction**
* **L1 Instruction and Data Caches**
* **Out-of-Order Execution** (Scoreboarding / Tomasulo)

### **3. Visualization & Debugging**

* Waveform viewer (logic-analyzer style)
* Pipeline Gantt chart
* Performance metrics (CPI, IPC, branch accuracy)

### **4. Software Ecosystem**

* ELF file loader (RISC-V GCC toolchain support)
* Memory-mapped peripherals (UART, VGA)

---

## **Why C++ and Qt**

### C++17

Used for hardware modeling, allowing precise control over bit-level behavior, timing, and data structures that closely resemble real processor logic.

### **Qt 6**

Provides a modern GUI framework. Qt’s **signal-slot mechanism** ensures real-time synchronization between the simulation backend and the visual interface.

---

## **Installation & Usage**

### **Requirements**

* **Qt Creator**
* **C++ Compiler** (GCC, Clang, or MSVC)

---

If you want, I can also:

* Convert this into a **`README.md` file**
* Add **screenshots placeholders**
* Write a **project abstract** for reports
* Tighten it for **academic submission**

