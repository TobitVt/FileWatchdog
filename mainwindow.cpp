#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "database.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidgetItem>
#include <QColor>
#include <QDebug>
#include <QApplication>


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

    // One baseline per folder: name it after the folder path itself,
    QString name = QString::fromStdString(selectedRoot.u8string());

    ui->progressBar->setVisible(true);
        ui->progressLabel->setText("Scanning...");

    try {
        Database db(default_database_path());
        ScanOutcome outcome = run_create(db, selectedRoot, name.toStdString(),
            [this](const fs::path&, std::size_t count) {
                ui->progressLabel->setText(QString("Scanned %1 files...").arg(count));
                QApplication::processEvents();
                return true; // never cancel
            });

        ui->progressBar->setVisible(false);
        ui->progressLabel->clear();
        QMessageBox::information(this, "Done", QString("Saved baseline with %1 files.").arg(outcome.files.size()));
    } catch (const std::exception& ex) {
        ui->progressBar->setVisible(false);
        ui->progressLabel->clear();
        QMessageBox::critical(this, "Error", ex.what());
    }
}

void MainWindow::onCompareClicked() {
    if (selectedRoot.empty()) {
        QMessageBox::warning(this, "No folder", "Pick a folder first.");
        return;
    }

    QString name = QString::fromStdString(selectedRoot.u8string());

   try {
        Database db(default_database_path());
        if (!db.baseline_exists(name.toStdString())) {
            QMessageBox::warning(this, "No baseline", "No baseline named '" + name + "' exists yet. Create one first.");
            return;
        }

        ui->progressBar->setVisible(true);
        ui->progressLabel->setText("Scanning...");

        CompareOutcome outcome = run_compare(db, selectedRoot, name.toStdString(),
            [this](const fs::path&, std::size_t count) {
                ui->progressLabel->setText(QString("Scanned %1 files...").arg(count));
                QApplication::processEvents();
                return true;
            });

        ui->progressBar->setVisible(false);
        ui->progressLabel->clear();

        qDebug() << "Compare found" << outcome.changes.size() << "changes.";
        ui->resultsTable->setRowCount(static_cast<int>(outcome.changes.size()));

        for (int row = 0; row < static_cast<int>(outcome.changes.size()); ++row) {
            const auto& change = outcome.changes[row];

            auto* pathItem = new QTableWidgetItem(QString::fromStdString(change.path));
            auto* statusItem = new QTableWidgetItem(QString::fromStdString(change_type_to_string(change.status)));

        QColor rowColor;

        switch (change.status) {
            case ChangeType::New:
                rowColor = QColor("#1565C0");     
                break;

            case ChangeType::Modified:
                rowColor = QColor("#F9A825");     
                break;

            case ChangeType::Deleted:
                rowColor = QColor("#C62828");      
                break;

            case ChangeType::Unchanged:
                rowColor = QColor("#2E7D32");     
                break;
        }
            pathItem->setBackground(rowColor);
            statusItem->setBackground(rowColor);

            ui->resultsTable->setItem(row, 0, pathItem);
            ui->resultsTable->setItem(row, 1, statusItem);
        }

        ui->resultsTable->resizeColumnsToContents();
    } catch (const std::exception& ex) {
        ui->progressBar->setVisible(false);
        ui->progressLabel->clear();
        QMessageBox::critical(this, "Error", ex.what());
    }
}