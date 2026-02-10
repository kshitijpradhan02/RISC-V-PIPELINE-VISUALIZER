#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "pipeline.h"
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_stepButton_clicked();
    void onLoadInstructionsClicked();
    void onResetInstructionsClicked();

private:
    void loadDefaultInstructions();
    bool validateInstructions(const QStringList& lines, QString& errorMsg);
    Ui::MainWindow *ui;

    PipelineSimulator sim;   // Backend simulator
    int cycle = 0;           // Current cycle count
};

#endif // MAINWINDOW_H
