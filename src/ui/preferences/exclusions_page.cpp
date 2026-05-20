/*
 * exclusions_page.cpp - Exclusions Preferences Page
 */

#include "exclusions_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqtextedit.h>
#include <ntqstringlist.h>
#include <tdelocale.h>

ExclusionsPage::ExclusionsPage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    mainLayout->addWidget(new TQLabel(i18n("Global Exclusions (one path per line):"), this));
    
    m_exclusionsEdit = new TQTextEdit(this);
    m_exclusionsEdit->setTextFormat(TQt::PlainText);
    mainLayout->addWidget(m_exclusionsEdit);
}

ExclusionsPage::~ExclusionsPage()
{
}

void ExclusionsPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    m_exclusionsEdit->setText(settings->value("global_exclusions", "").replace(";", "\n"));
}

void ExclusionsPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    TQStringList lines = TQStringList::split("\n", m_exclusionsEdit->text());
    settings->setValue("global_exclusions", lines.join(";"));
}

#include "exclusions_page.moc"
