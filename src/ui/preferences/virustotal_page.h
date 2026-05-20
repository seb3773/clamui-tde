/*
 * virustotal_page.h - VirusTotal Preferences Page
 */

#ifndef CLAM_VIRUSTOTAL_PAGE_H
#define CLAM_VIRUSTOTAL_PAGE_H

#include <tqwidget.h>

class TQCheckBox;
class TQLineEdit;
class SettingsManager;

class VirusTotalPage : public TQWidget
{
    TQ_OBJECT

public:
    VirusTotalPage(TQWidget *parent = 0);
    ~VirusTotalPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private:
    TQCheckBox *m_enableVtCb;
    TQCheckBox *m_autoSubmitCb;
    TQLineEdit *m_apiKeyEdit;
};

#endif /* CLAM_VIRUSTOTAL_PAGE_H */
