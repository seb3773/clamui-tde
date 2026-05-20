/*
 * audit_view.cpp - System Audit View
 */

#include "audit_view.h"
#include "mainwindow.h"
#include "../core/system_audit.h"
#include "../core/settings_manager.h"
#include "../core/log_manager.h"

#include <tqpushbutton.h>
#include <tqvbox.h>
#include <tqhbox.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqscrollview.h>
#include <tqframe.h>
#include <tqpixmap.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tqfont.h>
#include <tqclipboard.h>
#include <tqapplication.h>
#include <tqdatetime.h>
#include <tdemessagebox.h>
#include <tdeapplication.h>
#include <stdlib.h>
#include "icon_utils.h"

AuditView::AuditView(TQWidget *parent, TQWidgetStack *stack)
    : TQWidget(parent)
{
    m_audit = new SystemAudit(this);
    
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 10, 10);
    
    // Header
    m_headerWidget = new TQWidget(this);
    TQHBoxLayout *headerLayout = new TQHBoxLayout(m_headerWidget, 0, 0);
    
    TQVBoxLayout *titleLayout = new TQVBoxLayout(2);
    m_lblTitle = new TQLabel(i18n("<h2>System Security Audit</h2>"), m_headerWidget);
    titleLayout->addWidget(m_lblTitle);
    
    m_lblDesc = new TQLabel(i18n("Check your system security posture and get recommendations."), m_headerWidget);
    m_lblDesc->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    titleLayout->addWidget(m_lblDesc);
    
    headerLayout->addLayout(titleLayout, 1);
    
    m_btnRefresh = new TQPushButton(SmallIcon("view-refresh"), i18n("Refresh Audit"), m_headerWidget);
    m_btnRefresh->hide(); // Hidden initially
    connect(m_btnRefresh, SIGNAL(clicked()), this, SLOT(runAudit()));
    headerLayout->addWidget(m_btnRefresh);
    
    mainLayout->addWidget(m_headerWidget);
    
    TQFrame *line = new TQFrame(this);
    line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    mainLayout->addWidget(line);
    
    // Scroll view for content
    m_scrollView = new TQScrollView(this);
    m_scrollView->setResizePolicy(TQScrollView::AutoOneFit);
    m_scrollView->setFrameStyle(TQFrame::NoFrame);
    m_scrollView->viewport()->setBackgroundMode(PaletteBackground);
    
    m_scrollContent = new TQWidget(m_scrollView->viewport());
    m_scrollView->addChild(m_scrollContent);
    m_scrollLayout = new TQVBoxLayout(m_scrollContent, 10, 10);
    
    mainLayout->addWidget(m_scrollView);
    
    // Initial Run Button (Centered in scroll view initially)
    m_btnInitialRun = new TQPushButton(i18n("Run Security Audit"), m_scrollContent);
    m_btnInitialRun->setIconSet(TQIconSet(IconUtils::colorize(focus_png, focus_png_len, TQColor(50, 150, 250), 32, 32)));
    TQFont btnFont = m_btnInitialRun->font();
    btnFont.setBold(true);
    btnFont.setPointSize(btnFont.pointSize() + 2);
    m_btnInitialRun->setFont(btnFont);
    m_btnInitialRun->setFixedSize(250, 60);
    connect(m_btnInitialRun, SIGNAL(clicked()), this, SLOT(runAudit()));
    
    m_scrollLayout->addStretch(1);
    m_scrollLayout->addWidget(m_btnInitialRun, 0, TQt::AlignCenter);
    m_scrollLayout->addStretch(1);
    
    // Connect to settings manager for dark mode
    SettingsManager *settings = new SettingsManager(this);
    connect(settings, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()));
    
    // Check if we have a previous report
    AuditReport report = m_audit->loadAuditReport();
    if (report.sections.count() > 0) {
        buildReportUI(report);
    }
    
    applyTheme(settings->valueBool("dark_mode", false));
}

AuditView::~AuditView()
{
}

