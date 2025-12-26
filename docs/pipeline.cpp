#include "pipeline.h"
#include <sstream>
#include <cctype>

Pipeline::Pipeline() {
    regs.fill(0);
    hazardMsg = "Hazard: None";
}

void Pipeline::loadProgram(const std::vector<std::string> &lines) {
    program = lines;
    pc = 0;
    cycle = 0;
    stallThisCycle = false;
    hazardMsg = "Hazard: None";
    IF = {}; ID = {}; EX = {}; MEM = {}; WB = {};
}

bool Pipeline::finished() const {
    bool empty = !IF.valid && !ID.valid && !EX.valid && !MEM.valid && !WB.valid;
    return (pc / 4 >= static_cast<int>(program.size())) && empty;
}

int Pipeline::getCycle() const { return cycle; }
int Pipeline::getPC() const { return pc; }
bool Pipeline::stallInserted() const { return stallThisCycle; }
std::string Pipeline::hazardMessage() const { return hazardMsg; }
const std::array<int,32> &Pipeline::getRegisters() const { return regs; }
const std::map<int,int> &Pipeline::getMemory() const { return memory; }

Instruction Pipeline::decode(const std::string &line) {
    Instruction inst;
    inst.text = line;
    std::istringstream iss(line);
    std::string op;
    iss >> op;

    auto parseReg = [](const std::string &r)->int {
        if (r.size() >= 2 && (r[0] == 'x' || r[0] == 'X'))
            return std::stoi(r.substr(1));
        return 0;
    };

    auto cleanReg = [](std::string s) {
        if (!s.empty() && s.back() == ',') s.pop_back();
        return s;
    };

    if (op == "add") {
        std::string rd, rs1, rs2;
        iss >> rd >> rs1 >> rs2;
        inst.rd = parseReg(cleanReg(rd));
        inst.rs1 = parseReg(cleanReg(rs1));
        inst.rs2 = parseReg(cleanReg(rs2));
        inst.isALU = true;
    } else if (op == "lw") {
        std::string rd, mem;
        iss >> rd >> mem;
        inst.rd = parseReg(cleanReg(rd));
        inst.isLoad = true;
        size_t lparen = mem.find('(');
        size_t rparen = mem.find(')');
        if (lparen != std::string::npos && rparen != std::string::npos) {
            inst.imm = std::stoi(mem.substr(0, lparen));
            std::string rs1_str = mem.substr(lparen + 1, rparen - lparen - 1);
            inst.rs1 = parseReg(cleanReg(rs1_str));
        }
    } else if (op == "sw") {
        std::string rs2, mem;
        iss >> rs2 >> mem;
        inst.rs2 = parseReg(cleanReg(rs2));
        inst.isStore = true;
        size_t lparen = mem.find('(');
        size_t rparen = mem.find(')');
        if (lparen != std::string::npos && rparen != std::string::npos) {
            inst.imm = std::stoi(mem.substr(0, lparen));
            std::string rs1_str = mem.substr(lparen + 1, rparen - lparen - 1);
            inst.rs1 = parseReg(cleanReg(rs1_str));
        }
    } else if (op == "beq") {
        std::string rs1, rs2, label;
        iss >> rs1 >> rs2 >> label;
        inst.rs1 = parseReg(cleanReg(rs1));
        inst.rs2 = parseReg(cleanReg(rs2));
        inst.isBranch = true;
        try {
            inst.imm = std::stoi(label);
        } catch (...) {
            inst.imm = 0;
        }
    }
    return inst;
}

bool Pipeline::detectDataHazard() {
    if (!ID.valid) return false;
    int neededReg1 = ID.instr.rs1;
    int neededReg2 = ID.instr.rs2;
    if (neededReg1 == 0 && neededReg2 == 0) return false;

    if (EX.valid && EX.instr.rd != 0 &&
        (neededReg1 == EX.instr.rd || neededReg2 == EX.instr.rd)) return true;
    if (MEM.valid && MEM.instr.rd != 0 &&
        (neededReg1 == MEM.instr.rd || neededReg2 == MEM.instr.rd)) return true;
    if (WB.valid && WB.instr.rd != 0 &&
        (neededReg1 == WB.instr.rd || neededReg2 == WB.instr.rd)) return true;
    return false;
}

