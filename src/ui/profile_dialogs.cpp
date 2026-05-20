/*
 * profile_dialogs.cpp - Scan Profile Configuration Dialogs
 */

#include "profile_dialogs.h"
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqtextedit.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>
#include <tdelocale.h>
#include "icon_utils.h"

ProfileEditorDialog::ProfileEditorDialog(TQWidget *parent, const ScanProfile &profile)
    : TQDialog(parent, "profile_editor", true), m_profile(profile)
{
    setCaption(m_profile.id.isEmpty() ? i18n("New Scan Profile") : i18n("Edit Scan Profile"));
    setIcon(IconUtils::load(main_icon_png, main_icon_png_len));
    resize(450, 400);
    
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQHBoxLayout *nameLayout = new TQHBoxLayout(mainLayout, 6);
    nameLayout->addWidget(new TQLabel(i18n("Name:"), this));
    m_nameEdit = new TQLineEdit(m_profile.name, this);
    nameLayout->addWidget(m_nameEdit, 1);
    
    TQHBoxLayout *descLayout = new TQHBoxLayout(mainLayout, 6);
    descLayout->addWidget(new TQLabel(i18n("Description:"), this));
    m_descEdit = new TQLineEdit(m_profile.description, this);
    descLayout->addWidget(m_descEdit, 1);
    
    mainLayout->addWidget(new TQLabel(i18n("Target Paths (one per line):"), this));
    m_pathsEdit = new TQTextEdit(this);
    m_pathsEdit->setTextFormat(TQt::PlainText);
    m_pathsEdit->setText(m_profile.paths.join("\n"));
    mainLayout->addWidget(m_pathsEdit);
    
    mainLayout->addWidget(new TQLabel(i18n("Exclusions (one per line):"), this));
    m_exclusionsEdit = new TQTextEdit(this);
    m_exclusionsEdit->setTextFormat(TQt::PlainText);
    m_exclusionsEdit->setText(m_profile.exclusions.join("\n"));
    mainLayout->addWidget(m_exclusionsEdit);
    
    m_scanArchivesCb = new TQCheckBox(i18n("Scan Archives"), this);
    m_scanArchivesCb->setChecked(m_profile.scanArchives);
    mainLayout->addWidget(m_scanArchivesCb);
    
    m_heuristicCb = new TQCheckBox(i18n("Heuristic Scan"), this);
    m_heuristicCb->setChecked(m_profile.heuristicScan);
    mainLayout->addWidget(m_heuristicCb);
    
    TQHBoxLayout *btnLayout = new TQHBoxLayout(mainLayout, 6);
    btnLayout->addStretch(1);
    
    TQPushButton *btnCancel = new TQPushButton(i18n("&Cancel"), this);
    connect(btnCancel, TQT_SIGNAL(clicked()), this, TQT_SLOT(reject()));
    btnLayout->addWidget(btnCancel);
    
    TQPushButton *btnSave = new TQPushButton(i18n("&Save"), this);
    connect(btnSave, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()));
    btnLayout->addWidget(btnSave);
    btnSave->setDefault(true);
}

ProfileEditorDialog::~ProfileEditorDialog()
{
}

ScanProfile ProfileEditorDialog::getProfile() const
{
    ScanProfile p = m_profile;
    p.name = m_nameEdit->text();
    p.description = m_descEdit->text();
    p.paths = TQStringList::split("\n", m_pathsEdit->text());
    p.exclusions = TQStringList::split("\n", m_exclusionsEdit->text());
    p.scanArchives = m_scanArchivesCb->isChecked();
    p.heuristicScan = m_heuristicCb->isChecked();
    return p;
}

#include "profile_dialogs.moc"
