/*
 * scheduled_page.cpp - Scheduled Scans Preferences Page
 */

#include "scheduled_page.h"
#include "../../core/settings_manager.h"
#include "../../core/profiles.h"
#include "../../core/scheduler.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <ntqdatetimeedit.h>
#include <tdelocale.h>
#include <tdemessagebox.h>

ScheduledPage::ScheduledPage(TQWidget *parent, ProfileManager *profileManager)
    : TQWidget(parent), m_profileManager(profileManager)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    m_enableScheduleCb = new TQCheckBox(i18n("Enable automatic scheduled scans"), this);
    mainLayout->addWidget(m_enableScheduleCb);
    
    m_batteryCb = new TQCheckBox(i18n("Do not scan when running on battery power"), this);
    mainLayout->addWidget(m_batteryCb);
    
    TQHBoxLayout *freqLayout = new TQHBoxLayout(mainLayout, 6);
    m_freqLabel = new TQLabel(i18n("Frequency:"), this);
    freqLayout->addWidget(m_freqLabel);
    
    m_frequencyCombo = new TQComboBox(this);
    m_frequencyCombo->insertItem(i18n("Daily"));
    m_frequencyCombo->insertItem(i18n("Weekly"));
    m_frequencyCombo->insertItem(i18n("Monthly"));
    freqLayout->addWidget(m_frequencyCombo, 1);
    
    TQHBoxLayout *timeLayout = new TQHBoxLayout(mainLayout, 6);
    m_timeLabel = new TQLabel(i18n("Time:"), this);
    timeLayout->addWidget(m_timeLabel);
    
    m_timeEdit = new TQTimeEdit(this);
    timeLayout->addWidget(m_timeEdit, 1);
    
    TQHBoxLayout *profileLayout = new TQHBoxLayout(mainLayout, 6);
    m_profileLabel = new TQLabel(i18n("Profile to run:"), this);
    profileLayout->addWidget(m_profileLabel);
    
    m_profileCombo = new TQComboBox(this);
    if (m_profileManager) {
        TQValueList<ScanProfile> profiles = m_profileManager->getProfiles();
        for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
            m_profileCombo->insertItem((*it).name);
        }
        // Store profile IDs for lookup
        m_profileIds.clear();
        for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
            m_profileIds.append((*it).id);
        }
    } else {
        m_profileCombo->insertItem(i18n("Full Scan"));
        m_profileCombo->insertItem(i18n("System Scan"));
    }
    profileLayout->addWidget(m_profileCombo, 1);
    
    mainLayout->addStretch(1);
    
    /* Grey out controls when scheduling is disabled */
    connect(m_enableScheduleCb, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onEnableToggled(bool)));
    
    if (m_profileManager) {
        connect(m_profileManager, TQT_SIGNAL(profilesUpdated()), this, TQT_SLOT(reloadProfiles()));
    }
    
    /* Initial state: disabled until loadSettings sets the checkbox */
    onEnableToggled(false);
}

ScheduledPage::~ScheduledPage()
{
}

void ScheduledPage::onEnableToggled(bool enabled)
{
    m_batteryCb->setEnabled(enabled);
    m_freqLabel->setEnabled(enabled);
    m_frequencyCombo->setEnabled(enabled);
    m_timeLabel->setEnabled(enabled);
    m_timeEdit->setEnabled(enabled);
    m_profileLabel->setEnabled(enabled);
    m_profileCombo->setEnabled(enabled);
}

void ScheduledPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    
    bool isEnabled = (settings->value("schedule_enabled", "false") == "true");
    m_enableScheduleCb->setChecked(isEnabled);
    
    bool skipBattery = (settings->value("schedule_skip_battery", "true") == "true");
    m_batteryCb->setChecked(skipBattery);
    
    TQString freq = settings->value("schedule_frequency", "daily");
    if (freq == "weekly") m_frequencyCombo->setCurrentItem(1);
    else if (freq == "monthly") m_frequencyCombo->setCurrentItem(2);
    else m_frequencyCombo->setCurrentItem(0);
    
    // Load saved time (HH:MM format)
    TQString timeStr = settings->value("schedule_time", "02:00");
    TQTime t = TQTime::fromString(timeStr);
    if (t.isValid()) {
        m_timeEdit->setTime(t);
    }
    
    // Select profile by ID
    TQString profileId = settings->value("schedule_profile_id", "");
    if (!profileId.isEmpty()) {
        for (int i = 0; i < (int)m_profileIds.count(); ++i) {
            if (m_profileIds[i] == profileId) {
                m_profileCombo->setCurrentItem(i);
                break;
            }
        }
    }
}

void ScheduledPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    bool enabled = m_enableScheduleCb->isChecked();
    settings->setValue("schedule_enabled", enabled ? "true" : "false");
    
    bool skipBattery = m_batteryCb->isChecked();
    settings->setValue("schedule_skip_battery", skipBattery ? "true" : "false");
    
    TQString freq = "daily";
    if (m_frequencyCombo->currentItem() == 1) freq = "weekly";
    if (m_frequencyCombo->currentItem() == 2) freq = "monthly";
    settings->setValue("schedule_frequency", freq);
    
    // Save time
    TQTime t = m_timeEdit->time();
    TQString hh = TQString::number(t.hour()).rightJustify(2, '0');
    TQString mm = TQString::number(t.minute()).rightJustify(2, '0');
    settings->setValue("schedule_time", TQString("%1:%2").arg(hh).arg(mm));
    
    // Save selected profile ID
    int profileIdx = m_profileCombo->currentItem();
    if (profileIdx >= 0 && profileIdx < (int)m_profileIds.count()) {
        settings->setValue("schedule_profile_id", m_profileIds[profileIdx]);
    }
    
    // Apply via Scheduler
    Scheduler scheduler;
    if (enabled) {
        TQString profileId = (profileIdx >= 0 && profileIdx < (int)m_profileIds.count())
                            ? m_profileIds[profileIdx] : TQString::null;
        TQString timeStr = settings->value("schedule_time", "02:00");
        if (!scheduler.createSchedule(freq, timeStr, profileId, skipBattery)) {
            KMessageBox::error(this,
                i18n("Failed to create scheduled scan. Please check that systemd or cron is available."),
                i18n("Scheduler Error"));
        }
    } else {
        scheduler.removeSchedule();
    }
}

void ScheduledPage::reloadProfiles()
{
    TQString selectedId;
    int currentIdx = m_profileCombo->currentItem();
    if (currentIdx >= 0 && currentIdx < (int)m_profileIds.count()) {
        selectedId = m_profileIds[currentIdx];
    }
    
    m_profileCombo->clear();
    m_profileIds.clear();
    
    if (m_profileManager) {
        TQValueList<ScanProfile> profiles = m_profileManager->getProfiles();
        int newIdx = 0;
        int i = 0;
        for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
            m_profileCombo->insertItem((*it).name);
            m_profileIds.append((*it).id);
            if ((*it).id == selectedId) {
                newIdx = i;
            }
            i++;
        }
        m_profileCombo->setCurrentItem(newIdx);
    }
}

#include "scheduled_page.moc"
