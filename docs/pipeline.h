#ifndef PIPELINE_H
#define PIPELINE_H

#include <array>
#include <vector>
#include <string>
#include <map>

struct Instruction {
    std::string text;
    int rd = 0, rs1 = 0, rs2 = 0, imm = 0;
    bool isLoad = false;
    bool isStore = false;
    bool isALU = false;
    bool isBranch = false;
};

struct Stage {
    bool valid = false;
    Instruction instr;
    int pc = 0;
    int result = 0;
    bool branchTaken = false;
    int branchTarget = 0;
};

class Pipeline {
public:
    Pipeline();
    void loadProgram(const std::vector<std::string> &lines);
    void step();
    bool finished() const;

    // UI helpers
    int getCycle() const;
    int getPC() const;
    bool stallInserted() const;
    std::string hazardMessage() const;
    const std::array<int,32> &getRegisters() const;
    const std::map<int,int> &getMemory() const;

    // stage text getters (used by MainWindow)
    std::string stageIF() const;
    std::string stageID() const;
    std::string stageEX() const;
    std::string stageMEM() const;
    std::string stageWB() const;

private:
    // internal helpers
    Instruction decode(const std::string &line);
    bool detectDataHazard();
    bool detectLoadUseHazard();
    bool detectControlHazard();
    int forwardValue(int reg);

    // pipeline registers
    Stage IF, ID, EX, MEM, WB;
    // state
    std::vector<std::string> program;
    std::array<int,32> regs{};
    std::map<int,int> memory;
    int pc = 0;
    int cycle = 0;
    bool stallThisCycle = false;
    std::string hazardMsg;
};

#endif
