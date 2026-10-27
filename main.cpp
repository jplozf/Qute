#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/Qute_icon.png"));

    // Get command line arguments (excluding the program path at index 0)
    QStringList args = app.arguments();
    args.removeFirst();

    MainWindow w(args);
    w.show();

    return app.exec();
}
