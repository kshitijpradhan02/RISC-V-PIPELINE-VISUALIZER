#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>
#include <QString>

#include "pipeline.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void stepPipeline();

private:
    void updateRegisterTable();
    void updateMemoryTable();

    QTextEdit    *instrEdit = nullptr;
    QTextEdit    *cycleView = nullptr;
    QLabel       *pcLabel   = nullptr;
    QLabel       *hazardLabel = nullptr;
    QTableWidget *regTable  = nullptr;
    QTableWidget *memTable  = nullptr;

    QString  lastProgramText;
    Pipeline pipe;
};

#endif
