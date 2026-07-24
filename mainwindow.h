#pragma once

#include <QMainWindow>
#include "core.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCreateBaselineClicked();
    void onCompareClicked();
    void onBrowseFolderClicked();

private:
    Ui::MainWindow *ui;
    fs::path selectedRoot;
};