#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setStyleSheet(
        "QPushButton { background: #2d6cdf; color: white; border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background: #245bb5; }"
        "QPushButton:pressed { background: #1c478c; }"
    );

    MainWindow window;


    window.show();
    return app.exec();
}