bool Pipeline::detectLoadUseHazard() {
    if (!EX.valid || !EX.instr.isLoad || !ID.valid) return false;
    int loadDst = EX.instr.rd;
    if (loadDst == 0) return false;
    return (ID.instr.rs1 == loadDst) || (ID.instr.rs2 == loadDst);
}

bool Pipeline::detectControlHazard() {
    if (!EX.valid || !EX.instr.isBranch) return false;
    int val1 = forwardValue(EX.instr.rs1);
    int val2 = forwardValue(EX.instr.rs2);
    return (val1 == val2);
}

int Pipeline::forwardValue(int reg) {
    if (reg < 0 || reg >= 32) return 0;

    if (WB.valid && !WB.instr.isStore && WB.instr.rd == reg) return WB.result;
    if (MEM.valid && MEM.instr.rd == reg) return MEM.result;
    if (EX.valid && EX.instr.rd == reg && EX.instr.isALU) return EX.result;
    return regs[reg];
}

void Pipeline::step() {
    if (finished()) return;

    ++cycle;
    stallThisCycle = false;
    hazardMsg = "Hazard: None";

    // Execute stage computations FIRST (with forwarding)
    if (EX.valid) {
        auto &instr = EX.instr;
        if (instr.isALU) {
            int op1 = forwardValue(instr.rs1);
            int op2 = forwardValue(instr.rs2);
            EX.result = op1 + op2;
        } else if (instr.isLoad || instr.isStore) {
            EX.result = forwardValue(instr.rs1) + instr.imm;
        } else if (instr.isBranch) {
            int val1 = forwardValue(instr.rs1);
            int val2 = forwardValue(instr.rs2);
            EX.branchTaken = (val1 == val2);
            if (EX.branchTaken) {
                EX.branchTarget = EX.pc + (instr.imm * 4);
            }
        }
    }

    // 1. Writeback
    if (WB.valid && !WB.instr.isStore && WB.instr.rd != 0) {
        regs[WB.instr.rd] = WB.result;
    }

    // 2. Memory
    if (MEM.valid) {
        auto &instr = MEM.instr;
        if (instr.isLoad) {
            int addr = MEM.result;
            auto it = memory.find(addr);
            MEM.result = (it != memory.end()) ? it->second : 0;
        } else if (instr.isStore) {
            int addr = MEM.result;
            int val = forwardValue(instr.rs2);
            memory[addr] = val;
        }
    }

    // 3. Hazard Detection (Priority: Control > Load-Use > Data)
    if (detectControlHazard()) {
        stallThisCycle = true;
        hazardMsg = "Control hazard: branch taken";
        IF.valid = false;
        ID.valid = false;
        if (EX.branchTaken) pc = EX.branchTarget;
        WB = MEM; MEM = EX;
        return;
    }

    if (detectLoadUseHazard()) {
        stallThisCycle = true;
        hazardMsg = "Load-use hazard: stall";
        WB = MEM; MEM = EX;
        EX.valid = false;
        return;
    }

    // Data hazards handled by forwarding

    // 4. Normal pipeline advance
    WB = MEM; MEM = EX; EX = ID; ID = IF;

    // 5. Fetch
    if (pc / 4 < static_cast<int>(program.size())) {
        IF.valid = true;
        IF.instr = decode(program[pc / 4]);
        IF.pc = pc;
        pc += 4;
    } else {
        IF.valid = false;
    }
}

std::string Pipeline::stageIF()  const { return IF.valid  ? IF.instr.text  : "nop"; }
std::string Pipeline::stageID()  const { return ID.valid  ? ID.instr.text  : "nop"; }
std::string Pipeline::stageEX()  const { return EX.valid  ? EX.instr.text  : "nop"; }
std::string Pipeline::stageMEM() const { return MEM.valid ? MEM.instr.text : "nop"; }
std::string Pipeline::stageWB()  const { return WB.valid  ? WB.instr.text  : "nop"; }
