#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidgetItem>
#include <QSplitter>
#include <QMessageBox>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), sim(4096) {
    setWindowTitle("RISC-V 5-Stage Pipeline Simulator with Hazard Detection");
    setGeometry(100, 100, 1600, 1050);
    setupUI();
    loadDemoProgram();
    updateDisplay();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ===== INSTRUCTIONS PANEL =====
    QGroupBox *instrGroup = new QGroupBox("Instructions (Load/Store/R-Type)", this);
    QVBoxLayout *instrLayout = new QVBoxLayout();
    instrEdit = new QTextEdit();
    instrEdit->setPlaceholderText(
        "Enter RISC-V instructions (one per line):\n"
        "lw x5, 0x100(x0)\n"
        "add x6, x5, x0\n"
        "sw x6, 0x104(x0)\n"
        "lw x7, 0x108(x0)\n"
        "sub x8, x7, x6"
        );
    instrEdit->setMaximumHeight(100);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *loadBtn  = new QPushButton("Load Instructions");
    QPushButton *resetBtn = new QPushButton("Reset to Default");
    btnLayout->addWidget(loadBtn);
    btnLayout->addWidget(resetBtn);
    btnLayout->addStretch();

    instrLayout->addWidget(instrEdit);
    instrLayout->addLayout(btnLayout);
    instrGroup->setLayout(instrLayout);

    // ===== PIPELINE VISUALIZATION =====
    QGroupBox *pipelineGroup = new QGroupBox("Pipeline Stages with Hazard Detection", this);
    QVBoxLayout *pipeVLayout = new QVBoxLayout();

    // Pipeline boxes
    QHBoxLayout *pipelineLayout = new QHBoxLayout();
    ifLabel  = new QLabel("IF\n\nNOP");
    idLabel  = new QLabel("ID\n\nNOP");
    exLabel  = new QLabel("EX\n\nNOP");
    memLabel = new QLabel("MEM\n\nNOP");
    wbLabel  = new QLabel("WB\n\nNOP");

    QString normalBox =
        "QLabel { background-color: white; border: 2px solid #333; "
        "border-radius: 6px; padding: 10px; font-weight: bold; "
        "min-width: 120px; min-height: 90px; text-align: center; font-size: 12px; }";

    ifLabel->setStyleSheet(normalBox);
    idLabel->setStyleSheet(normalBox);
    exLabel->setStyleSheet(normalBox);
    memLabel->setStyleSheet(normalBox);
    wbLabel->setStyleSheet(normalBox);

    pipelineLayout->addWidget(ifLabel);
    pipelineLayout->addWidget(idLabel);
    pipelineLayout->addWidget(exLabel);
    pipelineLayout->addWidget(memLabel);
    pipelineLayout->addWidget(wbLabel);

    pipeVLayout->addLayout(pipelineLayout);

    // Status row: PC / Cycle / Stage
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->addStretch();
    pcLabel    = new QLabel("PC: 0x00000000");
    cycleLabel = new QLabel("Cycle: 0");
    stageLabel = new QLabel("Stage: IF");
    statusLayout->addWidget(pcLabel);
    statusLayout->addSpacing(20);
    statusLayout->addWidget(cycleLabel);
    statusLayout->addSpacing(20);
    statusLayout->addWidget(stageLabel);
    statusLayout->addStretch();
    pipeVLayout->addLayout(statusLayout);

    // Hazard info row with detailed info
    hazardLabel = new QLabel("✓ No Hazards Detected");
    hazardLabel->setStyleSheet("QLabel { color: green; font-weight: bold; font-size: 13px; }");
    hazardLabel->setMinimumHeight(25);
    pipeVLayout->addWidget(hazardLabel);

    // Data Forwarding info row
    forwardLabel = new QLabel("Forwarding: None");
    forwardLabel->setStyleSheet("QLabel { color: blue; font-size: 11px; }");
    pipeVLayout->addWidget(forwardLabel);

    // Control buttons
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QPushButton *stepBtn = new QPushButton("Step");
    QPushButton *showBtn = new QPushButton("Show");
    QPushButton *runBtn  = new QPushButton("Run All");
    controlLayout->addWidget(stepBtn);
    controlLayout->addWidget(showBtn);
    controlLayout->addWidget(runBtn);
    controlLayout->addStretch();
    pipeVLayout->addLayout(controlLayout);

    pipelineGroup->setLayout(pipeVLayout);

    // ===== REGISTER & MEMORY TABLES =====
    QSplitter *dataSplitter = new QSplitter(Qt::Horizontal);

    QGroupBox *regGroup = new QGroupBox("Register File (x0-x31)", this);
    QVBoxLayout *regLayout = new QVBoxLayout();
    regTable = new QTableWidget();
    regTable->setColumnCount(2);
    regTable->setHorizontalHeaderLabels({"Register", "Value"});
    regTable->setRowCount(32);
    regTable->setMaximumHeight(280);
    regTable->setColumnWidth(0, 80);
    regTable->setColumnWidth(1, 100);
    regLayout->addWidget(regTable);
    regGroup->setLayout(regLayout);

    QGroupBox *memGroup = new QGroupBox("Memory (0x100+)", this);
    QVBoxLayout *memLayout = new QVBoxLayout();
    memTable = new QTableWidget();
    memTable->setColumnCount(2);
    memTable->setHorizontalHeaderLabels({"Address", "Value"});
    memTable->setRowCount(8);
    memTable->setMaximumHeight(280);
    memTable->setColumnWidth(0, 120);
    memTable->setColumnWidth(1, 100);
    memLayout->addWidget(memTable);
    memGroup->setLayout(memLayout);

    dataSplitter->addWidget(regGroup);
    dataSplitter->addWidget(memGroup);
    dataSplitter->setStretchFactor(0, 1);
    dataSplitter->setStretchFactor(1, 1);

    // ===== MAIN LAYOUT =====
    mainLayout->addWidget(instrGroup, 1);
    mainLayout->addWidget(pipelineGroup, 2);
    mainLayout->addWidget(dataSplitter, 2);

    // Connections
    connect(stepBtn,  &QPushButton::clicked, this, &MainWindow::onStep);
    connect(loadBtn,  &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::onReset);
    connect(showBtn,  &QPushButton::clicked, this, &MainWindow::onShow);
    connect(runBtn,   &QPushButton::clicked, this, &MainWindow::onRunAll);
}

