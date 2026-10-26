#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFontDialog>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(const QFont &font, EditorTheme theme, QWidget *parent)
    : QDialog(parent), m_font(font), m_theme(theme)
{
    setWindowTitle(tr("Preferences"));
    resize(420, 200);
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
    themeComboBox->addItems({
        tr("Light"),
        tr("Dark (VS Code)"),
        tr("Monokai"),
        tr("Solarized Dark"),
        tr("Nord")
    });
    themeComboBox->setCurrentIndex(static_cast<int>(m_theme));
    formLayout->addRow(tr("Syntax Theme:"), themeComboBox);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // Dialog Buttons (OK / Cancel)
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connections
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