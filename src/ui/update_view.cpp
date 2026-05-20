/*
 * update_view.cpp - Virus Database Update View
 */

#include "update_view.h"
#include "../core/updater.h"
#include "../core/clamav_detection.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqprogressbar.h>
#include <tqtextedit.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kiconloader.h>
#include <ntqwidgetstack.h>
#include <stdlib.h>
#include <krun.h>
#include "../core/log_manager.h"
#include "../core/settings_manager.h"
#include "mainwindow.h"
#include "icon_utils.h"
#include <tqthread.h>
#include <tqtimer.h>

static TQString formatWithSpaces(int number) {
    TQString str = TQString::number(number);
    TQString res;
    int len = str.length();
    for (int i = 0; i < len; ++i) {
        if (i > 0 && (len - i) % 3 == 0) {
            res += " ";
        }
        res += str[i];
    }
    return res;
}

/* ---- Background thread for collecting DB info ---- */
void DbInfoThread::run()
{
    dbDate = ClamAVDetection::getDatabaseDate();
    m_ageDays = ClamAVDetection::getDatabaseAgeDays();
    dbInfos = ClamAVDetection::getDatabaseDetails();
    
    freshclamVersion = ClamAVDetection::getFreshclamVersion();
    serviceStatus = ClamAVDetection::getFreshclamServiceStatus();
    hasUnofficial = ClamAVDetection::getUnofficialDetails(ssFileCount, ssTotalSigs, ssTotalSizeKB, ssLastModified,
                                                          urlhausFileCount, urlhausTotalSigs, urlhausTotalSizeKB, urlhausLastModified,
                                                          otherFileCount, otherTotalSigs, otherTotalSizeKB, otherLastModified);
}

/* ---- UpdateView ---- */

UpdateView::UpdateView(TQWidget *parent, TQWidgetStack *stack)
    : TQWidget(parent), m_dbInfoThread(0), m_cachedAgeDays(-1), m_cacheValid(false)
{
    m_updater = new Updater(this);
    
    connect(m_updater, TQT_SIGNAL(updateProgress(const TQString &)), 
            this, TQT_SLOT(onUpdateProgress(const TQString &)));
    connect(m_updater, TQT_SIGNAL(updateFinished(int)), 
            this, TQT_SLOT(onUpdateFinished(int)));
    
    m_dbInfoTimer = new TQTimer(this);
    connect(m_dbInfoTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(onDbInfoReady()));
            
    setupUI();
    
    m_statusLabel->setText("");
    m_statusLabel->hide();
    m_lblDbDetails->setText(i18n("Retrieving database information..."));
    m_lblFreshclamStatus->setText(i18n("freshclam: checking..."));
    m_lblServiceStatus->setText(i18n("Service: checking..."));
    
    fetchDbInfoAsync();
}

UpdateView::~UpdateView()
{
    if (m_dbInfoThread) {
        m_dbInfoThread->wait();
        delete m_dbInfoThread;
    }
}

void UpdateView::showEvent(TQShowEvent *event)
{
    TQWidget::showEvent(event);
    // Just display cached data (instant) — no subprocess calls
    if (m_cacheValid) {
        displayCachedStatus();
    }
}

