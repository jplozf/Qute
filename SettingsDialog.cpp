#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFontDialog>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(const QFont &font, EditorTheme theme, int tabWidth, bool insertSpaces, QWidget *parent)
    : QDialog(parent), m_font(font), m_theme(theme), m_tabWidth(tabWidth), m_insertSpaces(insertSpaces)
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