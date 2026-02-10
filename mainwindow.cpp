#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow())
    , sim(1024)          // initialize backend
{
    ui->setupUi(this);

    // --- demo program: simple instructions ---
    vector<string> demoProgram = {
        "ADD x3, x1, x2",
        "LW x5, 0x100(x0)",
        "SW x5, 0x104(x0)",
        "NOP",
        "NOP"
    };

    // --- Try loading from resource ---
    QFile file(":/programs/test.txt");
    vector<string> programFromFile;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty())
                programFromFile.push_back(line.toStdString());
        }
        file.close();
    }

    // Load whichever program is available
    if (!programFromFile.empty()) {
        sim.loadProgram(programFromFile, 0u);
        qDebug() << "Loaded program from resource file.";
    } else {
        sim.loadProgram(demoProgram, 0u);
        qDebug() << "Loaded demo program.";
    }

    // initial registers to make ADD visible
    try {
        sim.regs.write(1, 5);   // x1 = 5
        sim.regs.write(2, 7);   // x2 = 7
    } catch (...) {}

    // initial memory for LW
    try {
        sim.mem.sw(0x100, 12345);
    } catch (...) {}

    // Set initial UI texts
    ui->ifLabel->setText("NOP");
    ui->idLabel->setText("NOP");
    ui->exLabel->setText("NOP");
    ui->memLabel->setText("NOP");
    ui->wbLabel->setText("NOP");
    ui->pcLabel->setText("0");
    ui->cycleLabel->setText("0");

    cycle = 0;
}

// ------------------------------------------------------
// STEP BUTTON
// ------------------------------------------------------
void MainWindow::on_stepButton_clicked()
{
    qDebug() << "Step clicked";

    // Advance pipeline
    try {
        sim.step();
    } catch (const std::exception &e) {
        qDebug() << "sim.step() exception:" << e.what();
    } catch (...) {
        qDebug() << "sim.step() unknown exception";
    }

    cycle++;

    // get snapshot
    PipelineState snap;
    try {
        snap = sim.snapshot(cycle, sim.pc);
    } catch (...) {
        snap.cycle = cycle;
        snap.pc = sim.pc;
        snap.IF_instr = "NOP";
        snap.ID_instr = "NOP";
        snap.EX_instr = "NOP";
        snap.MEM_instr = "NOP";
        snap.WB_instr = "NOP";
    }

    // Update pipeline labels
    ui->ifLabel->setText(QString::fromStdString(snap.IF_instr));
    ui->idLabel->setText(QString::fromStdString(snap.ID_instr));
    ui->exLabel->setText(QString::fromStdString(snap.EX_instr));
    ui->memLabel->setText(QString::fromStdString(snap.MEM_instr));
    ui->wbLabel->setText(QString::fromStdString(snap.WB_instr));

    // Update PC + cycle
    ui->pcLabel->setText(QString::number(snap.pc));
    ui->cycleLabel->setText(QString::number(snap.cycle));

    qDebug() << "PC after step =" << snap.pc;
}

MainWindow::~MainWindow()
{
    delete ui;
}
