#include "pipeline.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

// ===== RegisterFile =====
RegisterFile::RegisterFile() {
    regs.fill(0);
}

int32_t RegisterFile::read(uint8_t reg) const {
    if (reg > 31) throw out_of_range("read: reg index out of range");
    return regs[reg];
}

void RegisterFile::write(uint8_t reg, int32_t value) {
    if (reg == 0) return;
    if (reg > 31) throw out_of_range("write: reg index out of range");
    regs[reg] = value;
}

array<int32_t, 32> RegisterFile::snapshot() const {
    return regs;
}

void RegisterFile::dump(ostream &os) const {
    os << "Register file:\n";
    for (int i = 0; i < 32; ++i) {
        os << "x" << setw(2) << i << ": " << setw(11) << regs[i];
        if (i % 4 == 3) os << "\n";
        else os << "\t";
    }
}

// ===== Memory =====
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

// ===== PipelineSimulator =====
PipelineSimulator::PipelineSimulator(size_t memBytes)
    : mem(memBytes) {}

void PipelineSimulator::loadProgram(const vector<string>& program, uint32_t startPc) {
    instr_mem = program;
    pc = startPc;
}

bool PipelineSimulator::parseLoadStore(const string &s, uint32_t &rd, uint32_t &rs, uint32_t &imm) {
    string t = s;
    for (char &c : t) if (c == ',') c = ' ';
    auto lpos = t.find_first_not_of(" \t\r\n");
    if (lpos == string::npos) return false;
    auto rpos = t.find_last_not_of(" \t\r\n");
    t = t.substr(lpos, rpos - lpos + 1);
    char op[8] = {0}, regS[16] = {0}, offs[64] = {0};
    if (sscanf(t.c_str(), "%7s %15s %63s", op, regS, offs) < 3) return false;
    if (regS[0] != 'x' && regS[0] != 'X') return false;
    rd = static_cast<uint32_t>(atoi(regS + 1));
    unsigned int offset = 0, base = 0;
    if (sscanf(offs, "0x%x(x%u)", &offset, &base) == 2) { imm = offset; rs = base; return true; }
    if (sscanf(offs, "%u(x%u)", &offset, &base) == 2) { imm = offset; rs = base; return true; }
    if (sscanf(offs, "%u", &offset) == 1) { imm = offset; rs = 0; return true; }
    return false;
}

bool PipelineSimulator::parseStore(const string &s, uint32_t &rs, uint32_t &base, uint32_t &imm) {
    string t = s;
    for (char &c : t) if (c == ',') c = ' ';
    auto lpos = t.find_first_not_of(" \t\r\n");
    if (lpos == string::npos) return false;
    auto rpos = t.find_last_not_of(" \t\r\n");
    t = t.substr(lpos, rpos - lpos + 1);
    char op[8] = {0}, regS[16] = {0}, offs[64] = {0};
    if (sscanf(t.c_str(), "%7s %15s %63s", op, regS, offs) < 3) return false;
    if (regS[0] != 'x' && regS[0] != 'X') return false;
    rs = static_cast<uint32_t>(atoi(regS + 1));
    unsigned int offset = 0, baseReg = 0;
    if (sscanf(offs, "0x%x(x%u)", &offset, &baseReg) == 2) { imm = offset; base = baseReg; return true; }
    if (sscanf(offs, "%u(x%u)", &offset, &baseReg) == 2) { imm = offset; base = baseReg; return true; }
    if (sscanf(offs, "%u", &offset) == 1) { imm = offset; base = 0; return true; }
    return false;
}

bool PipelineSimulator::parseRType(const string &s, string &op, uint32_t &rd, uint32_t &rs1, uint32_t &rs2) {
    string t = s;
    for (char &c : t) if (c == ',') c = ' ';
    auto lpos = t.find_first_not_of(" \t\r\n");
    if (lpos == string::npos) return false;
    auto rpos = t.find_last_not_of(" \t\r\n");
    t = t.substr(lpos, rpos - lpos + 1);

    char opName[16] = {0}, rd_r[16] = {0}, rs1_r[16] = {0}, rs2_r[16] = {0};
    if (sscanf(t.c_str(), "%15s %15s %15s %15s", opName, rd_r, rs1_r, rs2_r) < 4) return false;

    op = opName;
    transform(op.begin(), op.end(), op.begin(), [](unsigned char c){ return (char)toupper(c); });

    if (rd_r[0] != 'x' || rs1_r[0] != 'x' || rs2_r[0] != 'x') return false;
    rd = static_cast<uint32_t>(atoi(rd_r + 1));
    rs1 = static_cast<uint32_t>(atoi(rs1_r + 1));
    rs2 = static_cast<uint32_t>(atoi(rs2_r + 1));
    return true;
}

