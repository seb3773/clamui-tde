/*
 * behavior_page.cpp - Behavior Preferences Page
 */

#include "behavior_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tdelocale.h>

BehaviorPage::BehaviorPage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    m_darkModeCb = new TQCheckBox(i18n("Enable dark mode for interface frames (cards)"), this);
    mainLayout->addWidget(m_darkModeCb);
    
    m_startMinimizedCb = new TQCheckBox(i18n("Start minimized on login"), this);
    mainLayout->addWidget(m_startMinimizedCb);
    
    m_minimizeToTrayCb = new TQCheckBox(i18n("Minimize to system tray instead of closing"), this);
    mainLayout->addWidget(m_minimizeToTrayCb);
    
    TQHBoxLayout *notifLayout = new TQHBoxLayout(mainLayout, 10);
    notifLayout->addWidget(new TQLabel(i18n("Notifications:"), this));
    
    m_notifyDaemonCb = new TQCheckBox(i18n("System Notification Daemon (libnotify)"), this);
    notifLayout->addWidget(m_notifyDaemonCb);
    
    m_notifyInternalCb = new TQCheckBox(i18n("Internal Popup"), this);
    notifLayout->addWidget(m_notifyInternalCb);
    
    notifLayout->addStretch(1);
    
    mainLayout->addStretch(1);
}

BehaviorPage::~BehaviorPage()
{
}

void BehaviorPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    m_darkModeCb->setChecked(settings->value("dark_mode", "false") == "true");
    m_startMinimizedCb->setChecked(settings->value("start_minimized", "false") == "true");
    
    // Support either key to be backward compatible
    TQString behavior = settings->value("close_behavior", "quit");
    bool minimize = (settings->value("minimize_to_tray", "false") == "true") || (behavior == "minimize");
    m_minimizeToTrayCb->setChecked(minimize);
    
    m_notifyDaemonCb->setChecked(settings->valueBool("notify_daemon", false));
    m_notifyInternalCb->setChecked(settings->valueBool("notify_internal", true));
}

void BehaviorPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    settings->setValue("dark_mode", m_darkModeCb->isChecked() ? "true" : "false");
    settings->setValue("start_minimized", m_startMinimizedCb->isChecked() ? "true" : "false");
    settings->setValue("minimize_to_tray", m_minimizeToTrayCb->isChecked() ? "true" : "false");
    
    // Write close_behavior to keep sync with both logic parts just in case
    settings->setValue("close_behavior", m_minimizeToTrayCb->isChecked() ? "minimize" : "quit");
    
    settings->setValue("notify_daemon", m_notifyDaemonCb->isChecked() ? "true" : "false");
    settings->setValue("notify_internal", m_notifyInternalCb->isChecked() ? "true" : "false");
}

#include "behavior_page.moc"
