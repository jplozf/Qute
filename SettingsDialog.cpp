#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFontDialog>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(const QFont &font, EditorTheme theme, int tabWidth,
                               bool insertSpaces, bool minimizeToTray, MainWindow *parent)
    : QDialog(parent), m_font(font), m_theme(theme), m_tabWidth(tabWidth),
    m_insertSpaces(insertSpaces), m_minimizeToTray(minimizeToTray),
    m_mainWindow(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(440, 260);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // Font Setting Row
    QHBoxLayout *fontLayout = new QHBoxLayout();
    fontPreviewLabel = new QLabel(QString("%1, %2pt").arg(m_font.family()).arg(m_font.pointSize()), this);
    fontButton = new QPushButton(tr("Change..."), this);
    fontLayout->addWidget(fontPreviewLabel);
    fontLayout->addWidget(fontButton);
    formLayout->addRow(tr("Editor Font:"), fontLayout);

    // Theme Setting Row
    themeComboBox = new QComboBox(this);
    themeComboBox->addItems({tr("Light"), tr("Dark (VS Code)"), tr("Monokai"), tr("Solarized Dark"), tr("Nord"), tr("Dracula"), tr("OneDark"), tr("GitHubDark"), tr("Coffee")});
    themeComboBox->setCurrentIndex(static_cast<int>(m_theme));
    formLayout->addRow(tr("Syntax Theme:"), themeComboBox);

    // Tab Width Row
    tabWidthSpinBox = new QSpinBox(this);
    tabWidthSpinBox->setRange(2, 8);
    tabWidthSpinBox->setValue(m_tabWidth);
    formLayout->addRow(tr("Tab Width (Spaces):"), tabWidthSpinBox);

    // Insert Spaces Checkbox
    insertSpacesCheckBox = new QCheckBox(tr("Insert spaces for tabs"), this);
    insertSpacesCheckBox->setChecked(m_insertSpaces);
    formLayout->addRow(QString(), insertSpacesCheckBox);

    // Minimize to Tray Checkbox
    minimizeToTrayCheckBox = new QCheckBox(tr("Minimize to system tray on close"), this);
    minimizeToTrayCheckBox->setChecked(m_minimizeToTray);
    formLayout->addRow(QString(), minimizeToTrayCheckBox);

    // Config File
    confFileButton = new QPushButton(tr("Open..."), this);
    connect(confFileButton, &QPushButton::clicked, this, &SettingsDialog::openConfFile);
    formLayout->addRow(tr("Config File"), confFileButton);

    // Tools File
    toolsFileButton = new QPushButton(tr("Open..."), this);
    connect(toolsFileButton, &QPushButton::clicked, this, &SettingsDialog::openToolsFile);
    formLayout->addRow(tr("Tools File"), toolsFileButton);

    // Templates Folder
    templatesFolderButton = new QPushButton(tr("Open..."), this);
    connect(templatesFolderButton, &QPushButton::clicked, this, &SettingsDialog::openTemplatesFolder);
    formLayout->addRow(tr("Templates Folder"), templatesFolderButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(fontButton, &QPushButton::clicked, this, &SettingsDialog::changeFont);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::changeFont() {
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, m_font, this, tr("Choose Monospace Font"));
    if (ok) {
        m_font = newFont;
        fontPreviewLabel->setText(QString("%1, %2pt").arg(m_font.family()).arg(m_font.pointSize()));
    }
}

EditorTheme SettingsDialog::getSelectedTheme() const {
    return static_cast<EditorTheme>(themeComboBox->currentIndex());
}

int SettingsDialog::getTabWidth() const {
    return tabWidthSpinBox->value();
}

bool SettingsDialog::getInsertSpaces() const {
    return insertSpacesCheckBox->isChecked();
}

bool SettingsDialog::getMinimizeToTray() const {
    return minimizeToTrayCheckBox->isChecked();
}

void SettingsDialog::openTemplatesFolder() {
    // Determine path: ~/.config/YourOrganization/Qute/templates
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/" + m_mainWindow->appConstants->getQString("ORGANIZATION_NAME")
                        + "/" + m_mainWindow->appConstants->getQString("APPLICATION_NAME");
    QDir().mkpath(configDir); // Ensure directory exists
    QString templatesDirPath = configDir + "/templates";
    QDesktopServices::openUrl(QUrl::fromLocalFile(templatesDirPath));
    accept();
}

void SettingsDialog::openConfFile() {
    // Determine path: ~/.config/YourOrganization/Qute.conf
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/" + m_mainWindow->appConstants->getQString("ORGANIZATION_NAME");
    QDir().mkpath(configDir); // Ensure directory exists
    QString confPath = configDir + "/" + m_mainWindow->appConstants->getQString("APPLICATION_NAME") + ".conf";

    m_mainWindow->openFileWithPath(confPath);
    accept();
}

void SettingsDialog::openToolsFile() {
    // Determine path: ~/.config/YourOrganization/Qute/tools.conf
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/" + m_mainWindow->appConstants->getQString("ORGANIZATION_NAME")
                        + "/" + m_mainWindow->appConstants->getQString("APPLICATION_NAME");
    QDir().mkpath(configDir); // Ensure directory exists
    QString confPath = configDir + "/tools.conf";

    // Create default tools.conf if it doesn't exist
    if (!QFile::exists(confPath)) {
        QFile file(confPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Define custom tools here: Menu Text : command\n";
            out << "# Placeholders: %F (full path), %f (filename), %d (directory), %n (name without ext)\n";
            out << "File Info : exiftool %F\n";
            out << "Open Containing Folder : xdg-open %d\n";
        }
    }
    m_mainWindow->openFileWithPath(confPath);
    accept();
}