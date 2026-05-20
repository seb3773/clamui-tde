/*
 * debug_page.cpp - Debug Preferences Page
 */

#include "debug_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tdelocale.h>

DebugPage::DebugPage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    m_enableDebugCb = new TQCheckBox(i18n("Enable verbose debug logging"), this);
    mainLayout->addWidget(m_enableDebugCb);
    
    TQLabel *descLabel = new TQLabel(i18n("Verbose logging will write detailed runtime diagnostics to ~/.local/share/tdeclamui/debug.log"), this);
    descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(descLabel);
    
    mainLayout->addStretch(1);
}

DebugPage::~DebugPage()
{
}

void DebugPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    m_enableDebugCb->setChecked(settings->value("debug_logging", "false") == "true");
}

void DebugPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    settings->setValue("debug_logging", m_enableDebugCb->isChecked() ? "true" : "false");
}

#include "debug_page.moc"