void AuditView::applyTheme(bool isDark)
{
    TQColor textColor = isDark ? TQColor(200, 200, 200) : TQColor(100, 100, 100);
    
    // Update frames (cards)
    TQColor frameBgColor = isDark ? TQColor(40, 40, 40) : TQColor(255, 255, 255);
    for (TQValueList<TQFrame*>::Iterator it = m_cardFrames.begin(); it != m_cardFrames.end(); ++it) {
        (*it)->setPaletteBackgroundColor(frameBgColor);
    }

    // Update subtitle labels
    for (TQValueList<TQLabel*>::Iterator it = m_subtitleLabels.begin(); it != m_subtitleLabels.end(); ++it) {
        (*it)->setPaletteForegroundColor(textColor);
        if ((*it)->parentWidget()) {
            (*it)->setPaletteBackgroundColor((*it)->parentWidget()->paletteBackgroundColor());
        }
    }
    
    // Update title labels
    TQColor titleColor = isDark ? TQColor(255, 255, 255) : TQColor(0, 0, 0);
    for (TQValueList<TQLabel*>::Iterator it = m_titleLabels.begin(); it != m_titleLabels.end(); ++it) {
        (*it)->setPaletteForegroundColor(titleColor);
        if ((*it)->parentWidget()) {
            (*it)->setPaletteBackgroundColor((*it)->parentWidget()->paletteBackgroundColor());
        }
    }
}

void AuditView::reloadIcons()
{
    if (m_btnRefresh) {
        m_btnRefresh->setIconSet(TQIconSet(SmallIcon("view-refresh")));
    }
    AuditReport report = m_audit->loadAuditReport();
    if (report.sections.count() > 0) {
        buildReportUI(report);
    }
}

void AuditView::onSettingsChanged()
{
    SettingsManager settings(this);
    applyTheme(settings.valueBool("dark_mode", false));
}

void AuditView::setScanRunningState(bool running)
{
    if (m_btnInitialRun) m_btnInitialRun->setEnabled(!running);
    if (m_btnRefresh) m_btnRefresh->setEnabled(!running);
}

void AuditView::clearUI()
{
    m_cardFrames.clear();
    m_subtitleLabels.clear();
    m_titleLabels.clear();
    
    // Clear scroll layout
    TQLayoutIterator it = m_scrollLayout->iterator();
    TQLayoutItem *item;
    while ((item = it.current()) != 0) {
        m_scrollLayout->removeItem(item);
        if (item->widget()) {
            if (item->widget() != m_btnInitialRun) {
                item->widget()->hide();
                item->widget()->deleteLater();
            } else {
                item->widget()->hide();
            }
        }
        delete item;
    }
}

