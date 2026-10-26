#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/Qute_icon.png"));
    MainWindow w;
    w.show();
    return QApplication::exec();
}
