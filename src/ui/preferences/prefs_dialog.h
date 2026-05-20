/*
 * prefs_dialog.h - Preferences Dialog
 */

#ifndef CLAM_PREFS_DIALOG_H
#define CLAM_PREFS_DIALOG_H

#include <kdialogbase.h>

class SettingsManager;
class ProfileManager;

class PrefsDialog : public KDialogBase
{
    TQ_OBJECT

public:
    PrefsDialog(TQWidget *parent = 0, SettingsManager *settings = 0, ProfileManager *profiles = 0, int initialPage = -1);
    ~PrefsDialog();

protected slots:
    virtual void slotOk();
    virtual void slotApply();
    virtual void slotDefault();

private:
    SettingsManager *m_settings;
    ProfileManager  *m_profiles;
    bool m_ownsSettings;
    
    TQWidget *m_behaviorPage;
    TQWidget *m_databasePage;
    TQWidget *m_scannerPage;
    TQWidget *m_exclusionsPage;
    TQWidget *m_scheduledPage;
    TQWidget *m_virusTotalPage;
    TQWidget *m_debugPage;
    
    void setupPages();
    void loadSettings();
    void saveSettings();
};

#endif /* CLAM_PREFS_DIALOG_H */