void UpdateView::setupUI()
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    // Header
    TQLabel *titleLabel = new TQLabel(i18n("<h2>Virus Database Update</h2>"), this);
    mainLayout->addWidget(titleLabel);
    
    TQLabel *descLabel = new TQLabel(i18n("Keep your system protected by ensuring the ClamAV virus signatures are up to date."), this);
    descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(descLabel);
    
    TQFrame *line = new TQFrame(this);
    line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    mainLayout->addWidget(line);
    
    mainLayout->addSpacing(10);
    
    // Status box
    TQHBoxLayout *statusLayout = new TQHBoxLayout(mainLayout, 6);
    
    m_dbWarningIcon = new TQLabel(this);
    m_dbWarningIcon->hide();
    statusLayout->addWidget(m_dbWarningIcon);
    
    m_statusLabel = new TQLabel(this);
    m_statusLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch(1);
    
    m_lblDbDetails = new TQLabel(this);
    m_lblDbDetails->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    m_lblDbDetails->setPaletteForegroundColor(TQt::darkGray);
    mainLayout->addWidget(m_lblDbDetails);
    
    mainLayout->addSpacing(10);
    
    // Progress
    m_progressBar = new TQProgressBar(this);
    m_progressBar->setTotalSteps(0); // Indeterminate by default
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);
    
    // Log output
    m_logOutput = new TQTextEdit(this);
    m_logOutput->setReadOnly(true);
    m_logOutput->setTextFormat(TQt::PlainText);
    mainLayout->addWidget(m_logOutput);
    
    // Buttons
    TQHBoxLayout *btnLayout = new TQHBoxLayout(mainLayout, 6);
    btnLayout->addStretch(1);
    
    m_btnCancel = new TQPushButton(IconUtils::load(stop_png, stop_png_len), i18n("&Cancel"), this);
    m_btnCancel->setEnabled(false);
    connect(m_btnCancel, TQT_SIGNAL(clicked()), this, TQT_SLOT(cancelUpdate()));
    btnLayout->addWidget(m_btnCancel);
    
    m_btnForceUpdate = new TQPushButton(IconUtils::load(refresh_png, refresh_png_len), i18n("&Force Update"), this);
    connect(m_btnForceUpdate, TQT_SIGNAL(clicked()), this, TQT_SLOT(forceUpdate()));
    btnLayout->addWidget(m_btnForceUpdate);
    
    m_btnUpdate = new TQPushButton(IconUtils::load(refresh_png, refresh_png_len), i18n("&Update Database"), this);
    connect(m_btnUpdate, TQT_SIGNAL(clicked()), this, TQT_SLOT(startUpdate()));
    btnLayout->addWidget(m_btnUpdate);
    
    mainLayout->addSpacing(10);
    
    // Bottom info (Freshclam and Service status)
    TQHBoxLayout *centerBox = new TQHBoxLayout(mainLayout, 0);
    centerBox->addStretch(1);
    
    TQVBoxLayout *statusBox = new TQVBoxLayout(centerBox, 6);
    
    TQHBoxLayout *freshclamBox = new TQHBoxLayout(statusBox, 6);
    TQLabel *icnFreshclam = new TQLabel(this);
    icnFreshclam->setPixmap(SmallIcon("button_ok")); // Green check
    freshclamBox->addWidget(icnFreshclam);
    m_lblFreshclamStatus = new TQLabel(this);
    m_lblFreshclamStatus->setPaletteForegroundColor(TQt::darkGray);
    freshclamBox->addWidget(m_lblFreshclamStatus);
    freshclamBox->addStretch(1);
    
    TQHBoxLayout *serviceBox = new TQHBoxLayout(statusBox, 6);
    TQLabel *icnService = new TQLabel(this);
    icnService->setPixmap(SmallIcon("player_play")); // Green play icon
    serviceBox->addWidget(icnService);
    m_lblServiceStatus = new TQLabel(this);
    m_lblServiceStatus->setPaletteForegroundColor(TQt::darkGray);
    serviceBox->addWidget(m_lblServiceStatus);
    serviceBox->addStretch(1);
    
    centerBox->addStretch(1);
}

void UpdateView::reloadIcons()
{
    if (m_btnCancel) m_btnCancel->setIconSet(TQIconSet(IconUtils::load(stop_png, stop_png_len)));
    if (m_btnForceUpdate) m_btnForceUpdate->setIconSet(TQIconSet(IconUtils::load(refresh_png, refresh_png_len)));
    if (m_btnUpdate) m_btnUpdate->setIconSet(TQIconSet(IconUtils::load(refresh_png, refresh_png_len)));
}

void UpdateView::fetchDbInfoAsync()
{
    if (m_dbInfoThread) {
        m_dbInfoThread->wait();
        delete m_dbInfoThread;
    }
    m_dbInfoThread = new DbInfoThread();
    m_dbInfoThread->start();
    m_dbInfoTimer->start(200); // Poll for completion
}

