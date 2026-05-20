/*
 * scanner_page.h - Scanner Preferences Page
 */

#ifndef CLAM_SCANNER_PAGE_H
#define CLAM_SCANNER_PAGE_H

#include <tqwidget.h>

class TQComboBox;
class TQSpinBox;
class TQCheckBox;
class TQLabel;
class TQLineEdit;
class TQPushButton;
class SettingsManager;

class ScannerPage : public TQWidget
{
    TQ_OBJECT

public:
    ScannerPage(TQWidget *parent = 0);
    ~ScannerPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private slots:
    void onBackendChanged(int index);
    void browseQuarantineDir();

private:
    TQComboBox *m_backendCombo;
    TQSpinBox *m_maxFileSizeSpin;
    TQCheckBox *m_scanArchivesCb;
    TQCheckBox *m_heuristicCb;
    TQComboBox *m_priorityCombo;
    TQLabel *m_priorityLabel;
    
    TQLineEdit *m_quarantineDirEdit;
    TQPushButton *m_btnBrowseQuarantine;
};

#endif /* CLAM_SCANNER_PAGE_H */