void MainWindow::loadDemoProgram() {
    std::vector<std::string> demo = {
        "lw x5, 0x100(x0)",
        "add x6, x5, x0",
        "sw x6, 0x104(x0)",
        "lw x7, 0x108(x0)",
        "sub x8, x7, x6"
    };

    try {
        sim.loadProgram(demo, 0);
        sim.mem.sw(0x100, 12345);
        sim.mem.sw(0x108, 67890);
    } catch (...) {}

    QString text;
    for (const auto& instr : demo) {
        text += QString::fromStdString(instr) + "\n";
    }
    instrEdit->setPlainText(text);
}

void MainWindow::updateDisplay() {
    auto snap = sim.snapshot(cycle, sim.pc);

    QString normalBox =
        "QLabel { background-color: white; border: 2px solid #333; "
        "border-radius: 6px; padding: 10px; font-weight: bold; "
        "min-width: 120px; min-height: 90px; text-align: center; font-size: 12px; }";

    QString activeBox =
        "QLabel { background-color: #ffe5e5; border: 3px solid red; "
        "border-radius: 6px; padding: 10px; font-weight: bold; "
        "min-width: 120px; min-height: 90px; text-align: center; font-size: 12px; }";

    auto makeStage = [&](QLabel *lbl, const QString &name, const std::string &instrStr) {
        QString instr = QString::fromStdString(instrStr).trimmed();
        bool active = instr.toLower() != "nop";
        QString dot = active ? "  ●" : "";
        lbl->setText(QString("%1%2\n\n%3")
                         .arg(name)
                         .arg(dot)
                         .arg(instr.left(12)));
        lbl->setStyleSheet(active ? activeBox : normalBox);
    };

    makeStage(ifLabel,  "IF",  snap.IF_instr);
    makeStage(idLabel,  "ID",  snap.ID_instr);
    makeStage(exLabel,  "EX",  snap.EX_instr);
    makeStage(memLabel, "MEM", snap.MEM_instr);
    makeStage(wbLabel,  "WB",  snap.WB_instr);

    // Status
    pcLabel->setText(QString("PC: 0x%1").arg(snap.pc, 8, 16, QChar('0')));
    cycleLabel->setText(QString("Cycle: %1 (Stalls: %2)").arg(snap.cycle).arg(sim.stall_cycles));
    stageLabel->setText("Stage: Pipeline Executing");

    // Hazard info with color coding
    QString hazardText = "✓ No Hazards Detected";
    QString hazardColor = "green";

    if (snap.load_use_hazard) {
        hazardText = "⚠ LOAD-USE HAZARD (RAW): Stalling pipeline for 1 cycle";
        hazardColor = "red";
    } else if (snap.structural_hazard) {
        hazardText = "⚠ STRUCTURAL HAZARD: Multiple writes to same register";
        hazardColor = "orange";
    }

    hazardLabel->setText(hazardText);
    hazardLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; font-size: 13px; }").arg(hazardColor));

    // Forwarding information
    QString fwdText = "Forwarding: ";
    if (snap.data_forward_rs1 || snap.data_forward_rs2) {
        if (snap.data_forward_rs1) fwdText += "RS1→EX ";
        if (snap.data_forward_rs2) fwdText += "RS2→EX ";
    } else {
        fwdText += "None";
    }
    forwardLabel->setText(fwdText);

    // Registers
    for (int i = 0; i < 32; ++i) {
        QTableWidgetItem *nameItem = new QTableWidgetItem(QString("x%1").arg(i));
        QTableWidgetItem *valItem  = new QTableWidgetItem(QString::number(snap.regs[i]));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        valItem->setFlags(valItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFont(QFont("Courier", 10));
        valItem->setFont(QFont("Courier", 10));
        regTable->setItem(i, 0, nameItem);
        regTable->setItem(i, 1, valItem);
    }

    // Memory
    for (int i = 0; i < 8; ++i) {
        uint32_t addr = 0x100 + i * 4;
        int32_t val = 0;
        try { val = sim.mem.lw(addr); } catch (...) {}
        QTableWidgetItem *addrItem = new QTableWidgetItem(
            QString("0x%1").arg(addr, 8, 16, QChar('0')));
        QTableWidgetItem *valItem = new QTableWidgetItem(QString::number(val));
        addrItem->setFlags(addrItem->flags() & ~Qt::ItemIsEditable);
        valItem->setFlags(valItem->flags() & ~Qt::ItemIsEditable);
        addrItem->setFont(QFont("Courier", 10));
        valItem->setFont(QFont("Courier", 10));
        memTable->setItem(i, 0, addrItem);
        memTable->setItem(i, 1, valItem);
    }
}