void UpdateView::onDbInfoReady()
{
    if (!m_dbInfoThread || !m_dbInfoThread->finished()) return;
    
    m_dbInfoTimer->stop();
    
    // Store results in cache
    m_cachedDbDate = m_dbInfoThread->dbDate;
    m_cachedAgeDays = m_dbInfoThread->m_ageDays;
    m_cachedDbInfos = m_dbInfoThread->dbInfos;
    m_cachedFreshclamVer = m_dbInfoThread->freshclamVersion;
    m_cachedServiceStatus = m_dbInfoThread->serviceStatus;
    m_cachedHasUnofficial = m_dbInfoThread->hasUnofficial;
    m_cachedSsFileCount = m_dbInfoThread->ssFileCount;
    m_cachedSsTotalSigs = m_dbInfoThread->ssTotalSigs;
    m_cachedSsTotalSizeKB = m_dbInfoThread->ssTotalSizeKB;
    m_cachedSsLastModified = m_dbInfoThread->ssLastModified;
    m_cachedUrlhausFileCount = m_dbInfoThread->urlhausFileCount;
    m_cachedUrlhausTotalSigs = m_dbInfoThread->urlhausTotalSigs;
    m_cachedUrlhausTotalSizeKB = m_dbInfoThread->urlhausTotalSizeKB;
    m_cachedUrlhausLastModified = m_dbInfoThread->urlhausLastModified;
    m_cachedOtherFileCount = m_dbInfoThread->otherFileCount;
    m_cachedOtherTotalSigs = m_dbInfoThread->otherTotalSigs;
    m_cachedOtherTotalSizeKB = m_dbInfoThread->otherTotalSizeKB;
    m_cachedOtherLastModified = m_dbInfoThread->otherLastModified;
    m_cacheValid = true;
    
    delete m_dbInfoThread;
    m_dbInfoThread = 0;
    
    displayCachedStatus();
}

void UpdateView::onSettingsChanged()
{
    m_cacheValid = false;
    fetchDbInfoAsync();
}

