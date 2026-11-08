#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPixmap>

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(tr("About Qute"));
        resize(420, 340);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);

        // App Icon (Ensure your icon is added to your .qrc resource file)
        QLabel *iconLabel = new QLabel(this);
        QPixmap iconPixmap(":/icons/Qute_icon.png"); // Adjust path to match your resource file
        if (!iconPixmap.isNull()) {
            iconLabel->setPixmap(iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        // Application Name & Version
        QString versionText = QString("%1.%2-%3")
                                .arg(APP_VERSION)
                                .arg(GIT_COMMIT_COUNT)
                                .arg(GIT_HASH);
        QLabel *titleLabel = new QLabel(tr("<h2>Qute</h2><p><b>Version %1</b></p>").arg(versionText), this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        // Description & "Usual Blah Blah" (Copyright, License, Qt version)
        QString aboutText = tr(
            "A lightweight, fast, and feature-rich multi-tabbed code text editor "
            "engineered with C++ and Qt 6 for Linux environments.<br><br>"
            "Powered by Qt 6.<br>"
            "&copy; 2026 J.-P. Liguori. All rights reserved.<br><br>"
            "Distributed under the MIT License."
            );

        QLabel *descLabel = new QLabel(aboutText, this);
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);
        descLabel->setOpenExternalLinks(true);
        layout->addWidget(descLabel);

        layout->addStretch();

        // Close Button Box
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
        layout->addWidget(buttonBox);
    }
};