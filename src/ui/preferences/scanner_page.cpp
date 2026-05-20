/*
 * scanner_page.cpp - Scanner Preferences Page
 */

#include "scanner_page.h"
#include "../../core/settings_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tqspinbox.h>
#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>
#include <tdelocale.h>
#include <tdefiledialog.h>
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <kiconloader.h>

ScannerPage::ScannerPage(TQWidget *parent)
    : TQWidget(parent)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQHBoxLayout *backendLayout = new TQHBoxLayout(mainLayout, 6);
    backendLayout->addWidget(new TQLabel(i18n("Scan Engine Backend:"), this));
    
    m_backendCombo = new TQComboBox(this);
    m_backendCombo->insertItem(i18n("Auto (Prefers clamd)"));
    m_backendCombo->insertItem(i18n("clamd (Daemon - Fast)"));
    m_backendCombo->insertItem(i18n("clamscan (CLI - Slow)"));
    backendLayout->addWidget(m_backendCombo, 1);
    
    TQHBoxLayout *sizeLayout = new TQHBoxLayout(mainLayout, 6);
    sizeLayout->addWidget(new TQLabel(i18n("Max File Size (MB):"), this));
    
    m_maxFileSizeSpin = new TQSpinBox(1, 4000, 10, this);
    sizeLayout->addWidget(m_maxFileSizeSpin);
    sizeLayout->addStretch(1);
    
    m_scanArchivesCb = new TQCheckBox(i18n("Scan inside archives by default"), this);
    mainLayout->addWidget(m_scanArchivesCb);
    
    m_heuristicCb = new TQCheckBox(i18n("Enable heuristic scanning"), this);
    mainLayout->addWidget(m_heuristicCb);
    
    mainLayout->addSpacing(10);
    
    // Process priority (clamscan only)
    TQHBoxLayout *priorityLayout = new TQHBoxLayout(mainLayout, 6);
    m_priorityLabel = new TQLabel(i18n("Process Priority (clamscan only):"), this);
    priorityLayout->addWidget(m_priorityLabel);
    
    m_priorityCombo = new TQComboBox(this);
    m_priorityCombo->insertItem(i18n("Normal"));
    m_priorityCombo->insertItem(i18n("Low (nice 10, idle I/O)"));
    m_priorityCombo->insertItem(i18n("Very Low (nice 19, idle I/O)"));
    priorityLayout->addWidget(m_priorityCombo, 1);
    
    // Grey out priority when daemon mode is selected
    connect(m_backendCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onBackendChanged(int)));
    
    mainLayout->addSpacing(10);
    
    // Quarantine Directory
    TQHBoxLayout *quarLayout = new TQHBoxLayout(mainLayout, 6);
    quarLayout->addWidget(new TQLabel(i18n("Quarantine Folder:"), this));
    
    m_quarantineDirEdit = new TQLineEdit(this);
    quarLayout->addWidget(m_quarantineDirEdit, 1);
    
    m_btnBrowseQuarantine = new TQPushButton(SmallIcon("folder"), i18n("Browse..."), this);
    connect(m_btnBrowseQuarantine, TQT_SIGNAL(clicked()), this, TQT_SLOT(browseQuarantineDir()));
    quarLayout->addWidget(m_btnBrowseQuarantine);
    
    mainLayout->addStretch(1);
}

ScannerPage::~ScannerPage()
{
}

void ScannerPage::loadSettings(SettingsManager *settings)
{
    if (!settings) return;
    TQString backend = settings->value("scan_backend", "auto");
    if (backend == "daemon") m_backendCombo->setCurrentItem(1);
    else if (backend == "clamscan") m_backendCombo->setCurrentItem(2);
    else m_backendCombo->setCurrentItem(0);
    
    m_maxFileSizeSpin->setValue(settings->value("max_file_size", "25").toInt());
    m_scanArchivesCb->setChecked(settings->value("scan_archives", "true") == "true");
    m_heuristicCb->setChecked(settings->value("heuristic_scan", "true") == "true");
    
    m_priorityCombo->setCurrentItem(settings->value("scan_priority", "0").toInt());
    
    TQString defaultQuar = TDEGlobal::dirs()->saveLocation("appdata") + "quarantine";
    m_quarantineDirEdit->setText(settings->value("quarantine_path", defaultQuar));
    
    // Trigger backend-dependent UI state
    onBackendChanged(m_backendCombo->currentItem());
}

void ScannerPage::saveSettings(SettingsManager *settings)
{
    if (!settings) return;
    TQString backend = "auto";
    if (m_backendCombo->currentItem() == 1) backend = "daemon";
    if (m_backendCombo->currentItem() == 2) backend = "clamscan";
    
    settings->setValue("scan_backend", backend);
    settings->setValue("max_file_size", TQString::number(m_maxFileSizeSpin->value()));
    settings->setValue("scan_archives", m_scanArchivesCb->isChecked() ? "true" : "false");
    settings->setValue("heuristic_scan", m_heuristicCb->isChecked() ? "true" : "false");
    settings->setValue("scan_priority", TQString::number(m_priorityCombo->currentItem()));
    settings->setValue("quarantine_path", m_quarantineDirEdit->text());
}

void ScannerPage::onBackendChanged(int index)
{
    bool isDaemon = (index == 1); // "clamd (Daemon)"
    m_priorityCombo->setEnabled(!isDaemon);
    m_priorityLabel->setEnabled(!isDaemon);
}

void ScannerPage::browseQuarantineDir()
{
    TQString dir = KFileDialog::getExistingDirectory(
        m_quarantineDirEdit->text(),
        this,
        i18n("Select Quarantine Folder")
    );
    if (!dir.isEmpty()) {
        m_quarantineDirEdit->setText(dir);
    }
}

#include "scanner_page.moc"
