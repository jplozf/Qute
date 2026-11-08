#include "HelpDialog.h"

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Qute Help & Documentation"));
    resize(700, 550);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QTextBrowser *browser = new QTextBrowser(this);

    // Load the embedded HTML file from Qt resources
    browser->setSource(QUrl("qrc:/help.html"));

    layout->addWidget(browser);
}