void AuditView::runAudit()
{
    m_btnInitialRun->hide();
    m_btnRefresh->setEnabled(false);
    
    clearUI();
    
    TQLabel *lblChecking = new TQLabel(i18n("Running security audit, please wait..."), m_scrollContent);
    lblChecking->setAlignment(TQt::AlignCenter);
    m_scrollLayout->addWidget(lblChecking);
    m_scrollLayout->addStretch(1);
    lblChecking->show();
    
    kapp->processEvents();
    
    AuditReport report;
    report.timestamp = TQDateTime::currentDateTime().toTime_t();
    
    report.sections.append(m_audit->checkClamAVHealth());
    report.sections.append(m_audit->checkFirewall());
    report.sections.append(m_audit->checkMacFramework());
    report.sections.append(m_audit->checkAutoUpdates());
    report.sections.append(m_audit->checkIntrusionDetection());
    report.sections.append(m_audit->checkSSHHardening());
    report.sections.append(m_audit->checkHomePermissions());
    report.sections.append(m_audit->checkKernelHardening());
    report.sections.append(m_audit->checkPasswordPolicy());
    report.sections.append(m_audit->checkTDEHealth());
    
    m_audit->saveAuditReport(report);
    
    // Create detailed audit log entry
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->getLogManager()) {
        int overallStatus = 0; // 0 = Pass, 1 = Warning, 2 = Fail
        TQStringList attentionItems;
        int checkCount = 0;
        int warningCount = 0;
        int failCount = 0;
        
        for (TQValueList<AuditSectionResult>::ConstIterator sit = report.sections.begin(); sit != report.sections.end(); ++sit) {
            for (TQValueList<AuditCheckResult>::ConstIterator cit = (*sit).checks.begin(); cit != (*sit).checks.end(); ++cit) {
                checkCount++;
                if ((*cit).status == AuditFail) {
                    failCount++;
                    if (overallStatus < 2) overallStatus = 2;
                    
                    TQString item = TQString("- [%1] %2: %3\n  %4").arg((*sit).title).arg((*cit).name).arg(i18n("ACTION REQUIRED")).arg((*cit).detail);
                    if (!(*cit).recommendation.isEmpty()) {
                        item += "\n  Recommendation: " + (*cit).recommendation;
                    }
                    attentionItems.append(item);
                } else if ((*cit).status == AuditWarning) {
                    warningCount++;
                    if (overallStatus < 1) overallStatus = 1;
                    
                    TQString item = TQString("- [%1] %2: %3\n  %4").arg((*sit).title).arg((*cit).name).arg(i18n("WARNING")).arg((*cit).detail);
                    if (!(*cit).recommendation.isEmpty()) {
                        item += "\n  Recommendation: " + (*cit).recommendation;
                    }
                    attentionItems.append(item);
                }
            }
        }
        
        TQString logDetails;
        logDetails += "========================================\n";
        logDetails += "         SYSTEM SECURITY AUDIT          \n";
        logDetails += "========================================\n\n";
        logDetails += "Date: " + TQDateTime::currentDateTime().toString() + "\n";
        
        TQString statusText;
        if (overallStatus == 2) statusText = "ACTION REQUIRED / VULNERABLE";
        else if (overallStatus == 1) statusText = "WARNINGS FOUND";
        else statusText = "SECURE";
        
        logDetails += "Overall Status: " + statusText + "\n";
        logDetails += TQString("Checks performed: %1 (Warnings: %2, Failures: %3)\n\n").arg(checkCount).arg(warningCount).arg(failCount);
        
        logDetails += "----------------------------------------\n";
        if (attentionItems.isEmpty()) {
            logDetails += "All checked security controls are secure.\n";
            logDetails += "No points of attention found.\n";
        } else {
            logDetails += "POINTS OF ATTENTION:\n\n";
            for (TQStringList::ConstIterator it = attentionItems.begin(); it != attentionItems.end(); ++it) {
                logDetails += *it + "\n\n";
            }
        }
        logDetails += "----------------------------------------\n";
        
        mainWindow->getLogManager()->addLog(2, overallStatus, logDetails);
    }
    
    clearUI(); // Remove the "Running..." label before building the actual report
    buildReportUI(report);
    m_btnRefresh->setEnabled(true);
}

TQFrame* AuditView::createCardFrame(const TQString &title, const TQPixmap &iconPixmap, const TQString &category)
{
    TQFrame *card = new TQFrame(m_scrollContent);
    card->setFrameStyle(TQFrame::StyledPanel | TQFrame::Raised);
    card->setMargin(10);
    m_cardFrames.append(card);
    
    TQVBoxLayout *cardLayout = new TQVBoxLayout(card, 5, 5);
    
    TQHBoxLayout *headerLayout = new TQHBoxLayout(cardLayout, 5);
    TQLabel *lblIcon = new TQLabel(card);
    lblIcon->setPixmap(iconPixmap);
    headerLayout->addWidget(lblIcon);
    
    TQLabel *lblTitle = new TQLabel(title, card);
    TQFont f = lblTitle->font();
    f.setBold(true);
    lblTitle->setFont(f);
    m_titleLabels.append(lblTitle);
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch(1);
    
    return card;
}