void UpdateView::displayCachedStatus()
{
    if (m_cachedAgeDays > 2) {
        TQString statusText = i18n("<font color='red'><b>Database is %1 days old \u2014 update recommended!</b></font>")
                         .arg(m_cachedAgeDays);
        m_statusLabel->setText(statusText);
        m_statusLabel->show();
        m_dbWarningIcon->setPixmap(IconUtils::load(secu_fail_png, secu_fail_png_len, 32, 32));
        m_dbWarningIcon->show();
    } else {
        m_statusLabel->hide();
        m_dbWarningIcon->hide();
    }
    
    // Detailed database info
    TQString filesLine;
    int totalSignatures = 0;
    
    for (TQValueList<ClamAVDetection::DbFileInfo>::ConstIterator it = m_cachedDbInfos.begin(); it != m_cachedDbInfos.end(); ++it) {
        totalSignatures += (*it).signatures;
        
        TQString sizeStr;
        if ((*it).sizeMB >= 1024) {
            sizeStr = TQString("%1 MB").arg((*it).sizeMB / 1024);
        } else {
            sizeStr = TQString("%1 KB").arg((*it).sizeMB);
        }
        
        TQString dateStr;
        if (!(*it).buildDate.isEmpty()) {
            dateStr = TQString(" (%1)").arg((*it).buildDate);
        }
        
        if (!filesLine.isEmpty()) filesLine += "&nbsp;&nbsp;|&nbsp;&nbsp;";
        filesLine += TQString("<b>%1</b> v%2: %3 sigs, %4%5")
            .arg((*it).name)
            .arg((*it).version)
            .arg(formatWithSpaces((*it).signatures))
            .arg(sizeStr)
            .arg(dateStr);
    }
    
    bool enableUnofficial = false;
    bool enableSanesecurity = false;
    bool enableUrlhaus = false;
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        SettingsManager *s = mainWindow->settingsManager();
        enableUnofficial = s->valueBool("enable_unofficial_sigs", false);
        enableSanesecurity = s->valueBool("enable_sanesecurity_sigs", false);
        enableUrlhaus = s->valueBool("enable_urlhaus_sigs", false);
    }

    int totalSigsCombined = totalSignatures;
    if (enableUnofficial) {
        totalSigsCombined += m_cachedOtherTotalSigs;
        if (enableSanesecurity) {
            totalSigsCombined += m_cachedSsTotalSigs;
        }
        if (enableUrlhaus) {
            totalSigsCombined += m_cachedUrlhausTotalSigs;
        }
    }

    TQString detailText;
    detailText = TQString("<font color=\"%1\" size=\"+1\">%2: <b><font color=\"%3\">%4</font></b></font><br>")
        .arg(colorGroup().text().name())
        .arg(i18n("Total current signatures"))
        .arg(colorGroup().link().name())
        .arg(formatWithSpaces(totalSigsCombined));

    detailText += TQString("<b>%1</b> : %2")
        .arg(i18n("clamAV base"))
        .arg(filesLine);

    if (enableUnofficial && (enableSanesecurity || enableUrlhaus)) {
        detailText += TQString("<br><i>%1:</i>")
            .arg(i18n("Additionnal signatures"));

        if (enableSanesecurity) {
            TQString ssSizeStr;
            if (m_cachedSsTotalSizeKB >= 1024) {
                ssSizeStr = TQString("%1 MB").arg(m_cachedSsTotalSizeKB / 1024);
            } else {
                ssSizeStr = TQString("%1 KB").arg(m_cachedSsTotalSizeKB);
            }
            
            TQString ssDateStr;
            if (!m_cachedSsLastModified.isEmpty()) {
                ssDateStr = TQString(" (%1)").arg(m_cachedSsLastModified);
            }

            detailText += TQString("<br>&nbsp;&nbsp;<b>%1</b> : %2 files, %3 sigs, %4%5")
                .arg(i18n("Sanesecurity"))
                .arg(m_cachedSsFileCount)
                .arg(formatWithSpaces(m_cachedSsTotalSigs))
                .arg(ssSizeStr)
                .arg(ssDateStr);
        }

        if (enableUrlhaus) {
            TQString uhSizeStr;
            if (m_cachedUrlhausTotalSizeKB >= 1024) {
                uhSizeStr = TQString("%1 MB").arg(m_cachedUrlhausTotalSizeKB / 1024);
            } else {
                uhSizeStr = TQString("%1 KB").arg(m_cachedUrlhausTotalSizeKB);
            }
            
            TQString uhDateStr;
            if (!m_cachedUrlhausLastModified.isEmpty()) {
                uhDateStr = TQString(" (%1)").arg(m_cachedUrlhausLastModified);
            }

            detailText += TQString("<br>&nbsp;&nbsp;<b>%1</b> : %2 files, %3 sigs, %4%5")
                .arg(i18n("URLhaus"))
                .arg(m_cachedUrlhausFileCount)
                .arg(formatWithSpaces(m_cachedUrlhausTotalSigs))
                .arg(uhSizeStr)
                .arg(uhDateStr);
        }
    }
    
    m_lblDbDetails->setText(detailText);
    
    m_lblFreshclamStatus->setText(i18n("freshclam: %1").arg(m_cachedFreshclamVer));
    m_lblServiceStatus->setText(i18n("Service: %1").arg(m_cachedServiceStatus));
}

void UpdateView::startUpdate()
{
    m_btnUpdate->setEnabled(false);
    m_btnForceUpdate->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_progressBar->show();
    m_progressBar->setTotalSteps(0); // Indeterminate
    
    m_logOutput->clear();
    m_logOutput->append(i18n("Starting update..."));
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        SettingsManager *s = mainWindow->settingsManager();
        m_updater->setDatabaseMirror(s->value("db_mirror", "database.clamav.net"));
        m_updater->setChecksPerDay(s->value("db_checks", "12").toInt());
    }
    
    /* Check if service is running BEFORE starting - if so, the response will
     * be synchronous (SERVICE_RUNNING) and we must NOT emit updateStateChanged(true)
     * because onUpdateFinished will never be called after this returns.
     * Only emit if a real async process is about to start. */
    bool serviceAlreadyRunning = m_updater->isServiceRunning();
    m_updater->startUpdate(false);
    if (!serviceAlreadyRunning) {
        emit updateStateChanged(true);
    } else {
        /* Restore buttons immediately - no process running */
        m_btnUpdate->setEnabled(true);
        m_btnForceUpdate->setEnabled(true);
        m_btnCancel->setEnabled(false);
        m_progressBar->hide();
    }
}

