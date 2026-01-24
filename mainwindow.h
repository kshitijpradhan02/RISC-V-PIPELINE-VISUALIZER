#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include "pipeline.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStep();
    void onLoad();
    void onReset();
    void onShow();
    void onRunAll();

private:
    void setupUI();
    void updateDisplay();
    void loadDemoProgram();

    // Backend
    PipelineSimulator sim{4096};
    int cycle = 0;

    // UI Widgets
    QTextEdit *instrEdit;

    // Pipeline display
    QLabel *ifLabel;
    QLabel *idLabel;
    QLabel *exLabel;
    QLabel *memLabel;
    QLabel *wbLabel;

    // Status & Hazard info
    QLabel *pcLabel;
    QLabel *cycleLabel;
    QLabel *stageLabel;
    QLabel *hazardLabel;
    QLabel *forwardLabel;

    // Register & Memory
    QTableWidget *regTable;
    QTableWidget *memTable;
};

#endif // MAINWINDOW_H
