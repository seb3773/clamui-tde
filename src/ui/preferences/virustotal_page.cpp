/*
 * virustotal_page.cpp - VirusTotal Preferences Page
 */

#include "virustotal_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tqlineedit.h>
#include <tdelocale.h>

VirusTotalPage::VirusTotalPage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    m_enableVtCb = new TQCheckBox(i18n("Enable VirusTotal integration"), this);
    mainLayout->addWidget(m_enableVtCb);
    
    m_autoSubmitCb = new TQCheckBox(i18n("Automatically submit unknown suspicious files"), this);
    mainLayout->addWidget(m_autoSubmitCb);
    
    mainLayout->addSpacing(10);
    mainLayout->addWidget(new TQLabel(i18n("API Key:"), this));
    
    m_apiKeyEdit = new TQLineEdit(this);
    m_apiKeyEdit->setEchoMode(TQLineEdit::Password);
    mainLayout->addWidget(m_apiKeyEdit);
    
    TQLabel *infoLabel = new TQLabel(i18n("<small>A free API key can be obtained by creating an account on virustotal.com</small>"), this);
    mainLayout->addWidget(infoLabel);
    
    mainLayout->addStretch(1);
}

VirusTotalPage::~VirusTotalPage()
{
}

void VirusTotalPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    m_enableVtCb->setChecked(settings->value("virustotal_enabled", "false") == "true");
    m_autoSubmitCb->setChecked(settings->value("virustotal_auto_submit", "false") == "true");
    
    // In a real app, API key should come from KeyringManager
    m_apiKeyEdit->setText(settings->value("virustotal_api_key", ""));
}

void VirusTotalPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    settings->setValue("virustotal_enabled", m_enableVtCb->isChecked() ? "true" : "false");
    settings->setValue("virustotal_auto_submit", m_autoSubmitCb->isChecked() ? "true" : "false");
    settings->setValue("virustotal_api_key", m_apiKeyEdit->text());
}

#include "virustotal_page.moc"