// ===== HAZARD DETECTION (const versions) =====
bool PipelineSimulator::checkLoadUseHazard() const {
    // RAW Hazard: Load in ID, use in next stage (EX needs operand from load result)
    if (id_ex.valid && id_ex.memRead && id_ex.rd != 0) {
        if (if_id.valid) {
            string t = if_id.instr_str;
            for (char &c : t) if (c == ',') c = ' ';
            auto lpos = t.find_first_not_of(" \t\r\n");
            if (lpos != string::npos) {
                auto rpos = t.find_last_not_of(" \t\r\n");
                t = t.substr(lpos, rpos - lpos + 1);
                char op[16] = {0};
                sscanf(t.c_str(), "%15s", op);
                string opcode(op);
                transform(opcode.begin(), opcode.end(), opcode.begin(),
                          [](unsigned char c){ return (char)toupper(c); });

                uint32_t rd = 0, rs1 = 0, rs2 = 0, imm = 0;
                if (opcode == "LW") {
                    if (const_cast<PipelineSimulator*>(this)->parseLoadStore(t, rd, rs1, imm)) {
                        if (rs1 == id_ex.rd) return true;
                    }
                } else if (opcode == "SW") {
                    if (const_cast<PipelineSimulator*>(this)->parseStore(t, rs2, rs1, imm)) {
                        if (rs1 == id_ex.rd || rs2 == id_ex.rd) return true;
                    }
                } else if (opcode == "ADD" || opcode == "SUB" || opcode == "AND" ||
                           opcode == "OR" || opcode == "XOR" || opcode == "SLT" ||
                           opcode == "SLL" || opcode == "SRL") {
                    if (const_cast<PipelineSimulator*>(this)->parseRType(t, opcode, rd, rs1, rs2)) {
                        if ((rs1 == id_ex.rd && rs1 != 0) || (rs2 == id_ex.rd && rs2 != 0)) return true;
                    }
                }
            }
        }
    }
    return false;
}

bool PipelineSimulator::checkStructuralHazard() const {
    // Multiple writes to same register simultaneously
    int writes = 0;
    uint8_t writeReg = 255;
    if (ex_mem.valid && ex_mem.regWrite && ex_mem.rd != 0) {
        writes++;
        writeReg = ex_mem.rd;
    }
    if (mem_wb.valid && mem_wb.regWrite && mem_wb.rd != 0) {
        if (mem_wb.rd == writeReg) return true;
        writes++;
    }
    return writes > 1;
}

void PipelineSimulator::detectHazards() {
    has_stall = false;
    if (checkLoadUseHazard()) {
        has_stall = true;
        stall_cycles++;
    }
}