void AuditView::addCheckRow(TQFrame *card, const AuditCheckResult &check)
{
    TQVBoxLayout *cardLayout = static_cast<TQVBoxLayout*>(card->layout());
    
    // Check row
    TQWidget *rowWidget = new TQWidget(card);
    TQHBoxLayout *rowLayout = new TQHBoxLayout(rowWidget, 0, 5);
    
    TQVBoxLayout *textLayout = new TQVBoxLayout(rowLayout, 0, 0);
    rowLayout->setStretchFactor(textLayout, 1);
    TQLabel *lblName = new TQLabel(check.name, rowWidget);
    m_titleLabels.append(lblName);
    textLayout->addWidget(lblName);
    
    if (!check.detail.isEmpty()) {
        TQLabel *lblDetail = new TQLabel(check.detail, rowWidget);
        m_subtitleLabels.append(lblDetail);
        textLayout->addWidget(lblDetail);
    }
    
    if (!check.launch_command.isEmpty()) {
        TQPushButton *btnLaunch = new TQPushButton(check.launch_label, rowWidget);
        btnLaunch->setName(check.launch_command.latin1()); // Use name to pass command
        connect(btnLaunch, SIGNAL(clicked()), this, SLOT(copyToClipboard())); // Reuse or create launch slot
        rowLayout->addWidget(btnLaunch);
    }
    
    // Status Icon
    TQLabel *lblStatus = new TQLabel(rowWidget);
    if (check.status == AuditPass) {
        lblStatus->setPixmap(IconUtils::colorize(check_png, check_png_len, TQt::green, 16, 16));
    } else if (check.status == AuditWarning) {
        lblStatus->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQColor(255, 165, 0), 16, 16));
    } else if (check.status == AuditFail) {
        lblStatus->setPixmap(IconUtils::load(secu_fail_png, secu_fail_png_len, 16, 16));
    } else {
        lblStatus->setPixmap(IconUtils::colorize(warning_png, warning_png_len, TQt::gray, 16, 16));
    }
    rowLayout->addWidget(lblStatus);
    
    cardLayout->addWidget(rowWidget);
    
    // Recommendation
    if (!check.recommendation.isEmpty() && check.status != AuditPass) {
        TQHBoxLayout *recLayout = new TQHBoxLayout(cardLayout, 5);
        
        TQLabel *lblTipIcon = new TQLabel(card);
        lblTipIcon->setPixmap(IconUtils::load(tip_png, tip_png_len, 16, 16));
        recLayout->addWidget(lblTipIcon, 0, TQt::AlignTop);
        
        TQLabel *lblRec = new TQLabel(check.recommendation, card);
        lblRec->setAlignment(TQt::WordBreak | TQt::AlignLeft);
        m_subtitleLabels.append(lblRec);
        recLayout->addWidget(lblRec, 1);
    }
    
    // Install command
    if (!check.install_command.isEmpty()) {
        TQHBoxLayout *cmdLayout = new TQHBoxLayout(cardLayout, 5);
        TQLabel *lblCmd = new TQLabel(check.install_command, card);
        lblCmd->setAlignment(TQt::WordBreak | TQt::AlignLeft);
        lblCmd->setFrameStyle(TQFrame::Panel | TQFrame::Sunken);
        lblCmd->setMargin(5);
        TQFont mono = lblCmd->font();
        mono.setFamily("monospace");
        lblCmd->setFont(mono);
        
        TQPalette pal = lblCmd->palette();
        if (IconUtils::isDarkMode()) {
            pal.setColor(TQColorGroup::Background, TQColor(20, 20, 20));
            pal.setColor(TQColorGroup::Foreground, TQColor(255, 255, 255));
            pal.setColor(TQColorGroup::Text, TQColor(255, 255, 255));
        } else {
            pal.setColor(TQColorGroup::Background, TQColor(245, 245, 245));
            pal.setColor(TQColorGroup::Foreground, TQColor(0, 0, 0));
            pal.setColor(TQColorGroup::Text, TQColor(0, 0, 0));
        }
        lblCmd->setPalette(pal);
        lblCmd->setBackgroundMode(TQt::PaletteBackground);
        
        cmdLayout->addWidget(lblCmd, 1);
        
        TQPushButton *btnCopy = new TQPushButton(SmallIcon("edit-copy"), "", card);
        btnCopy->setName(check.install_command.latin1());
        connect(btnCopy, SIGNAL(clicked()), this, SLOT(copyToClipboard()));
        cmdLayout->addWidget(btnCopy);
    }
}

