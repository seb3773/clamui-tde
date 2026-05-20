/*
 * scan_results_dialog.cpp - Post-scan summary dialog
 */

#include "scan_results_dialog.h"
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqlistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include "../core/quarantine_manager.h"
#include "icon_utils.h"

ScanResultsDialog::ScanResultsDialog(TQWidget *parent, const TQStringList &targetPaths, int filesScanned, int durationSecs, TQListView *threatList, QuarantineManager *quarantineManager)
    : TQDialog(parent, "scan_results", true), m_sourceThreatList(threatList), m_quarantineManager(quarantineManager)
{
    int threatsFound = threatList ? threatList->childCount() : 0;
    
    setCaption(i18n("Scan Results"));
    setIcon(IconUtils::load(main_icon_png, main_icon_png_len));
    resize(600, 450);
    
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQHBoxLayout *titleLayout = new TQHBoxLayout(6);
    titleLayout->addStretch(1);
    
    TQLabel *iconLabel = new TQLabel(this);
    if (threatsFound > 0) {
        iconLabel->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQColor(231, 76, 60), 24, 24)); // Soft red
    } else {
        iconLabel->setPixmap(IconUtils::colorize(check_png, check_png_len, TQColor(39, 174, 96), 24, 24)); // Soft green
    }
    titleLayout->addWidget(iconLabel);
    
    TQString titleText = threatsFound > 0 
        ? i18n("<h2 style='color:#e74c3c; margin:0;'>Threats Detected!</h2>") 
        : i18n("<h2 style='color:#27ae60; margin:0;'>System is Clean</h2>");
        
    TQLabel *titleLabel = new TQLabel(titleText, this);
    titleLabel->setAlignment(TQt::AlignVCenter | TQt::AlignLeft);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch(1);
    mainLayout->addLayout(titleLayout);
    
    TQLabel *statsLabel = new TQLabel(this);
    TQString stats;
    stats += i18n("<b>Target Paths:</b><br>");
    for (int i = 0; i < (int)targetPaths.count(); ++i) {
        TQString path = targetPaths[i];
        path.replace("&", "&amp;");
        path.replace("<", "&lt;");
        path.replace(">", "&gt;");
        stats += "<b>" + path + "</b><br>";
    }
    stats += "<br>";
    
    stats += i18n("<b>Files Scanned:</b> <font color=\"#3498db\"><b>%1</b></font><br>").arg(filesScanned);
    
    if (threatsFound == 0) {
        stats += i18n("<b>Threats Found:</b> <font color=\"#27ae60\"><b>%1</b></font><br>").arg(threatsFound);
    } else {
        stats += i18n("<b>Threats Found:</b> <font color=\"#e74c3c\"><b>%1</b></font><br>").arg(threatsFound);
    }
    
    TQString timeStr;
    if (durationSecs < 60) {
        timeStr = i18n("%1 sec").arg(durationSecs);
    } else if (durationSecs < 3600) {
        timeStr = i18n("%1 min %2 sec").arg(durationSecs / 60).arg(durationSecs % 60);
    } else {
        timeStr = i18n("%1 hours %2 min %3 sec").arg(durationSecs / 3600).arg((durationSecs % 3600) / 60).arg(durationSecs % 60);
    }
    stats += i18n("<b>Time Elapsed:</b> <i><b>%1</b></i><br>").arg(timeStr);
    
    statsLabel->setText(stats);
    statsLabel->setAlignment(TQt::AlignCenter);
    mainLayout->addWidget(statsLabel);
    
    if (threatsFound > 0) {
        m_resultsList = new TQListView(this);
        m_resultsList->addColumn(i18n("File Path"));
        m_resultsList->addColumn(i18n("Threat Name"));
        m_resultsList->setAllColumnsShowFocus(true);
        m_resultsList->setSelectionMode(TQListView::Single);
        
        if (m_sourceThreatList) {
            TQListViewItem *item = m_sourceThreatList->firstChild();
            while (item) {
                new TQListViewItem(m_resultsList, item->text(0), item->text(1));
                item = item->nextSibling();
            }
        }
        
        mainLayout->addWidget(m_resultsList);
        
        connect(m_resultsList, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(itemSelectionChanged()));
    } else {
        m_resultsList = 0;
        mainLayout->addStretch(1);
    }
    
    TQHBoxLayout *btnLayout = new TQHBoxLayout(mainLayout, 6);
    
    if (threatsFound > 0 && m_quarantineManager) {
        m_btnQuarantineSelected = new TQPushButton(i18n("Quarantine Selected"), this);
        connect(m_btnQuarantineSelected, TQT_SIGNAL(clicked()), this, TQT_SLOT(quarantineSelected()));
        btnLayout->addWidget(m_btnQuarantineSelected);
        
        m_btnQuarantineAll = new TQPushButton(i18n("Quarantine All"), this);
        m_btnQuarantineAll->setPaletteBackgroundColor(TQColor(255, 200, 200));
        connect(m_btnQuarantineAll, TQT_SIGNAL(clicked()), this, TQT_SLOT(quarantineAll()));
        btnLayout->addWidget(m_btnQuarantineAll);
    } else {
        m_btnQuarantineSelected = 0;
        m_btnQuarantineAll = 0;
    }
    
    btnLayout->addStretch(1);
    
    TQPushButton *btnGoToLogs = new TQPushButton(i18n("Close and go to log"), this);
    connect(btnGoToLogs, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotGoToLogs()));
    btnLayout->addWidget(btnGoToLogs);
    
    TQPushButton *btnOk = new TQPushButton(i18n("&Close"), this);
    connect(btnOk, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()));
    btnLayout->addWidget(btnOk);
    btnOk->setFocus();
    
    updateButtons();
}