// ===== PIPELINE EXECUTION =====
void PipelineSimulator::step() {
    detectHazards();

    IF_ID next_if = IF_ID{};
    ID_EX next_id = ID_EX{};
    EX_MEM next_ex = EX_MEM{};
    MEM_WB next_mem = MEM_WB{};

    // ===== WB Stage =====
    if (mem_wb.valid) {
        if (mem_wb.regWrite && mem_wb.rd != 0) {
            int32_t val = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
            regs.write(mem_wb.rd, val);
        }
    }

    // ===== MEM Stage =====
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

    // ===== EX Stage (with DATA FORWARDING) =====
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

        // Forward from EX/MEM
        if (ex_mem.valid && ex_mem.regWrite && ex_mem.rd != 0) {
            if (ex_mem.rd == id_ex.rs1) { a = ex_mem.alu_result; }
            if (!id_ex.useImm && ex_mem.rd == id_ex.rs2) { b = ex_mem.alu_result; }
        }

        // Forward from MEM/WB
        if (mem_wb.valid && mem_wb.regWrite && mem_wb.rd != 0) {
            int32_t fwd = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
            if (mem_wb.rd == id_ex.rs1) { a = fwd; }
            if (!id_ex.useImm && mem_wb.rd == id_ex.rs2) { b = fwd; }
        }

        int32_t res = 0;
        switch (id_ex.aluOp) {
        case ID_EX::ALU_ADD: res = a + b; break;
        case ID_EX::ALU_SUB: res = a - b; break;
        case ID_EX::ALU_AND: res = a & b; break;
        case ID_EX::ALU_OR:  res = a | b; break;
        case ID_EX::ALU_XOR: res = a ^ b; break;
        case ID_EX::ALU_SLT: res = (a < b ? 1 : 0); break;
        case ID_EX::ALU_SLL: res = (int32_t)((uint32_t)a << (b & 31)); break;
        case ID_EX::ALU_SRL: res = (int32_t)((uint32_t)a >> (b & 31)); break;
        default: break;
        }
        next_ex.alu_result = res;
        next_ex.rs2_val = id_ex.rs2_val;
    }

    // ===== ID Stage (with STALL on hazard) =====
    if (has_stall) {
        // STALL: keep IF/ID same, bubble to EX
        next_if = if_id;
        next_id = id_ex;
        next_ex.valid = false;
    } else if (if_id.valid) {
        next_id.valid = true;
        next_id.pc = if_id.pc;
        next_id.instr_str = if_id.instr_str;

        const string &s = if_id.instr_str;
        string t = s;
        t.erase(0, t.find_first_not_of(" \t\r\n"));
        size_t p = t.find_first_of(" \t");
        string opcode = (p == string::npos) ? t : t.substr(0, p);
        transform(opcode.begin(), opcode.end(), opcode.begin(),
                  [](unsigned char c){ return (char)toupper(c); });

        // ===== LW Instruction =====
        if (opcode == "LW") {
            uint32_t rd = 0, rs = 0, imm = 0;
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
        // ===== SW Instruction =====
        else if (opcode == "SW") {
            uint32_t r = 0, base = 0, imm = 0;
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
        // ===== R-Type Instructions (ADD, SUB, AND, OR, XOR, SLT, SLL, SRL) =====
        else if (opcode == "ADD" || opcode == "SUB" || opcode == "AND" ||
                 opcode == "OR" || opcode == "XOR" || opcode == "SLT" ||
                 opcode == "SLL" || opcode == "SRL") {
            uint32_t rd = 0, rs1 = 0, rs2 = 0;
            if (parseRType(s, opcode, rd, rs1, rs2)) {
                next_id.rd = rd;
                next_id.rs1 = rs1;
                next_id.rs2 = rs2;
                next_id.rs1_val = regs.read(next_id.rs1);
                next_id.rs2_val = regs.read(next_id.rs2);
                next_id.regWrite = true;
                next_id.useImm = false;

                if (opcode == "ADD") next_id.aluOp = ID_EX::ALU_ADD;
                else if (opcode == "SUB") next_id.aluOp = ID_EX::ALU_SUB;
                else if (opcode == "AND") next_id.aluOp = ID_EX::ALU_AND;
                else if (opcode == "OR") next_id.aluOp = ID_EX::ALU_OR;
                else if (opcode == "XOR") next_id.aluOp = ID_EX::ALU_XOR;
                else if (opcode == "SLT") next_id.aluOp = ID_EX::ALU_SLT;
                else if (opcode == "SLL") next_id.aluOp = ID_EX::ALU_SLL;
                else if (opcode == "SRL") next_id.aluOp = ID_EX::ALU_SRL;
            } else next_id.valid = false;
        }
        // ===== NOP =====
        else if (opcode == "NOP") {
            next_id.valid = false;
        }
        // ===== Unknown/Invalid =====
        else {
            next_id.valid = false;
        }

        // ===== IF Stage (advance PC) =====
        string fetched = "NOP";
        uint32_t idx = pc / 4;
        if (idx < instr_mem.size()) fetched = instr_mem[idx];
        next_if.valid = (fetched != "NOP");
        next_if.instr_str = fetched;
        next_if.pc = pc;
        pc += 4;
    }

    // Update pipeline registers
    if_id = next_if;
    id_ex = next_id;
    ex_mem = next_ex;
    mem_wb = next_mem;
}

// ===== SNAPSHOT & STATE CAPTURE =====
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

    // Hazard detection flags
    s.load_use_hazard = has_stall;
    s.structural_hazard = checkStructuralHazard();

    // Check for data forwarding happening
    s.data_forward_rs1 = false;
    s.data_forward_rs2 = false;
    if (id_ex.valid && id_ex.rs1 != 0) {
        if ((ex_mem.valid && ex_mem.regWrite && ex_mem.rd == id_ex.rs1) ||
            (mem_wb.valid && mem_wb.regWrite && mem_wb.rd == id_ex.rs1)) {
            s.data_forward_rs1 = true;
        }
    }
    if (id_ex.valid && id_ex.rs2 != 0 && !id_ex.useImm) {
        if ((ex_mem.valid && ex_mem.regWrite && ex_mem.rd == id_ex.rs2) ||
            (mem_wb.valid && mem_wb.regWrite && mem_wb.rd == id_ex.rs2)) {
            s.data_forward_rs2 = true;
        }
    }

    // Hazard messages
    if (has_stall) {
        s.hazard_msg = "STALL: Load-Use Data Hazard (RAW)";
    }
    if (s.structural_hazard) {
        s.hazard_msg = "WARNING: Structural Hazard Detected";
    }

    return s;
}
