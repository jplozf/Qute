#include "MainWindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>

// Helper lambda to ensure a file exists on disk before opening
auto ensureFileExists = [](const QString &path) {
    QFileInfo info(path);
    if (!info.exists()) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.close(); // Creates an empty file on disk
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/Qute_icon.png"));

    const QString serverName = "qute_single_instance_socket";

    // Try to connect to an already running instance
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        QStringList args = app.arguments();
        QStringList filesToSend;

        for (int i = 1; i < args.count(); ++i) {
            QString absPath = QDir::current().absoluteFilePath(args.at(i));
            ensureFileExists(absPath); // Create if it doesn't exist yet
            filesToSend.append(absPath);
        }

        if (!filesToSend.isEmpty()) {
            QTextStream stream(&socket);
            stream << filesToSend.join("\n");
            stream.flush();
            socket.waitForBytesWritten(1000);
        }

        return 0;
    }

    // Primary instance startup
    QLocalServer server;
    QLocalServer::removeServer(serverName);
    server.listen(serverName);

    MainWindow w;

    QStringList args = app.arguments();
    for (int i = 1; i < args.count(); ++i) {
        QString absPath = QDir::current().absoluteFilePath(args.at(i));
        ensureFileExists(absPath); // Create if it doesn't exist yet
        w.openFileWithPath(absPath);
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
                    ensureFileExists(file); // Create if it doesn't exist yet
                    w.openFileWithPath(file);
                }
            }
            w.showNormal();
            w.raise();
            w.activateWindow();
            clientSocket->deleteLater();
        });
    });

    w.show();
    return app.exec();
}