ScanResultsDialog::~ScanResultsDialog()
{
}

void ScanResultsDialog::itemSelectionChanged()
{
    updateButtons();
}

void ScanResultsDialog::updateButtons()
{
    if (!m_resultsList) return;
    
    bool hasItems = m_resultsList->childCount() > 0;
    bool hasSelection = m_resultsList->selectedItem() != 0;
    
    if (m_btnQuarantineAll) m_btnQuarantineAll->setEnabled(hasItems);
    if (m_btnQuarantineSelected) m_btnQuarantineSelected->setEnabled(hasSelection);
}

void ScanResultsDialog::quarantineSelected()
{
    if (!m_resultsList || !m_quarantineManager) return;
    
    TQListViewItem *item = m_resultsList->selectedItem();
    if (!item) return;
    
    TQString filePath = item->text(0);
    TQString threatName = item->text(1);
    
    if (m_quarantineManager->quarantineFile(filePath, threatName)) {
        delete item;
        updateButtons();
    } else {
        KMessageBox::error(this, i18n("Failed to quarantine file: %1").arg(filePath), i18n("Quarantine Error"));
    }
}

void ScanResultsDialog::quarantineAll()
{
    if (!m_resultsList || !m_quarantineManager) return;
    
    int successCount = 0;
    int errorCount = 0;
    
    TQListViewItem *item = m_resultsList->firstChild();
    while (item) {
        TQListViewItem *next = item->nextSibling();
        
        TQString filePath = item->text(0);
        TQString threatName = item->text(1);
        
        if (m_quarantineManager->quarantineFile(filePath, threatName)) {
            delete item;
            successCount++;
        } else {
            errorCount++;
        }
        
        item = next;
    }
    
    updateButtons();
    
    if (errorCount == 0 && successCount > 0) {
        KMessageBox::information(this, i18n("Successfully quarantined %1 threat(s).").arg(successCount), i18n("Quarantine Success"));
    } else if (errorCount > 0) {
        KMessageBox::sorry(this, i18n("Quarantined %1 threat(s), but failed to quarantine %2 threat(s).").arg(successCount).arg(errorCount), i18n("Quarantine Partial Success"));
    }
}

void ScanResultsDialog::slotGoToLogs()
{
    done(10);
}

#include "scan_results_dialog.moc"
