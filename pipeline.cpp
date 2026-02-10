#include "pipeline.h"      // <-- All structs/classes now come from here

#include <array>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <algorithm> // <-- added
#include <cctype>    // <-- added

using namespace std;

// ----------------------
// RegisterFile methods
// ----------------------
RegisterFile::RegisterFile() {
    regs.fill(0);
}

int32_t RegisterFile::read(uint8_t reg) const {
    if (reg > 31) throw out_of_range("read: reg index out of range");
    return regs[reg];
}

void RegisterFile::write(uint8_t reg, int32_t value) {
    if (reg == 0) return; // x0 is hardwired to zero
    if (reg > 31) throw out_of_range("write: reg index out of range");
    regs[reg] = value;
}

array<int32_t, 32> RegisterFile::snapshot() const {
    return regs;
}

void RegisterFile::dump(ostream &os) const {
    os << "Register file:\n";
    for (int i = 0; i < 32; ++i) {
        os << "x" << setw(2) << i << ": "
           << setw(11) << regs[i];
        if (i % 4 == 3) os << "\n";
        else os << "\t";
    }
}

// ----------------------
// Memory methods
// ----------------------
Memory::Memory(size_t bytes) {
    data.resize(bytes, 0);
}

void Memory::sw(uint32_t address, int32_t value) {
    if (address % 4 != 0) throw runtime_error("sw: unaligned address");
    size_t idx = static_cast<size_t>(address);
    if (idx + 4 > data.size()) throw out_of_range("sw: out of range");
    data[idx]     = static_cast<uint8_t>(value & 0xFF);
    data[idx + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[idx + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[idx + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

int32_t Memory::lw(uint32_t address) const {
    if (address % 4 != 0) throw runtime_error("lw: unaligned address");
    size_t idx = static_cast<size_t>(address);
    if (idx + 4 > data.size()) throw out_of_range("lw: out of range");
    uint32_t v = 0;
    v |= static_cast<uint32_t>(data[idx]);
    v |= static_cast<uint32_t>(data[idx + 1]) << 8;
    v |= static_cast<uint32_t>(data[idx + 2]) << 16;
    v |= static_cast<uint32_t>(data[idx + 3]) << 24;
    return static_cast<int32_t>(v);
}

void Memory::dumpWords(ostream &os, uint32_t startAddr, uint32_t words) const {
    os << "Memory dump (word aligned):\n";
    for (uint32_t i = 0; i < words; ++i) {
        uint32_t addr = startAddr + i * 4;
        if (static_cast<size_t>(addr) + 4 > data.size()) break;
        os << "0x" << hex << setw(8) << setfill('0') << addr << ": "
           << dec << lw(addr) << "\n";
    }
    os << setfill(' ');
}

PipelineSimulator::PipelineSimulator(size_t memBytes)
    : mem(memBytes)
{}

// ------------------ loadProgram ------------------
void PipelineSimulator::loadProgram(const vector<string>& program, uint32_t startPc) {
    instr_mem = program;
    pc = startPc;
}

// ------------------ fetch_instruction ------------------
void PipelineSimulator::fetch_instruction(const string &instr, uint32_t pc_val) {
    if_id.valid = true;
    if_id.pc = pc_val;
    if_id.instr_str = instr;
}

// ------------------ decode ------------------
void PipelineSimulator::decode() {
    if (!if_id.valid) {
        id_ex.valid = false;
        return;
    }

    id_ex = ID_EX();
    id_ex.valid = true;
    id_ex.pc = if_id.pc;
    id_ex.instr_str = if_id.instr_str;

    const string &s = if_id.instr_str;

    // make opcode check tolerant: trim leading spaces and uppercase opcode token
    string tmp = s;
    // trim leading whitespace
    tmp.erase(0, tmp.find_first_not_of(" \t\r\n"));
    // extract first token (opcode)
    string op;
    size_t pos = tmp.find_first_of(" \t");
    if (pos == string::npos) op = tmp;
    else op = tmp.substr(0, pos);
    // uppercase op
    transform(op.begin(), op.end(), op.begin(), [](unsigned char c){ return (char)toupper(c); });

    if (op == "LW") {
        uint32_t rd = 0, rs = 0, imm = 0;
        if (parseLoadStore(s, rd, rs, imm)) {
            id_ex.rd = rd;
            id_ex.rs1 = rs;
            id_ex.imm = imm;
            id_ex.rs1_val = regs.read(id_ex.rs1);
            id_ex.memRead = true;
            id_ex.regWrite = true;
            id_ex.memToReg = true;
            id_ex.useImm = true;
            id_ex.aluOp = ID_EX::ALU_ADD;
        } else {
            id_ex.valid = false;
        }
    }
    else if (op == "SW") {
        uint32_t rs = 0, base = 0, imm = 0;
        if (parseStore(s, rs, base, imm)) {
            id_ex.rs1 = base;
            id_ex.rs2 = rs;
            id_ex.imm = imm;
            id_ex.rs1_val = regs.read(id_ex.rs1);
            id_ex.rs2_val = regs.read(id_ex.rs2);
            id_ex.memWrite = true;
            id_ex.useImm = true;
            id_ex.aluOp = ID_EX::ALU_ADD;
        } else {
            id_ex.valid = false;
        }
    }
    else {
        id_ex.valid = false;
    }

    if_id.valid = false;   // advance
}

// ------------------ execute ------------------
void PipelineSimulator::execute() {
    if (!id_ex.valid) {
        ex_mem.valid = false;
        return;
    }

    ex_mem = EX_MEM();
    ex_mem.valid = true;
    ex_mem.pc = id_ex.pc;
    ex_mem.instr_str = id_ex.instr_str;
    ex_mem.rd = id_ex.rd;
    ex_mem.memRead = id_ex.memRead;
    ex_mem.memWrite = id_ex.memWrite;
    ex_mem.regWrite = id_ex.regWrite;
    ex_mem.memToReg = id_ex.memToReg;

    int32_t a = id_ex.rs1_val;
    int32_t b = id_ex.useImm ? id_ex.imm : id_ex.rs2_val;
    int32_t res = 0;

    switch (id_ex.aluOp) {
    case ID_EX::ALU_ADD: res = a + b; break;
    case ID_EX::ALU_SUB: res = a - b; break;
    case ID_EX::ALU_AND: res = a & b; break;
    case ID_EX::ALU_OR:  res = a | b; break;
    case ID_EX::ALU_XOR: res = a ^ b; break;
    case ID_EX::ALU_SLT: res = (a < b); break;
    case ID_EX::ALU_SLL: res = (uint32_t)a << (b & 31); break;
    case ID_EX::ALU_SRL: res = (uint32_t)a >> (b & 31); break;
    default: break;
    }

    ex_mem.alu_result = res;
    ex_mem.rs2_val = id_ex.rs2_val;
}

// ------------------ mem_access ------------------
void PipelineSimulator::mem_access() {
    if (!ex_mem.valid) {
        mem_wb.valid = false;
        return;
    }

    mem_wb = MEM_WB();
    mem_wb.valid = true;
    mem_wb.pc = ex_mem.pc;
    mem_wb.instr_str = ex_mem.instr_str;
    mem_wb.rd = ex_mem.rd;
    mem_wb.regWrite = ex_mem.regWrite;
    mem_wb.memToReg = ex_mem.memToReg;
    mem_wb.alu_result = ex_mem.alu_result;

    if (ex_mem.memRead) {
        mem_wb.mem_data = mem.lw(ex_mem.alu_result);
    }

    if (ex_mem.memWrite) {
        mem.sw(ex_mem.alu_result, ex_mem.rs2_val);
    }
}

// ------------------ writeback ------------------
void PipelineSimulator::writeback() {
    if (!mem_wb.valid) return;

    if (mem_wb.regWrite && mem_wb.rd != 0) {
        int32_t value = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
        regs.write(mem_wb.rd, value);
    }
}

// ------------------ snapshot ------------------
PipelineState PipelineSimulator::snapshot(int cycle, uint32_t pc_val) const {
    PipelineState s;
    s.cycle = cycle;
    s.pc = pc_val;
    s.IF_instr = if_id.valid ? if_id.instr_str : "NOP";
    s.ID_instr = id_ex.valid ? id_ex.instr_str : "NOP";
    s.EX_instr = ex_mem.valid ? ex_mem.instr_str : "NOP";
    s.MEM_instr = mem_wb.valid ? mem_wb.instr_str : "NOP";
    s.WB_instr = mem_wb.valid ? mem_wb.instr_str : "NOP";
    s.regs = regs.snapshot();
    return s;
}

// ------------------ parseLoadStore ------------------
// tolerant parsing: commas optional, case-insensitive opcode handled earlier
bool PipelineSimulator::parseLoadStore(const string &s, uint32_t &rd, uint32_t &rs, uint32_t &imm) {
    // make a local copy and normalize spaces/commas
    string t = s;
    // replace commas with spaces
    for (char &c : t) if (c == ',') c = ' ';
    // trim leading/trailing whitespace
    auto lpos = t.find_first_not_of(" \t\r\n");
    if (lpos == string::npos) return false;
    auto rpos = t.find_last_not_of(" \t\r\n");
    t = t.substr(lpos, rpos - lpos + 1);

    // now parse: expect "LW <reg> <offset>(x<base>)"
    char op[8] = {0}, regS[16] = {0}, offs[64] = {0};
    if (sscanf(t.c_str(), "%7s %15s %63s", op, regS, offs) < 3) {
        return false;
    }

    if (regS[0] != 'x' && regS[0] != 'X') return false;
    rd = static_cast<uint32_t>(atoi(regS + 1));

    unsigned int offset = 0, base = 0;
    // allow offsets like 0x100(x0) or 100(x0)
    if (sscanf(offs, "0x%x(x%u)", &offset, &base) == 2) {
        imm = offset; rs = base; return true;
    }
    if (sscanf(offs, "%u(x%u)", &offset, &base) == 2) {
        imm = offset; rs = base; return true;
    }
    // as a fallback, also allow the offset to be decimal without parentheses (unlikely but safe)
    if (sscanf(offs, "%u", &offset) == 1) {
        imm = offset; rs = 0; return true;
    }

    return false;
}

// ------------------ parseStore ------------------
// tolerant parsing: commas optional, case-insensitive opcode handled earlier
bool PipelineSimulator::parseStore(const string &s, uint32_t &rs, uint32_t &base, uint32_t &imm) {
    string t = s;
    for (char &c : t) if (c == ',') c = ' ';
    auto lpos = t.find_first_not_of(" \t\r\n");
    if (lpos == string::npos) return false;
    auto rpos = t.find_last_not_of(" \t\r\n");
    t = t.substr(lpos, rpos - lpos + 1);

    // parse: "SW <rs> <offset>(x<base>)"
    char op[8] = {0}, regS[16] = {0}, offs[64] = {0};
    if (sscanf(t.c_str(), "%7s %15s %63s", op, regS, offs) < 3) {
        return false;
    }

    if (regS[0] != 'x' && regS[0] != 'X') return false;
    rs = static_cast<uint32_t>(atoi(regS + 1));

    unsigned int offset = 0, baseReg = 0;
    if (sscanf(offs, "0x%x(x%u)", &offset, &baseReg) == 2) {
        imm = offset; base = baseReg; return true;
    }
    if (sscanf(offs, "%u(x%u)", &offset, &baseReg) == 2) {
        imm = offset; base = baseReg; return true;
    }
    if (sscanf(offs, "%u", &offset) == 1) {
        imm = offset; base = 0; return true;
    }
    return false;
}

// ------------------ step (full pipeline) ------------------
void PipelineSimulator::step() {
    IF_ID  next_if = IF_ID{};
    ID_EX  next_id = ID_EX{};
    EX_MEM next_ex = EX_MEM{};
    MEM_WB next_mem = MEM_WB{};

    // WB
    if (mem_wb.valid) {
        if (mem_wb.regWrite && mem_wb.rd != 0) {
            int32_t val = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
            regs.write(mem_wb.rd, val);
        }
    }

    // MEM
    if (ex_mem.valid) {
        next_mem.valid = true;
        next_mem.pc = ex_mem.pc;
        next_mem.instr_str = ex_mem.instr_str;
        next_mem.rd = ex_mem.rd;
        next_mem.regWrite = ex_mem.regWrite;
        next_mem.memToReg = ex_mem.memToReg;
        next_mem.alu_result = ex_mem.alu_result;

        if (ex_mem.memRead) next_mem.mem_data = mem.lw(ex_mem.alu_result);
        if (ex_mem.memWrite) mem.sw(ex_mem.alu_result, ex_mem.rs2_val);
    }

    // EX
    if (id_ex.valid) {
        next_ex.valid = true;
        next_ex.pc = id_ex.pc;
        next_ex.instr_str = id_ex.instr_str;
        next_ex.rd = id_ex.rd;
        next_ex.memRead = id_ex.memRead;
        next_ex.memWrite = id_ex.memWrite;
        next_ex.regWrite = id_ex.regWrite;
        next_ex.memToReg = id_ex.memToReg;

        int32_t a = id_ex.rs1_val;
        int32_t b = id_ex.useImm ? id_ex.imm : id_ex.rs2_val;
        int32_t res = 0;
        switch (id_ex.aluOp) {
        case ID_EX::ALU_ADD: res = a + b; break;
        case ID_EX::ALU_SUB: res = a - b; break;
        case ID_EX::ALU_AND: res = a & b; break;
        case ID_EX::ALU_OR:  res = a | b; break;
        case ID_EX::ALU_XOR: res = a ^ b; break;
        case ID_EX::ALU_SLT: res = (a < b); break;
        case ID_EX::ALU_SLL: res = (uint32_t)a << (b & 31); break;
        case ID_EX::ALU_SRL: res = (uint32_t)a >> (b & 31); break;
        default: break;
        }
        next_ex.alu_result = res;
        next_ex.rs2_val = id_ex.rs2_val;
    }

    // ID
    // --- ID: produce next_id from if_id (use same parse logic) ---
    if (if_id.valid) {
        next_id = ID_EX();
        next_id.valid = true;
        next_id.pc = if_id.pc;
        next_id.instr_str = if_id.instr_str;

        const string &s = if_id.instr_str;

        // prepare opcode in uppercase and trimmed
        string t = s;
        t.erase(0, t.find_first_not_of(" \t\r\n"));
        size_t p = t.find_first_of(" \t");
        string opcode = (p == string::npos) ? t : t.substr(0, p);
        transform(opcode.begin(), opcode.end(), opcode.begin(), [](unsigned char c){ return (char)toupper(c); });

        // handle LW (load)
        if (opcode == "LW") {
            uint32_t rd=0, rs=0, imm=0;
            if (parseLoadStore(s, rd, rs, imm)) {
                next_id.rd = rd;
                next_id.rs1 = rs;
                next_id.imm = imm;
                next_id.rs1_val = regs.read(next_id.rs1);
                next_id.memRead = true;
                next_id.regWrite = true;
                next_id.memToReg = true;
                next_id.useImm = true;
                next_id.aluOp = ID_EX::ALU_ADD;
            } else next_id.valid = false;
        }
        // handle SW (store)
        else if (opcode == "SW") {
            uint32_t r=0, base=0, imm=0;
            if (parseStore(s, r, base, imm)) {
                next_id.rs1 = base;
                next_id.rs2 = r;
                next_id.imm = imm;
                next_id.rs1_val = regs.read(next_id.rs1);
                next_id.rs2_val = regs.read(next_id.rs2);
                next_id.memWrite = true;
                next_id.useImm = true;
                next_id.aluOp = ID_EX::ALU_ADD;
            } else next_id.valid = false;
        }
        // handle simple R-type ADD: "ADD xrd, xrs1, xrs2"
        else if (opcode == "ADD") {
            // tolerant parsing: replace commas with spaces then sscanf
            string tmp = s;
            for (char &c : tmp) if (c == ',') c = ' ';
            char rdName[16] = {0}, rs1Name[16] = {0}, rs2Name[16] = {0};
            if (sscanf(tmp.c_str(), "ADD %15s %15s %15s", rdName, rs1Name, rs2Name) == 3) {
                if ((rdName[0] == 'x' || rdName[0] == 'X') &&
                    (rs1Name[0] == 'x' || rs1Name[0] == 'X') &&
                    (rs2Name[0] == 'x' || rs2Name[0] == 'X')) {

                    uint32_t rd = (uint32_t)atoi(rdName + 1);
                    uint32_t rs1 = (uint32_t)atoi(rs1Name + 1);
                    uint32_t rs2 = (uint32_t)atoi(rs2Name + 1);

                    next_id.rd = static_cast<uint8_t>(rd);
                    next_id.rs1 = static_cast<uint8_t>(rs1);
                    next_id.rs2 = static_cast<uint8_t>(rs2);
                    // read register values now (for this simple model)
                    next_id.rs1_val = regs.read(next_id.rs1);
                    next_id.rs2_val = regs.read(next_id.rs2);

                    next_id.regWrite = true;
                    next_id.useImm = false;
                    next_id.aluOp = ID_EX::ALU_ADD;
                } else {
                    next_id.valid = false;
                }
            } else {
                // parse failed -> treat as NOP
                next_id.valid = false;
            }
        }
        // unsupported instruction -> NOP in ID
        else {
            next_id.valid = false;
        }
    }

    // IF
    string fetched = "NOP";
    uint32_t idx = pc / 4;
    if (idx < instr_mem.size()) fetched = instr_mem[idx];

    next_if.valid = (fetched != "NOP");
    next_if.instr_str = fetched;
    next_if.pc = pc;

    pc += 4;

    // Commit
    if_id = next_if;
    id_ex = next_id;
    ex_mem = next_ex;
    mem_wb = next_mem;
}