void AuditView::buildReportUI(const AuditReport &report)
{
    clearUI();
    m_btnInitialRun->hide();
    m_btnRefresh->show();
    
    // Summary Banner
    int failCount = 0;
    int warnCount = 0;
    int totalChecks = 0;
    for (TQValueList<AuditSectionResult>::ConstIterator it = report.sections.begin(); it != report.sections.end(); ++it) {
        for (TQValueList<AuditCheckResult>::ConstIterator cit = (*it).checks.begin(); cit != (*it).checks.end(); ++cit) {
            totalChecks++;
            if ((*cit).status == AuditFail) failCount++;
            else if ((*cit).status == AuditWarning) warnCount++;
        }
    }
    
    TQString summaryText;
    if (failCount > 0 && warnCount > 0) {
        summaryText = i18n("%1 security issues, %2 warnings (%3 checks)").arg(failCount).arg(warnCount).arg(totalChecks);
    } else if (failCount > 0) {
        summaryText = i18n("%1 security issues need attention (%2 checks)").arg(failCount).arg(totalChecks);
    } else if (warnCount > 0) {
        summaryText = i18n("%1 checks need review (%2 checks)").arg(warnCount).arg(totalChecks);
    } else {
        summaryText = i18n("Audit complete - System is secure (%1 checks)").arg(totalChecks);
    }
    
    TQLabel *lblSummary = new TQLabel(summaryText, m_scrollContent);
    TQFont sumF = lblSummary->font();
    sumF.setBold(true);
    lblSummary->setFont(sumF);
    m_scrollLayout->addWidget(lblSummary);
    lblSummary->show();
    
    TQDateTime dt;
    dt.setTime_t(report.timestamp);
    TQLabel *lblTime = new TQLabel(i18n("Last audit: %1").arg(dt.toString(TQt::LocalDate)), m_scrollContent);
    m_scrollLayout->addWidget(lblTime);
    lblTime->show();
    
    // Build sections
    for (TQValueList<AuditSectionResult>::ConstIterator it = report.sections.begin(); it != report.sections.end(); ++it) {
        const AuditSectionResult &section = *it;
        AuditStatus st = section.overallStatus();
        
        TQPixmap sectionIcon;
        if (section.category == "intrusion_detection") {
            sectionIcon = IconUtils::load(intrusion_png, intrusion_png_len, 24, 24);
        } else if (section.category == "deep_scans") {
            sectionIcon = IconUtils::load(deep_scan_png, deep_scan_png_len, 24, 24);
        } else {
            if (st == AuditPass) sectionIcon = IconUtils::load(secu_ok_png, secu_ok_png_len, 24, 24);
            else if (st == AuditWarning) sectionIcon = IconUtils::colorize(warning_png, warning_png_len, TQColor(255, 165, 0), 24, 24);
            else sectionIcon = IconUtils::load(secu_fail_png, secu_fail_png_len, 24, 24);
        }
        
        TQFrame *card = createCardFrame(section.title, sectionIcon, section.category);
        for (TQValueList<AuditCheckResult>::ConstIterator cit = section.checks.begin(); cit != section.checks.end(); ++cit) {
            addCheckRow(card, *cit);
        }
        m_scrollLayout->addWidget(card);
        card->show();
    }
    
    // Add Deep Scans Section
    TQFrame *deepCard = createCardFrame(i18n("Deep Scans"), IconUtils::load(deep_scan_png, deep_scan_png_len, 24, 24), "deep_scans");
    
    TQLabel *lblDeepDesc = new TQLabel(i18n("Require administrator privileges and may take several minutes"), deepCard);
    m_subtitleLabels.append(lblDeepDesc);
    static_cast<TQVBoxLayout*>(deepCard->layout())->addWidget(lblDeepDesc);
    
    // Lynis row
    bool lynisOk = m_audit->isBinaryAvailable("lynis");
    TQHBoxLayout *lynisLayout = new TQHBoxLayout(static_cast<TQVBoxLayout*>(deepCard->layout()), 5);
    TQVBoxLayout *lynisTextLayout = new TQVBoxLayout(lynisLayout, 0);
    TQLabel *lblLynis = new TQLabel(i18n("Lynis Security Audit"), deepCard);
    m_titleLabels.append(lblLynis);
    lynisTextLayout->addWidget(lblLynis);
    if (!lynisOk) {
        TQLabel *lblLynisHint = new TQLabel(i18n("Not installed — sudo apt install lynis"), deepCard);
        m_subtitleLabels.append(lblLynisHint);
        lynisTextLayout->addWidget(lblLynisHint);
    }
    lynisLayout->addStretch(1);
    TQPushButton *btnLynis = new TQPushButton(i18n("Run Lynis"), deepCard);
    btnLynis->setEnabled(lynisOk);
    connect(btnLynis, SIGNAL(clicked()), this, SLOT(runLynis()));
    lynisLayout->addWidget(btnLynis);
    
    // chkrootkit row
    bool rkOk = m_audit->isBinaryAvailable("chkrootkit");
    TQHBoxLayout *rkLayout = new TQHBoxLayout(static_cast<TQVBoxLayout*>(deepCard->layout()), 5);
    TQVBoxLayout *rkTextLayout = new TQVBoxLayout(rkLayout, 0);
    TQLabel *lblRk = new TQLabel(i18n("Rootkit Detection"), deepCard);
    m_titleLabels.append(lblRk);
    rkTextLayout->addWidget(lblRk);
    TQLabel *lblRkDesc = new TQLabel(rkOk ? i18n("Scan for hidden rootkits, backdoors, and local exploits")
                                         : i18n("Not installed — sudo apt install chkrootkit"), deepCard);
    m_subtitleLabels.append(lblRkDesc);
    rkTextLayout->addWidget(lblRkDesc);
    rkLayout->addStretch(1);
    
    TQPushButton *btnRk = new TQPushButton(i18n("Run chkrootkit"), deepCard);
    btnRk->setEnabled(rkOk);
    connect(btnRk, SIGNAL(clicked()), this, SLOT(runRootkit()));
    rkLayout->addWidget(btnRk);
    
    m_scrollLayout->addWidget(deepCard);
    deepCard->show();
    
    m_scrollLayout->addStretch(1);
    
    SettingsManager settings(this);
    applyTheme(settings.valueBool("dark_mode", false));
}

void AuditView::runLynis()
{
    // Disable button briefly to show feedback
    TQPushButton *btn = (TQPushButton*)sender();
    btn->setEnabled(false);
    btn->setText(i18n("Launching..."));
    kapp->processEvents();
    
    m_audit->runLynisAudit();
    
    // Restore button immediately as it runs detached
    btn->setText(i18n("Run Lynis"));
    btn->setEnabled(true);
}

void AuditView::runRootkit()
{
    TQPushButton *btn = (TQPushButton*)sender();
    btn->setEnabled(false);
    btn->setText(i18n("Launching..."));
    kapp->processEvents();
    
    m_audit->runRootkitCheck();
    
    btn->setText(i18n("Run chkrootkit"));
    btn->setEnabled(true);
}

void AuditView::copyToClipboard()
{
    TQPushButton *btn = (TQPushButton*)sender();
    if (btn) {
        TQString cmd = TQString(btn->name());
        if (cmd.startsWith("gufw")) {
            // It's a launch command, not a copy
            system((cmd + " &").latin1());
            return;
        }
        TQApplication::clipboard()->setText(cmd);
        KMessageBox::information(this, i18n("Command copied to clipboard."), i18n("Copied"));
    }
}

#include "audit_view.moc"
