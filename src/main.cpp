#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <cstdlib>
#include <exception>

int main(int argc, char *argv[])
{
    // The QApplication is outside the try-catch because
    // QMessageBox requires an active QApplication to work.

    QApplication a(argc, argv);

    try
    {
        MainWindow w;
        w.show();

        return a.exec();
    }
    catch (const std::exception &ex)
    {
        QString errMsg = QString(ex.what()) + "\nThe application will now exit.";
        QMessageBox::critical(nullptr, "Fatal Error", errMsg);
        return EXIT_FAILURE;
    }
}
