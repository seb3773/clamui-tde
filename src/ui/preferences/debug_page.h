/*
 * debug_page.h - Debug Preferences Page
 */

#ifndef CLAM_DEBUG_PAGE_H
#define CLAM_DEBUG_PAGE_H

#include <tqwidget.h>

class TQCheckBox;
class SettingsManager;

class DebugPage : public TQWidget
{
    TQ_OBJECT

public:
    DebugPage(TQWidget *parent = 0);
    ~DebugPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private:
    TQCheckBox *m_enableDebugCb;
};

#endif /* CLAM_DEBUG_PAGE_H */
