#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "defines.h"

#include <QDebug>
#include <QFontDialog>
#include <QColorDialog>
#include <QDir>

PreferencesDialog::PreferencesDialog(QSettings *appSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    settings = appSettings;
    ui->setupUi(this);

#ifdef Q_WS_WIN
    QFont font = ui->infoLabel1->font();
    font.setPointSize(7);
    ui->infoLabel1->setFont(font);
#endif

    setupConnections();
    updateUIFromSettings();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::setupConnections()
{
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(rejected()));
    connect(ui->fontButton, SIGNAL(clicked()), this, SLOT(fontButtonClicked()));
    connect(ui->highightLineColorButton, SIGNAL(clicked()), this, SLOT(lineHighlightColorButtonClicked()));
}

void PreferencesDialog::setFontToLabel(QFont font)
{
    ui->fontLabel->setFont(font);
    QString sizeStr;
    if (font.pixelSize() > -1)
        sizeStr = QVariant(font.pixelSize()).toString()+" px";
    else
        sizeStr = QVariant(font.pointSize()).toString()+" pt";
    ui->fontLabel->setText(font.family()+" "+sizeStr);
}


// Some helper macros
#define PREF_TO_UI_INT(pref, def, elem) elem->setValue(settings->value(pref, QVariant(def)).toInt())
#define PREF_TO_UI_DOUBLE(pref, def, elem) elem->setValue(settings->value(pref, QVariant(def)).toDouble())
#define PREF_TO_UI_BOOL_CHECKBOX(pref, def, elem) elem->setChecked(settings->value(pref, QVariant(def)).toBool())

void PreferencesDialog::updateUIFromSettings()
{
    // font
    QFont font;
    if (settings->contains(SETTING_FONT))
        font.fromString(settings->value(SETTING_FONT).toString());
    else {
        font.setFamily(DEF_FONT_FAMILY);
        font.setPointSize(DEF_FONT_SIZE);
        font.setFixedPitch(true);
    }
    setFontToLabel(font);

    // line highlight color
    QColor lineHighlightColor = settings->value(SETTING_LINE_HIGHLIGHT_COLOR,
                                                QVariant(DEF_LINE_HIGHLIGHT_COLOR)).value<QColor>();
    QPalette palette = ui->highlightLineColorLabel->palette();
    palette.setColor(ui->highlightLineColorLabel->backgroundRole(), lineHighlightColor);
    ui->highlightLineColorLabel->setPalette(palette);

    // styles
    QString highlightingStyle = settings->value(SETTING_STYLE,
                                                QVariant(DEF_STYLE)).toString();
    ui->stylesComboBox->clear();
    int i = 0;
    foreach (QString style, QDir(":/styles/").entryList())
    {
        ui->stylesComboBox->addItem(style);
        if (style == highlightingStyle)
            ui->stylesComboBox->setCurrentIndex(i);
        i++;
    }

    // others
    PREF_TO_UI_INT(SETTING_TAB_WIDTH, DEF_TAB_WIDTH, ui->tabWidthSpinBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_INDENT_WITH_TABS, DEF_INDENT_WITH_TABS, ui->tabsWithSpacesCheckBox);
    PREF_TO_UI_DOUBLE(SETTING_HIGHLIGHT_INTERVAL, DEF_HIGHLIGHT_INTERVAL, ui->highlightIntervalSpinBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_REMEMBER_LAST_FILE, DEF_REMEMBER_LAST_FILE, ui->rememberLastFileCheckBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_CLICKABLE_LINKS, DEF_CLICKABLE_LINKS, ui->linksClickableCheckBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_HIGHLIGHT_CURRENT_LINE, DEF_HIGHLIGHT_CURRENT_LINE, ui->highlightLineCheckBox);
}

void PreferencesDialog::updateSettingsFromUI()
{
    settings->setValue(SETTING_FONT, ui->fontLabel->font().toString());
    settings->setValue(SETTING_TAB_WIDTH, ui->tabWidthSpinBox->value());
    settings->setValue(SETTING_HIGHLIGHT_INTERVAL, ui->highlightIntervalSpinBox->value());
    settings->setValue(SETTING_INDENT_WITH_TABS, ui->tabsWithSpacesCheckBox->isChecked());
    settings->setValue(SETTING_REMEMBER_LAST_FILE, ui->rememberLastFileCheckBox->isChecked());
    settings->setValue(SETTING_CLICKABLE_LINKS, ui->linksClickableCheckBox->isChecked());
    settings->setValue(SETTING_HIGHLIGHT_CURRENT_LINE, ui->highlightLineCheckBox->isChecked());
    settings->setValue(SETTING_LINE_HIGHLIGHT_COLOR, ui->highlightLineColorLabel->palette().background().color());
    settings->setValue(SETTING_STYLE, ui->stylesComboBox->currentText());
    settings->sync();
}

void PreferencesDialog::fontButtonClicked()
{
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, ui->fontLabel->font(),
                                         this, tr("Select New Font"));
    if (!ok)
        return;
    setFontToLabel(newFont);
}

void PreferencesDialog::lineHighlightColorButtonClicked()
{
    QColor currColor = ui->highlightLineColorLabel->palette().background().color();
    QColor newColor = QColorDialog::getColor(currColor, this);
    QPalette palette = ui->highlightLineColorLabel->palette();
    palette.setColor(ui->highlightLineColorLabel->backgroundRole(), newColor);
    ui->highlightLineColorLabel->setPalette(palette);
}

void PreferencesDialog::accepted()
{
    updateSettingsFromUI();
    emit updated();
}

void PreferencesDialog::rejected()
{
    updateUIFromSettings();
}
