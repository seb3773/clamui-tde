/*
 * scheduled_page.h - Scheduled Scans Preferences Page
 */

#ifndef CLAM_SCHEDULED_PAGE_H
#define CLAM_SCHEDULED_PAGE_H

#include <tqwidget.h>
#include <ntqstringlist.h>

class TQCheckBox;
class TQComboBox;
class TQTimeEdit;
class TQLabel;
class SettingsManager;
class ProfileManager;

class ScheduledPage : public TQWidget
{
    TQ_OBJECT

public:
    ScheduledPage(TQWidget *parent = 0, ProfileManager *profileManager = 0);
    ~ScheduledPage();

    void loadSettings(SettingsManager *settings);
    void saveSettings(SettingsManager *settings);

private slots:
    void onEnableToggled(bool enabled);
    void reloadProfiles();

private:
    ProfileManager *m_profileManager;
    TQStringList    m_profileIds;

    TQCheckBox *m_enableScheduleCb;
    TQCheckBox *m_batteryCb;
    TQLabel    *m_freqLabel;
    TQComboBox *m_frequencyCombo;
    TQLabel    *m_timeLabel;
    TQTimeEdit *m_timeEdit;
    TQLabel    *m_profileLabel;
    TQComboBox *m_profileCombo;
};

#endif /* CLAM_SCHEDULED_PAGE_H */
