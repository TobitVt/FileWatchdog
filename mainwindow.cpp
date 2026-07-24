#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "database.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidgetItem>
#include <QColor>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) 
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseFolderClicked);
    connect(ui->createButton, &QPushButton::clicked, this, &MainWindow::onCreateBaselineClicked);
    connect(ui->compareButton, &QPushButton::clicked, this, &MainWindow::onCompareClicked);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::onBrowseFolderClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select folder to monitor");
    if (!dir.isEmpty()) {
        selectedRoot = dir.toStdString();
        ui->folderLabel->setText(dir);
    }
}

void MainWindow::onCreateBaselineClicked() {
    if (selectedRoot.empty()) {
        QMessageBox::warning(this, "No folder", "Pick a folder first.");
        return;
    }

    bool ok;
    QString name = QInputDialog::getText(this, "Baseline name", "Name:", QLineEdit::Normal, "default_baseline", &ok);
    if (!ok || name.isEmpty()) return;

    try {
        Database db(default_database_path());
        ScanOutcome outcome = run_create(db, selectedRoot, name.toStdString());
        QMessageBox::information(this, "Done", QString("Saved baseline with %1 files.").arg(outcome.files.size()));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", ex.what());
    }
}

void MainWindow::onCompareClicked() {
    if (selectedRoot.empty()) {
        QMessageBox::warning(this, "No folder", "Pick a folder first.");
        return;
    }

    bool ok;
    QString name = QInputDialog::getText(this, "Baseline name", "Name:", QLineEdit::Normal, "default_baseline", &ok);
    if (!ok || name.isEmpty()) return;

    try {
        Database db(default_database_path());
        CompareOutcome outcome = run_compare(db, selectedRoot, name.toStdString());

        qDebug() << "Compare found" << outcome.changes.size() << "changes.";
        ui->resultsTable->setRowCount(static_cast<int>(outcome.changes.size()));

        for (int row = 0; row < static_cast<int>(outcome.changes.size()); ++row) {
            const auto& change = outcome.changes[row];

            auto* pathItem = new QTableWidgetItem(QString::fromStdString(change.path));
            auto* statusItem = new QTableWidgetItem(QString::fromStdString(change_type_to_string(change.status)));

            QColor rowColor;
            switch (change.status) {
                case ChangeType::New:       rowColor = QColor(200, 255, 200); break; // light green
                case ChangeType::Modified:  rowColor = QColor(255, 240, 180); break; // light amber
                case ChangeType::Deleted:   rowColor = QColor(255, 200, 200); break; // light red
                case ChangeType::Unchanged: rowColor = QColor(240, 240, 240); break; // light gray
            }
            pathItem->setBackground(rowColor);
            statusItem->setBackground(rowColor);

            ui->resultsTable->setItem(row, 0, pathItem);
            ui->resultsTable->setItem(row, 1, statusItem);
        }

        ui->resultsTable->resizeColumnsToContents();
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", ex.what());
    }
}