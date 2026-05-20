/*
 * behavior_page.h - Behavior Preferences Page
 */

#ifndef CLAM_BEHAVIOR_PAGE_H
#define CLAM_BEHAVIOR_PAGE_H

#include <tqwidget.h>

class TQCheckBox;
class TQComboBox;
class SettingsManager;

class BehaviorPage : public TQWidget
{
    TQ_OBJECT

public:
    BehaviorPage(TQWidget *parent = 0);
    ~BehaviorPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private:
    TQCheckBox *m_darkModeCb;
    TQCheckBox *m_startMinimizedCb;
    TQCheckBox *m_minimizeToTrayCb;
    TQCheckBox *m_notifyDaemonCb;
    TQCheckBox *m_notifyInternalCb;
};

#endif /* CLAM_BEHAVIOR_PAGE_H */
