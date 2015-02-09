#include "mainwindow.h"
#include <QApplication>
#include <exception>
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    try
    {
        QApplication a(argc, argv);
        MainWindow w;
        w.show();

        return a.exec();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal Error: " << ex.what() << "\nThe application will now exit." << std::endl;
        return EXIT_FAILURE;
    }
}
