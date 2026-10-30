#include "MainWindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFileInfo>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/Qute_icon.png"));

    // Define a unique local socket name for Qute
    const QString serverName = "qute_single_instance_socket";

    // Try to connect to an already running instance
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        // An instance is already running! Send command-line file arguments to it.
        QStringList args = app.arguments();
        QStringList filesToSend;

        for (int i = 1; i < args.count(); ++i) {
            QString arg = args.at(i);
            // Convert relative paths to absolute paths so the main instance can find them
            QFileInfo info(arg);
            if (info.exists()) {
                filesToSend.append(info.absoluteFilePath());
            }
        }

        if (!filesToSend.isEmpty()) {
            QTextStream stream(&socket);
            stream << filesToSend.join("\n");
            stream.flush();
            socket.waitForBytesWritten(1000);
        }

        return 0; // Exit this secondary process immediately
    }

    // No running instance found. We are the primary instance!
    QLocalServer server;
    // Clean up any stale socket if a previous crash occurred
    QLocalServer::removeServer(serverName);
    server.listen(serverName);

    MainWindow w;

    // ---> PROCESS COMMAND-LINE ARGUMENTS ON FIRST LAUNCH <---
    QStringList args = app.arguments();
    for (int i = 1; i < args.count(); ++i) {
        QString arg = args.at(i);
        QFileInfo info(arg);
        if (info.exists() && info.isFile()) {
            w.openFileWithPath(info.absoluteFilePath());
        }
    }

    // Listen for incoming file open requests from secondary instances
    QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w]() {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if (!clientSocket) return;

        QObject::connect(clientSocket, &QLocalSocket::readyRead, &w, [clientSocket, &w]() {
            QTextStream stream(clientSocket);
            QString data = stream.readAll();
            QStringList files = data.split('\n', Qt::SkipEmptyParts);

            for (const QString &file : files) {
                if (!file.isEmpty()) {
                    w.openFileWithPath(file); // Opens as a new tab (or focuses if already open)
                }
            }
            // ---> BRING WINDOW TO THE FRONT <---
            w.showNormal();       // Restores window if it was minimized to the taskbar
            w.raise();            // Brings the window to the top of the stacking order
            w.activateWindow();   // Requests keyboard focus from the window manager
            clientSocket->deleteLater();
        });
    });

    w.show();
    return app.exec();
}