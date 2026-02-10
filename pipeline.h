#ifndef PIPELINE_H
#define PIPELINE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

using std::array;
using std::string;
using std::vector;
using std::ostream;

// ----------------------------
// Pipeline Register Structs
// ----------------------------
struct IF_ID {
    uint32_t pc = 0;
    uint32_t instr = 0;
    string instr_str = "NOP";
    bool valid = false;
};

struct ID_EX {
    uint32_t pc = 0;
    uint32_t instr = 0;
    string instr_str = "NOP";

    int32_t rs1_val = 0;
    int32_t rs2_val = 0;
    int32_t imm = 0;

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;

    bool useImm = false;
    bool memRead = false;
    bool memWrite = false;
    bool regWrite = false;
    bool memToReg = false;

    enum ALUOp_t {
        ALU_NONE = 0, ALU_ADD, ALU_SUB, ALU_AND,
        ALU_OR, ALU_XOR, ALU_SLT, ALU_SLL, ALU_SRL
    } aluOp = ALU_NONE;

    bool valid = false;
};

struct EX_MEM {
    uint32_t pc = 0;
    uint32_t instr = 0;
    string instr_str = "NOP";

    int32_t alu_result = 0;
    int32_t rs2_val = 0;
    uint8_t rd = 0;

    bool memRead = false;
    bool memWrite = false;
    bool regWrite = false;
    bool memToReg = false;

    bool valid = false;
};

struct MEM_WB {
    uint32_t pc = 0;
    uint32_t instr = 0;
    string instr_str = "NOP";

    int32_t mem_data = 0;
    int32_t alu_result = 0;
    uint8_t rd = 0;

    bool regWrite = false;
    bool memToReg = false;

    bool valid = false;
};

// ----------------------------
// Snapshot Struct
// ----------------------------
struct PipelineState {
    int cycle = 0;
    uint32_t pc = 0;

    string IF_instr = "NOP";
    string ID_instr = "NOP";
    string EX_instr = "NOP";
    string MEM_instr = "NOP";
    string WB_instr = "NOP";

    array<int32_t, 32> regs{};
};

// ----------------------------
// Register File
// ----------------------------
class RegisterFile {
public:
    RegisterFile();
    int32_t read(uint8_t reg) const;
    void write(uint8_t reg, int32_t value);
    array<int32_t, 32> snapshot() const;
    void dump(ostream &os = std::cout) const;

private:
    array<int32_t, 32> regs{};
};

// ----------------------------
// Memory
// ----------------------------
class Memory {
public:
    Memory(size_t bytes = 1024);
    void sw(uint32_t address, int32_t value);
    int32_t lw(uint32_t address) const;
    void dumpWords(ostream &os = std::cout,
                   uint32_t startAddr = 0, uint32_t words = 16) const;

private:
    vector<uint8_t> data;
};

// ----------------------------
// Pipeline Simulator
// ----------------------------
class PipelineSimulator {
public:
    PipelineSimulator(size_t memBytes = 1024);

    void loadProgram(const vector<string>& program,
                     uint32_t startPc = 0u);

    void fetch_instruction(const string &instr, uint32_t pc_val = 0);
    void decode();
    void execute();
    void mem_access();
    void writeback();

    // Single pipelined cycle
    void step();          // <-- made public so Qt can call it

    PipelineState snapshot(int cycle, uint32_t pc_val) const;

    // Public for UI visibility
    vector<string> instr_mem;
    uint32_t pc = 0;

    RegisterFile regs;
    Memory mem;

private:
    // Internal pipeline registers
    IF_ID if_id;
    ID_EX id_ex;
    EX_MEM ex_mem;
    MEM_WB mem_wb;

    // Parsers
    static bool parseLoadStore(const string &s,
                               uint32_t &rd, uint32_t &rs, uint32_t &imm);

    static bool parseStore(const string &s,
                           uint32_t &rs, uint32_t &base, uint32_t &imm);
};

#endif
