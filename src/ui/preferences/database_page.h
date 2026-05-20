/*
 * database_page.h - Database Preferences Page
 */

#ifndef CLAM_DATABASE_PAGE_H
#define CLAM_DATABASE_PAGE_H

#include <tqwidget.h>

class TQSpinBox;
class TQComboBox;
class TQCheckBox;
class SettingsManager;

class DatabasePage : public TQWidget
{
    TQ_OBJECT

public:
    DatabasePage(TQWidget *parent = 0);
    ~DatabasePage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private slots:
    void onUnofficialSigsToggled(bool checked);

private:
    TQComboBox *m_mirrorCombo;
    TQSpinBox *m_checkFreqSpin;
    TQCheckBox *m_unofficialSigsCheck;
    TQCheckBox *m_sanesecurityCheck;
    TQCheckBox *m_urlhausCheck;
};

#endif /* CLAM_DATABASE_PAGE_H */
