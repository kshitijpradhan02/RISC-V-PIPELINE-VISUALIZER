#include "mainwindow.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextCursor>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(900, 900);

    QWidget *c = new QWidget(this);
    setCentralWidget(c);

    QVBoxLayout *l = new QVBoxLayout(c);

    instrEdit = new QTextEdit();
    instrEdit->setPlaceholderText(
        "Example:\n"
        "lw x1, 0x100(x0)\n"
        "add x2, x1, x3"
        );
    l->addWidget(instrEdit);

    QPushButton *stepBtn = new QPushButton("STEP");
    l->addWidget(stepBtn);

    pcLabel = new QLabel("PC: 0x0");
    l->addWidget(pcLabel);

    cycleView = new QTextEdit();
    cycleView->setReadOnly(true);
    l->addWidget(cycleView);

    hazardLabel = new QLabel("Hazard: None");
    l->addWidget(hazardLabel);

    regTable = new QTableWidget(32, 2);
    regTable->setHorizontalHeaderLabels({"Register","Value"});
    l->addWidget(regTable);

    memTable = new QTableWidget(0, 2);
    memTable->setHorizontalHeaderLabels({"Address","Value"});
    l->addWidget(memTable);

    connect(stepBtn, &QPushButton::clicked,
            this, &MainWindow::stepPipeline);
}

void MainWindow::stepPipeline()
{
    QString txt = instrEdit->toPlainText();

    if (txt != lastProgramText) {
        QStringList lines = txt.split("\n");
        std::vector<std::string> prog;

        for (auto &l : lines)
            if (!l.trimmed().isEmpty())
                prog.push_back(l.toStdString());

        pipe.loadProgram(prog);
        cycleView->clear();
        lastProgramText = txt;
    }

    pipe.step();

    pcLabel->setText("PC: 0x" + QString::number(pipe.getPC(), 16));

    QTextCursor cur(cycleView->textCursor());
    cur.movePosition(QTextCursor::End);

    cur.insertText("Cycle " + QString::number(pipe.getCycle()) + "\n");
    cur.insertText("IF  : " + QString::fromStdString(pipe.stageIF())  + "\n");
    cur.insertText("ID  : " + QString::fromStdString(pipe.stageID())  + "\n");
    cur.insertText("EX  : " + QString::fromStdString(pipe.stageEX())  + "\n");
    cur.insertText("MEM : " + QString::fromStdString(pipe.stageMEM()) + "\n");
    cur.insertText("WB  : " + QString::fromStdString(pipe.stageWB())  + "\n\n");

    hazardLabel->setText(
        pipe.stallInserted()
            ? QString::fromStdString(pipe.hazardMessage())
            : "Hazard: None"
        );

    updateRegisterTable();
    updateMemoryTable();
}

void MainWindow::updateRegisterTable()
{
    auto regs = pipe.getRegisters();
    for (int i = 0; i < 32; ++i) {
        regTable->setItem(i, 0,
                          new QTableWidgetItem("x" + QString::number(i)));
        regTable->setItem(i, 1,
                          new QTableWidgetItem(QString::number(regs[i])));
    }
}

void MainWindow::updateMemoryTable()
{
    auto mem = pipe.getMemory();
    memTable->setRowCount(static_cast<int>(mem.size()));

    int r = 0;
    for (auto &e : mem) {
        memTable->setItem(r, 0,
                          new QTableWidgetItem(QString::number(e.first)));
        memTable->setItem(r, 1,
                          new QTableWidgetItem(QString::number(e.second)));
        ++r;
    }
}
