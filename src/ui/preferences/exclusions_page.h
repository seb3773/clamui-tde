/*
 * exclusions_page.h - Exclusions Preferences Page
 */

#ifndef CLAM_EXCLUSIONS_PAGE_H
#define CLAM_EXCLUSIONS_PAGE_H

#include <tqwidget.h>

class TQTextEdit;
class SettingsManager;

class ExclusionsPage : public TQWidget
{
    TQ_OBJECT

public:
    ExclusionsPage(TQWidget *parent = 0);
    ~ExclusionsPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private:
    TQTextEdit *m_exclusionsEdit;
};

#endif /* CLAM_EXCLUSIONS_PAGE_H */
