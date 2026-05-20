/*
 * prefs_dialog.cpp - Preferences Dialog
 */

#include "prefs_dialog.h"
#include "behavior_page.h"
#include "database_page.h"
#include "scanner_page.h"
#include "exclusions_page.h"
#include "scheduled_page.h"
#include "virustotal_page.h"
#include "debug_page.h"
#include "../../core/profiles.h"
#include "../../core/settings_manager.h"
#include "../icon_utils.h"

#include <tdelocale.h>

PrefsDialog::PrefsDialog(TQWidget *parent, SettingsManager *settings, ProfileManager *profiles, int initialPage)
    : KDialogBase(IconList, i18n("Preferences"), Help | Default | Ok | Apply | Cancel, Ok, parent, "prefs_dialog", true, true)
{
    if (settings) {
        m_settings = settings;
        m_ownsSettings = false;
    } else {
        m_settings = new SettingsManager();
        m_ownsSettings = true;
    }
    m_profiles = profiles;
    
    setIcon(IconUtils::load(main_icon_png, main_icon_png_len));
    setMinimumSize(750, 450);
    resize(750, 450);
    
    setupPages();
    loadSettings();
    
    if (initialPage >= 0) {
        showPage(initialPage);
    }
}

PrefsDialog::~PrefsDialog()
{
    if (m_ownsSettings) {
        delete m_settings;
    }
}

#include <kiconloader.h>
#include <ntqvbox.h>
#include "../icon_utils.h"

void PrefsDialog::setupPages()
{
    TQVBox *page1 = addVBoxPage(i18n("Behavior"), i18n("Behavior"), IconUtils::load(system_png, system_png_len, 32, 32));
    m_behaviorPage = new BehaviorPage(page1);
    
    TQVBox *page2 = addVBoxPage(i18n("Database"), i18n("Database"), IconUtils::load(network_server_symbolic_png, network_server_symbolic_png_len, 32, 32));
    m_databasePage = new DatabasePage(page2);
    
    TQVBox *page3 = addVBoxPage(i18n("Scanner"), i18n("Scanner"), IconUtils::load(find_png, find_png_len, 32, 32));
    m_scannerPage = new ScannerPage(page3);
    
    TQVBox *page4 = addVBoxPage(i18n("Exclusions"), i18n("Exclusions"), IconUtils::load(exclude_png, exclude_png_len, 32, 32));
    m_exclusionsPage = new ExclusionsPage(page4);
    
    TQVBox *page5 = addVBoxPage(i18n("Scheduled"), i18n("Scheduled"), IconUtils::load(schedule_png, schedule_png_len, 32, 32));
    m_scheduledPage = new ScheduledPage(page5, m_profiles);
    
    TQVBox *page6 = addVBoxPage(i18n("VirusTotal"), i18n("VirusTotal"), IconUtils::load(virustotal_png, virustotal_png_len, 32, 32));
    m_virusTotalPage = new VirusTotalPage(page6);
    
    TQVBox *page7 = addVBoxPage(i18n("Debug"), i18n("Debug"), IconUtils::load(debug_png, debug_png_len, 32, 32));
    m_debugPage = new DebugPage(page7);
}

void PrefsDialog::loadSettings()
{
    static_cast<BehaviorPage*>(m_behaviorPage)->loadSettings(m_settings);
    static_cast<DatabasePage*>(m_databasePage)->loadSettings(m_settings);
    static_cast<ScannerPage*>(m_scannerPage)->loadSettings(m_settings);
    static_cast<ExclusionsPage*>(m_exclusionsPage)->loadSettings(m_settings);
    static_cast<ScheduledPage*>(m_scheduledPage)->loadSettings(m_settings);
    static_cast<VirusTotalPage*>(m_virusTotalPage)->loadSettings(m_settings);
    static_cast<DebugPage*>(m_debugPage)->loadSettings(m_settings);
}

void PrefsDialog::saveSettings()
{
    static_cast<BehaviorPage*>(m_behaviorPage)->saveSettings(m_settings);
    static_cast<DatabasePage*>(m_databasePage)->saveSettings(m_settings);
    static_cast<ScannerPage*>(m_scannerPage)->saveSettings(m_settings);
    static_cast<ExclusionsPage*>(m_exclusionsPage)->saveSettings(m_settings);
    static_cast<ScheduledPage*>(m_scheduledPage)->saveSettings(m_settings);
    static_cast<VirusTotalPage*>(m_virusTotalPage)->saveSettings(m_settings);
    static_cast<DebugPage*>(m_debugPage)->saveSettings(m_settings);
    
    // If we don't own settings, we must sync them explicitly to file
    // and emit the settingsChanged signal so the rest of the app updates
    m_settings->save();
}

void PrefsDialog::slotOk()
{
    saveSettings();
    KDialogBase::slotOk();
}

void PrefsDialog::slotApply()
{
    saveSettings();
    KDialogBase::slotApply();
}

void PrefsDialog::slotDefault()
{
    // Real app would reset to defaults
    KDialogBase::slotDefault();
}

#include "prefs_dialog.moc"