void MainWindow::onStep() {
    try {
        sim.step();
        cycle++;
        updateDisplay();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString::fromStdString(std::string(e.what())));
    }
}

void MainWindow::onLoad() {
    QString text = instrEdit->toPlainText();
    std::vector<std::string> program;
    for (const auto& line : text.split('\n')) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            program.push_back(trimmed.toStdString());
        }
    }

    if (program.empty()) {
        QMessageBox::warning(this, "Warning", "No instructions to load!");
        return;
    }

    try {
        cycle = 0;
        sim = PipelineSimulator(4096);
        sim.loadProgram(program, 0);
        sim.mem.sw(0x100, 12345);
        sim.mem.sw(0x108, 67890);
        updateDisplay();
        QMessageBox::information(this, "Success",
                                 QString("Program loaded (%1 instructions)").arg(program.size()));
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString::fromStdString(std::string(e.what())));
    }
}

void MainWindow::onReset() {
    cycle = 0;
    sim = PipelineSimulator(4096);
    loadDemoProgram();
    updateDisplay();
}

void MainWindow::onShow() {
    updateDisplay();
}

void MainWindow::onRunAll() {
    try {
        // Run until no more valid instructions
        int maxCycles = 100;
        while (cycle < maxCycles) {
            sim.step();
            cycle++;

            // Check if pipeline is empty
            auto snap = sim.snapshot(cycle, sim.pc);
            if (snap.IF_instr == "NOP" && snap.ID_instr == "NOP" &&
                snap.EX_instr == "NOP" && snap.MEM_instr == "NOP" &&
                snap.WB_instr == "NOP") {
                break;
            }
        }
        updateDisplay();
        QMessageBox::information(this, "Complete",
                                 QString("Program executed in %1 cycles with %2 stall cycles")
                                     .arg(cycle).arg(sim.stall_cycles));
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString::fromStdString(std::string(e.what())));
    }
}