void UpdateView::forceUpdate()
{
    m_btnUpdate->setEnabled(false);
    m_btnForceUpdate->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_progressBar->show();
    m_progressBar->setTotalSteps(0); // Indeterminate
    
    m_logOutput->clear();
    m_logOutput->append(i18n("Starting forced update..."));
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        SettingsManager *s = mainWindow->settingsManager();
        m_updater->setDatabaseMirror(s->value("db_mirror", "database.clamav.net"));
        m_updater->setChecksPerDay(s->value("db_checks", "12").toInt());
    }
    
    m_updater->startUpdate(true);
    emit updateStateChanged(true);
}

void UpdateView::cancelUpdate()
{
    m_btnCancel->setEnabled(false);
    m_logOutput->append(i18n("Cancelling update..."));
    m_updater->cancelUpdate();
}

void UpdateView::onUpdateProgress(const TQString &message)
{
    m_logOutput->append(message);
    
    // Attempt to parse download percentage for the progress bar
    // freshclam output e.g. "Downloading daily.cvd [100%]"
    int percentIdx = message.findRev('%');
    if (percentIdx != -1) {
        int bracketIdx = message.findRev('[', percentIdx);
        if (bracketIdx != -1) {
            bool ok;
            int pct = message.mid(bracketIdx + 1, percentIdx - bracketIdx - 1).toInt(&ok);
            if (ok) {
                m_progressBar->setTotalSteps(100);
                m_progressBar->setProgress(pct);
            }
        }
    }
}

void UpdateView::setScanRunningState(bool running)
{
    if (!m_updater->isRunning()) {
        m_btnUpdate->setEnabled(!running);
        m_btnForceUpdate->setEnabled(!running);
    }
}

void UpdateView::onUpdateFinished(int status)
{
    emit updateStateChanged(false);
    m_btnUpdate->setEnabled(true);
    m_btnForceUpdate->setEnabled(true);
    m_btnCancel->setEnabled(false);
    m_progressBar->hide();
    
    // Re-fetch database info in background (database may have changed)
    fetchDbInfoAsync();
    
    int logStatus = 0;
    TQString logDetails;
    
    switch (status) {
        case Updater::SUCCESS:
        case Updater::ERROR:
            if (m_logOutput->text().contains("cool-down") || m_logOutput->text().contains("rate limit") || m_logOutput->text().contains("error code 429")) {
                m_logOutput->append(i18n("\nUpdate blocked by rate limit."));
                
                logStatus = 2; // Error
                if (isVisible()) {
                    int res = KMessageBox::questionYesNo(this, 
                        i18n("You are on cool-down from the ClamAV servers because of too many update attempts.\n\n"
                             "Would you like to manually download the database file (daily.cvd) via your web browser?\n"
                             "Once downloaded, you will need to open a root file manager (or terminal) and copy it to /var/lib/clamav/."), 
                        i18n("Rate Limited"),
                        KGuiItem(i18n("Download Manually"), "network-server"),
                        KStdGuiItem::cancel());
                        
                    if (res == KMessageBox::Yes) {
                        KRun::runURL(KURL("https://database.clamav.net/daily.cvd"), "text/html");
                    }
                }
            } else if (status == Updater::ERROR || m_logOutput->text().contains("ERROR:")) {
                m_logOutput->append(i18n("\nUpdate failed."));
                if (isVisible()) KMessageBox::error(this, i18n("An error occurred during the update process. Please check the log for details."), i18n("Update Failed"));
                logStatus = 2;
            } else {
                m_logOutput->append(i18n("\nUpdate completed successfully."));
                if (isVisible()) KMessageBox::information(this, i18n("The virus database has been updated successfully."), i18n("Update Complete"));
                logStatus = 0;
            }
            break;
        case Updater::CANCELLED:
            m_logOutput->append(i18n("\nUpdate cancelled by user."));
            logStatus = 2;
            break;
        case Updater::SERVICE_RUNNING:
            m_logOutput->append(i18n("\nThe ClamAV update service is already running in the background."));
            if (isVisible()) KMessageBox::information(this, i18n("The system's ClamAV daemon is currently managing updates in the background. Your database is being kept up to date automatically."), i18n("Update Managed"));
            logStatus = 0;
            break;
    }
    
    logDetails = m_logOutput->text();
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->getLogManager()) {
        mainWindow->getLogManager()->addLog(1, logStatus, logDetails); // 1 = Update
    }
    
    emit updateFinishedEvent(logStatus);
}

#include "update_view.moc"
