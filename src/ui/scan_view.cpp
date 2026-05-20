/*
 * scan_view.cpp - Virus Scan View
 */

#include "scan_view.h"
#include "../core/scanner.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqprogressbar.h>
#include <tqlistview.h>
#include <tqlistbox.h>
#include <tqcombobox.h>
#include <tqgroupbox.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdefiledialog.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <tqdir.h>
#include <tqdragobject.h>
#include <tqvgroupbox.h>
#include <tqhbox.h>
#include <kinputdialog.h>
#include <tqdatetime.h>
#include "../core/clamav_detection.h"
#include "../core/log_manager.h"
#include "../core/quarantine_manager.h"
#include "../core/profiles.h"
#include "../core/settings_manager.h"
#include "mainwindow.h"
#include "scan_results_dialog.h"
#include "icon_utils.h"
#include <tqpopupmenu.h>
#include <tqthread.h>
#include <tqtimer.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

class FileCounterThread : public TQThread {
public:
    FileCounterThread(const TQStringList &paths, const TQStringList &exclusions)
        : m_paths(paths), m_exclusions(exclusions), m_count(0), m_cancel(false) {}

    int count() const { return m_count; }
    void cancel() { m_cancel = true; }

protected:
    virtual void run() {
        for (int i = 0; i < (int)m_paths.count(); ++i) {
            if (m_cancel) return;
            TQString p = m_paths[i];
            struct stat st;
            // Use stat() (not lstat) so symlinks like /bin -> usr/bin are followed,
            // matching ClamAV's default --follow-dir-symlinks=1 behavior
            if (stat(p.local8Bit(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    m_count += countFilesInDir(p, m_exclusions);
                } else if (S_ISREG(st.st_mode)) {
                    m_count++;
                }
            }
        }
    }

private:
    TQStringList m_paths;
    TQStringList m_exclusions;
    int m_count;
    bool m_cancel;

    int countFilesInDir(const TQString &path, const TQStringList &exclusions) {
        if (m_cancel) return 0;
        int count = 0;
        DIR *dir = opendir(path.local8Bit());
        if (!dir) return 0;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (m_cancel) break;
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
                continue;
                
            TQString fullPath = path + "/" + entry->d_name;
            
            bool excluded = false;
            for (TQStringList::ConstIterator it = exclusions.begin(); it != exclusions.end(); ++it) {
                if (!(*it).isEmpty() && fullPath.startsWith(*it)) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) continue;
            
            if (entry->d_type == DT_DIR) {
                count += countFilesInDir(fullPath, exclusions);
            } else if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                count++;
            } else if (entry->d_type == DT_UNKNOWN) {
                struct stat st;
                if (lstat(fullPath.local8Bit(), &st) == 0) {
                    if (S_ISDIR(st.st_mode)) count += countFilesInDir(fullPath, exclusions);
                    else if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) count++;
                }
            }
        }
        closedir(dir);
        return count;
    }
};

ScanView::ScanView(TQWidget *parent, TQWidgetStack *stack)
    : TQWidget(parent), m_isRunning(false), m_totalFiles(-1), m_pulseStep(0), m_counterThread(0)
{
    m_scanner = new Scanner(this);
    m_counterTimer = new TQTimer(this);
    connect(m_counterTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(onCounterTimeout()));
    
    connect(m_scanner, TQT_SIGNAL(scanProgress(int, const TQString &)), 
            this, TQT_SLOT(onScanProgress(int, const TQString &)));
    connect(m_scanner, TQT_SIGNAL(scanFinished(int, int)), 
            this, TQT_SLOT(onScanFinished(int, int)));
    connect(m_scanner, TQT_SIGNAL(threatFound(const TQString &, const TQString &)), 
            this, TQT_SLOT(onThreatFound(const TQString &, const TQString &)));
    connect(m_scanner, TQT_SIGNAL(scanError(const TQString &)), 
            this, TQT_SLOT(onScanError(const TQString &)));
    connect(m_scanner, TQT_SIGNAL(scanLog(const TQString &)), 
            this, TQT_SLOT(onScanLog(const TQString &)));
            
    setupUI();
    setUIState(false);
}

ScanView::~ScanView()
{
    if (m_counterThread) {
        m_counterThread->cancel();
        m_counterThread->wait();
        delete m_counterThread;
    }
}

void ScanView::setupUI()
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    // Header
    TQHBoxLayout *headerLayout = new TQHBoxLayout(mainLayout, 6);
    TQLabel *titleLabel = new TQLabel(i18n("<h2>Virus Scanner</h2>"), this);
    headerLayout->addWidget(titleLabel, 1);
    
    // Profile Selector
    TQHBoxLayout *profileLayout = new TQHBoxLayout(headerLayout, 6);
    profileLayout->addWidget(new TQLabel(i18n("Scan Profile:"), this));
    m_profileCombo = new TQComboBox(this);
    m_profileCombo->insertItem(i18n("No Profile (Manual)"));
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->profileManager()) {
        TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
        for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
            m_profileCombo->insertItem((*it).name);
        }
        connect(mainWindow->profileManager(), TQT_SIGNAL(profilesUpdated()), this, TQT_SLOT(reloadProfiles()));
    }
    
    connect(m_profileCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onProfileSelected(int)));
    profileLayout->addWidget(m_profileCombo);
    
    TQLabel *descLabel = new TQLabel(i18n("Select files or directories to scan for viruses and malware."), this);
    descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(descLabel);
    
    TQFrame *line = new TQFrame(this);
    line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    mainLayout->addWidget(line);
    
    mainLayout->addSpacing(5);
    
    // Schedule Banner
    m_scheduleBanner = new TQWidget(this);
    TQHBoxLayout *schedLayout = new TQHBoxLayout(m_scheduleBanner, 6, 6);
    
    m_lblScheduleIcon = new TQLabel(m_scheduleBanner);
    m_lblScheduleIcon->setPixmap(IconUtils::load(important_png, important_png_len, 16, 16));
    schedLayout->addWidget(m_lblScheduleIcon);
    
    m_lblScheduleText = new TQLabel(m_scheduleBanner);
    TQFont schedFont = m_lblScheduleText->font();
    schedFont.setItalic(true);
    m_lblScheduleText->setFont(schedFont);
    schedLayout->addWidget(m_lblScheduleText, 1);
    
    m_btnConfigureSchedule = new TQPushButton(IconUtils::load(schedule_png, schedule_png_len), i18n("Configure..."), m_scheduleBanner);
    m_btnConfigureSchedule->setFlat(true);
    connect(m_btnConfigureSchedule, TQT_SIGNAL(clicked()), this, TQT_SLOT(onConfigureScheduleClicked()));
    schedLayout->addWidget(m_btnConfigureSchedule);
    
    mainLayout->addWidget(m_scheduleBanner);
    mainLayout->addSpacing(5);
    
    // Targets Group
    TQVGroupBox *targetGroup = new TQVGroupBox(i18n("Scan Targets"), this);
    mainLayout->addWidget(targetGroup);
    
    m_targetList = new TQListBox(targetGroup);
    m_targetList->setMinimumHeight(80);
    m_targetList->insertItem(i18n("Drop files here or click Add..."));
    m_targetList->setAcceptDrops(true);
    m_targetList->installEventFilter(this);
    
    TQHBox *targetBtnBox = new TQHBox(targetGroup);
    targetBtnBox->setSpacing(6);
    TQWidget *spacer = new TQWidget(targetBtnBox);
    spacer->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
    
    m_btnSaveProfile = new TQPushButton(IconUtils::load(save_png, save_png_len), i18n("Save as Profile..."), targetBtnBox);
    connect(m_btnSaveProfile, TQT_SIGNAL(clicked()), this, TQT_SLOT(saveAsProfile()));
    
    m_btnRemoveTarget = new TQPushButton(IconUtils::load(trash_png, trash_png_len), i18n("Remove"), targetBtnBox);
    connect(m_btnRemoveTarget, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeTarget()));
    
    m_btnAddTarget = new TQPushButton(IconUtils::load(folder_new_png, folder_new_png_len), i18n("Add Target(s)..."), targetBtnBox);
    
    m_addMenu = new TQPopupMenu(this);
    m_addMenu->insertItem(IconUtils::load(file_png, file_png_len), i18n("Add File(s)..."), this, TQT_SLOT(addFileTarget()));
    m_addMenu->insertItem(IconUtils::load(folder_png, folder_png_len), i18n("Add Directory..."), this, TQT_SLOT(addDirectoryTarget()));
    m_btnAddTarget->setPopup(m_addMenu);
    
    // Action Buttons
    TQHBoxLayout *actionLayout = new TQHBoxLayout(mainLayout, 6);
    
    m_btnEicar = new TQPushButton(IconUtils::load(virus_png, virus_png_len), i18n("EICAR Test"), this);
    connect(m_btnEicar, TQT_SIGNAL(clicked()), this, TQT_SLOT(runEicarTest()));
    actionLayout->addWidget(m_btnEicar);
    
    actionLayout->addStretch(1);
    
    m_btnCancel = new TQPushButton(IconUtils::load(stop_png, stop_png_len), i18n("&Stop Scan"), this);
    connect(m_btnCancel, TQT_SIGNAL(clicked()), this, TQT_SLOT(cancelScan()));
    actionLayout->addWidget(m_btnCancel);
    
    m_btnStart = new TQPushButton(IconUtils::load(start_png, start_png_len), i18n("&Start Scan"), this);
    m_btnStart->setPaletteBackgroundColor(TQColor(120, 200, 100)); // Light green
    connect(m_btnStart, TQT_SIGNAL(clicked()), this, TQT_SLOT(startScan()));
    actionLayout->addWidget(m_btnStart);
    
    mainLayout->addSpacing(10);
    
    TQString backend = ClamAVDetection::isClamdscanInstalled() ? "clamdscan (daemon)" : "clamscan (standalone)";
    m_lblBackend = new TQLabel(i18n("Backend: %1").arg(backend), this);
    m_lblBackend->setAlignment(TQt::AlignCenter);
    TQFont f = m_lblBackend->font();
    f.setPointSize(f.pointSize() - 1);
    m_lblBackend->setFont(f);
    m_lblBackend->setPaletteForegroundColor(TQt::gray);
    mainLayout->addWidget(m_lblBackend);
    
    // Progress
    m_statusLabel = new TQLabel(i18n("Ready."), this);
    m_statusLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(m_statusLabel);
    
    m_progressBar = new TQProgressBar(this);
    m_progressBar->setTotalSteps(0); // Indeterminate
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);
    
    // Threat List
    TQLabel *lblThreats = new TQLabel(i18n("Detected Threats:"), this);
    mainLayout->addWidget(lblThreats);
    
    m_threatList = new TQListView(this);
    m_threatList->addColumn(i18n("File"));
    m_threatList->addColumn(i18n("Threat Name"));
    m_threatList->setAllColumnsShowFocus(true);
    mainLayout->addWidget(m_threatList);

    // Auto-select the last used scan profile if configured
    ClamMainWindow *mw = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mw && mw->settingsManager()) {
        TQString lastProfile = mw->settingsManager()->value("last_scan_profile", "");
        if (!lastProfile.isEmpty()) {
            for (int idx = 0; idx < m_profileCombo->count(); ++idx) {
                if (m_profileCombo->text(idx) == lastProfile) {
                    m_profileCombo->setCurrentItem(idx);
                    onProfileSelected(idx);
                    break;
                }
            }
        }
    }
}

void ScanView::reloadIcons()
{
    if (m_btnRemoveTarget) m_btnRemoveTarget->setIconSet(TQIconSet(IconUtils::load(trash_png, trash_png_len, 16, 16)));
    if (m_btnAddTarget) m_btnAddTarget->setIconSet(TQIconSet(IconUtils::load(folder_new_png, folder_new_png_len, 16, 16)));
    if (m_btnEicar) m_btnEicar->setIconSet(TQIconSet(IconUtils::load(virus_png, virus_png_len)));
    if (m_btnCancel) m_btnCancel->setIconSet(TQIconSet(IconUtils::load(stop_png, stop_png_len)));
    if (m_btnStart) m_btnStart->setIconSet(TQIconSet(IconUtils::load(start_png, start_png_len)));
    
    if (m_addMenu) {
        m_addMenu->changeItem(m_addMenu->idAt(0), IconUtils::load(file_png, file_png_len, 16, 16), i18n("Add File(s)..."));
        m_addMenu->changeItem(m_addMenu->idAt(1), IconUtils::load(folder_png, folder_png_len, 16, 16), i18n("Add Directory..."));
    }
}

void ScanView::setUIState(bool running)
{
    m_profileCombo->setEnabled(!running);
    m_btnAddTarget->setEnabled(!running);
    m_btnRemoveTarget->setEnabled(!running);
    m_targetList->setEnabled(!running);
    m_btnEicar->setEnabled(!running);
    m_btnStart->setEnabled(!running);
    m_btnCancel->setEnabled(running);
    
    if (running) {
        m_btnSaveProfile->setEnabled(false);
        m_progressBar->show();
    } else {
        m_progressBar->hide();
        updateSaveProfileButtonState();
    }
    
    if (m_isRunning != running) {
        m_isRunning = running;
        emit scanStateChanged(m_isRunning);
    }
}

void ScanView::addTarget()
{
    addDirectoryTarget();
}

void ScanView::addDirectoryTarget()
{
    // Clear placeholder
    if (m_targetPaths.isEmpty() && m_targetList->count() > 0) {
        m_targetList->clear();
    }
    
    TQString dir = KFileDialog::getExistingDirectory(
        TQString::null,
        this,
        i18n("Select Directory to Scan")
    );
    
    if (!dir.isEmpty() && !m_targetPaths.contains(dir)) {
        m_targetPaths.append(dir);
        m_targetList->insertItem(dir);
    }
    updateSaveProfileButtonState();
}

void ScanView::addFileTarget()
{
    // Clear placeholder
    if (m_targetPaths.isEmpty() && m_targetList->count() > 0) {
        m_targetList->clear();
    }
    
    TQStringList files = KFileDialog::getOpenFileNames(
        TQString::null,
        TQString::null,
        this,
        i18n("Select File(s) to Scan")
    );
    
    for (TQStringList::Iterator it = files.begin(); it != files.end(); ++it) {
        if (!m_targetPaths.contains(*it)) {
            m_targetPaths.append(*it);
            m_targetList->insertItem(*it);
        }
    }
    updateSaveProfileButtonState();
}

void ScanView::removeTarget()
{
    int index = m_targetList->currentItem();
    if (index >= 0 && index < (int)m_targetPaths.count()) {
        m_targetPaths.remove(m_targetList->text(index));
        m_targetList->removeItem(index);
    }
    
    if (m_targetList->count() == 0) {
        m_targetList->insertItem(i18n("Drop files here or click Add..."));
    }
    updateSaveProfileButtonState();
}

void ScanView::runEicarTest()
{
    // Write EICAR string to temporary file
    TQString eicarPath = "/tmp/eicar_com.zip"; // Simulated extension for detection test
    
    TQFile f(eicarPath);
    if (f.open(IO_WriteOnly)) {
        TQTextStream t(&f);
        t << "X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*";
        f.close();
    }
    
    m_targetPaths.clear();
    m_targetPaths.append(eicarPath);
    
    m_targetList->clear();
    m_targetList->insertItem(eicarPath);
    
    startScan();
}

void ScanView::triggerQuickScan()
{
    m_profileCombo->setCurrentItem(3); // Home Directory
    
    // Clear current targets
    m_targetPaths.clear();
    m_targetList->clear();
    
    // Set target path to home directory
    TQString homeDir = TQDir::homeDirPath();
    m_targetPaths.append(homeDir);
    m_targetList->insertItem(homeDir);
    
    // Start the scan
    startScan();
}

void ScanView::startScan()
{
    if (m_targetPaths.isEmpty()) {
        KMessageBox::error(this, i18n("Please specify a valid path to scan."));
        return;
    }
    
    m_threatList->clear();
    setUIState(true);
    
    m_fullLog = i18n("Starting scan...\nTargets:\n");
    for (int i = 0; i < (int)m_targetPaths.count(); ++i) {
        m_fullLog += " - " + m_targetPaths[i] + "\n";
    }
    m_fullLog += "\n";
    
    m_statusLabel->setText(i18n("<b>Scanning:</b> Initializing engine..."));
    m_scanStartTime.start();
    m_lastEtaStr = "";
    m_lastEtaUpdateMs = 0;
    
    // Read settings from the shared manager
    TQStringList exclusions;
    bool scanArchives = true;
    bool heuristic = true;
    TQString backend = "auto";
    int maxFileSizeMB = 25;
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        SettingsManager *s = mainWindow->settingsManager();
        backend = s->value("scan_backend", "auto");
        maxFileSizeMB = s->value("max_file_size", "25").toInt();
        scanArchives = (s->value("scan_archives", "true") == "true");
        heuristic = (s->value("heuristic_scan", "true") == "true");
        
        TQString rawExcl = s->value("global_exclusions", "");
        if (!rawExcl.isEmpty()) {
            exclusions = TQStringList::split(";", rawExcl);
        }
    }
    
    int niceLevel = 0;
    if (mainWindow && mainWindow->settingsManager()) {
        niceLevel = mainWindow->settingsManager()->value("scan_priority", "0").toInt();
    }
    
    m_totalFiles = -1;
    m_progressBar->setTotalSteps(0);
    m_progressBar->setProgress(0);
    m_pulseStep = 0;
    
    if (m_counterThread) {
        m_counterThread->cancel();
        m_counterThread->wait();
        delete m_counterThread;
    }
    
    m_counterThread = new FileCounterThread(m_targetPaths, exclusions);
    m_counterThread->start();
    m_counterTimer->start(30); // Check thread & fast pulse
    
    m_scanner->startScan(m_targetPaths, exclusions, scanArchives, heuristic, backend, maxFileSizeMB, niceLevel);
}

void ScanView::onCounterTimeout()
{
    // Check if counter thread finished
    if (m_counterThread && m_counterThread->finished()) {
        m_totalFiles = m_counterThread->count();
        delete m_counterThread;
        m_counterThread = 0;
    }
    
    // Keep pulsing while counting OR while engine hasn't started scanning yet
    if (m_totalFiles <= 0 || m_scanner->filesScanned() == 0) {
        m_pulseStep += 4;
        m_progressBar->setProgress(m_pulseStep);
    } else {
        // Both conditions met: count is known AND scan has started producing output
        m_counterTimer->stop();
        m_progressBar->setTotalSteps(m_totalFiles);
        m_progressBar->setProgress(m_scanner->filesScanned());
    }
}

void ScanView::cancelScan()
{
    if (m_counterThread) {
        m_counterThread->cancel();
    }
    m_counterTimer->stop();
    m_scanner->cancelScan();
    m_statusLabel->setText(i18n("<b>Scan aborted by user.</b>"));
    setUIState(false);
}

void ScanView::onScanProgress(int filesScanned, const TQString &currentFile)
{
    // Limit string length for display
    TQString displayFile = currentFile;
    if (displayFile.length() > 60) {
        displayFile = "..." + displayFile.right(57);
    }
    
    TQString coloredFile = TQString("<font color=\"#3498db\"><b>%1</b></font>").arg(displayFile);
    
    if (m_totalFiles > 0) {
        m_progressBar->setProgress(filesScanned);
        int percent = (int)(((double)filesScanned / m_totalFiles) * 100);
        
        int elapsedMs = m_scanStartTime.elapsed();
        // Compute ETA if we have enough credible samples (e.g. > 30 secs)
        if (elapsedMs > 30000 && m_totalFiles > filesScanned) {
            // Update the cached ETA string only every 5 seconds to reduce UI fluctuation
            if (elapsedMs - m_lastEtaUpdateMs > 5000 || m_lastEtaStr.isEmpty()) {
                // This formula uses the total global average (Total Elapsed / Total Scanned)
                double msPerFile = (double)elapsedMs / filesScanned;
                int remainingFiles = m_totalFiles - filesScanned;
                int remainingSecs = (int)((remainingFiles * msPerFile) / 1000.0);
                
                if (remainingSecs < 60) {
                    m_lastEtaStr = i18n(" - (<i>ETA: %1 sec</i>)").arg(remainingSecs);
                } else if (remainingSecs < 3600) {
                    m_lastEtaStr = i18n(" - (<i>ETA: %1 min %2 sec</i>)").arg(remainingSecs / 60).arg(remainingSecs % 60);
                } else {
                    int hours = remainingSecs / 3600;
                    int mins = (remainingSecs % 3600) / 60;
                    int secs = remainingSecs % 60;
                    m_lastEtaStr = i18n(" - (<i>ETA: %1 hours %2 min %3 sec</i>)").arg(hours).arg(mins).arg(secs);
                }
                m_lastEtaUpdateMs = elapsedMs;
            }
        } else {
            m_lastEtaStr = "";
        }
        
        m_statusLabel->setText(i18n("<b>Scanning (%1% - %2/%3 files)%4:</b> %5")
            .arg(percent).arg(filesScanned).arg(m_totalFiles).arg(m_lastEtaStr).arg(coloredFile));
        emit scanProgressUpdated(percent);
    } else {
        m_statusLabel->setText(i18n("<b>Scanning (%1 files):</b> %2")
            .arg(filesScanned).arg(coloredFile));
        emit scanProgressUpdated(-1);
    }
}

void ScanView::onScanFinished(int status, int infectedCount)
{
    if (m_counterThread) {
        m_counterThread->cancel();
        m_counterThread->wait();
        delete m_counterThread;
        m_counterThread = 0;
    }
    m_counterTimer->stop();
    
    setUIState(false);
    
    // Emit scanFinishedEvent BEFORE showing dialog or switching views
    // to prevent ClamMainWindow from mistakenly firing a system notification.
    emit scanFinishedEvent(status, infectedCount);
    
    int logStatus = 0;
    TQString logDetails;
    
    switch (status) {
        case Scanner::CLEAN:
            m_statusLabel->setText(i18n("<b>Scan complete. No threats found.</b>"));
            logStatus = 0;
            logDetails = m_fullLog + "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: 0\n";
            break;
        case Scanner::INFECTED:
            m_statusLabel->setText(i18n("<b>Scan complete. %1 threats found!</b>").arg(infectedCount));
            logStatus = 1;
            logDetails = m_fullLog + "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(infectedCount) + "\n";
            break;
        case Scanner::ERROR:
            m_statusLabel->setText(i18n("<b>Scan failed.</b>"));
            if (isVisible()) KMessageBox::error(this, i18n("An error occurred during the scan. Please check the logs for more information."), i18n("Scan Error"));
            logStatus = 2;
            logDetails = m_fullLog + "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(infectedCount) + "\n";
            break;
        case Scanner::CANCELLED:
            m_statusLabel->setText(i18n("<b>Scan was cancelled.</b>"));
            logStatus = 2;
            logDetails = m_fullLog + "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(infectedCount)
                       + "\nScan cancelled by user.\n";
            break;
    }
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow) {
        if (mainWindow->getLogManager()) {
            double duration = m_scanStartTime.elapsed() / 1000.0;
            mainWindow->getLogManager()->addLog(0, logStatus, logDetails, duration); // 0 = Scan
        }
        
        if (isVisible() && (status == Scanner::CLEAN || status == Scanner::INFECTED)) {
            ScanResultsDialog dialog(mainWindow, 
                m_targetPaths,
                m_scanner->filesScanned(), 
                m_scanStartTime.elapsed() / 1000, 
                m_threatList, 
                mainWindow->quarantineManager());
            if (dialog.exec() == 10) {
                emit showLogsRequested();
            }
        }
    }
}

void ScanView::onThreatFound(const TQString &filePath, const TQString &threatName)
{
    new TQListViewItem(m_threatList, filePath, threatName);
}

void ScanView::onScanError(const TQString &errorMessage)
{
    m_fullLog += "\nERROR: " + errorMessage + "\n";
    m_statusLabel->setText(i18n("<b>Error:</b> %1").arg(errorMessage));
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->getLogManager()) {
        mainWindow->getLogManager()->addLog(0, 2, m_fullLog); // 0=Scan, 2=Error
    }
    setUIState(false);
}

void ScanView::onScanLog(const TQString &logLine)
{
    m_fullLog += logLine + "\n";
}

bool ScanView::eventFilter(TQObject *obj, TQEvent *event)
{
    if (obj == m_targetList) {
        if (event->type() == TQEvent::DragEnter) {
            TQDragEnterEvent *dee = static_cast<TQDragEnterEvent*>(event);
            if (TQUriDrag::canDecode(dee)) {
                dee->accept();
                return true;
            }
        } else if (event->type() == TQEvent::Drop) {
            TQDropEvent *de = static_cast<TQDropEvent*>(event);
            TQStrList uris;
            if (TQUriDrag::decode(de, uris)) {
                // Clear placeholder if present
                if (m_targetPaths.isEmpty() && m_targetList->count() > 0) {
                    m_targetList->clear();
                }
                for (TQStrListIterator it(uris); it.current(); ++it) {
                    KURL url(it.current());
                    if (url.isLocalFile()) {
                        TQString path = url.path();
                        if (!m_targetPaths.contains(path)) {
                            m_targetPaths.append(path);
                            m_targetList->insertItem(path);
                        }
                    }
                }
                updateSaveProfileButtonState();
                return true;
            }
        }
    }
    return TQWidget::eventFilter(obj, event);
}

void ScanView::onProfileSelected(int index)
{
    m_targetPaths.clear();
    m_targetList->clear();
    
    // Clear previous scan results and status
    m_threatList->clear();
    m_statusLabel->setText(i18n("Ready."));
    m_progressBar->hide();
    
    if (index == 0) {
        // Manual mode
        m_targetList->insertItem(i18n("Drop files here or click Add..."));
    } else {
        ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
        if (mainWindow && mainWindow->profileManager()) {
            TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
            if (index - 1 >= 0 && index - 1 < (int)profiles.count()) {
                ScanProfile p = profiles[index - 1];
                for (TQStringList::ConstIterator it = p.paths.begin(); it != p.paths.end(); ++it) {
                    m_targetPaths.append(*it);
                    m_targetList->insertItem(*it);
                }
            }
        }
    }
    
    updateSaveProfileButtonState();
    
    // Save last used profile
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        TQString profileName = m_profileCombo->text(index);
        mainWindow->settingsManager()->setValue("last_scan_profile", profileName);
        mainWindow->settingsManager()->save();
    }
}

void ScanView::onConfigureScheduleClicked()
{
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow) {
        mainWindow->showPreferences(4); // Show "Scheduled" page (index 4)
        updateScheduledScanBanner();
    }
}

void ScanView::updateScheduledScanBanner()
{
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (!mainWindow) {
        TQObject *p = parent();
        while (p) {
            mainWindow = dynamic_cast<ClamMainWindow*>(p);
            if (mainWindow) break;
            p = p->parent();
        }
    }
    
    if (!mainWindow || !mainWindow->settingsManager()) return;
    
    SettingsManager *s = mainWindow->settingsManager();
    bool enabled = (s->value("schedule_enabled", "false") == "true");
    
    if (enabled) {
        TQString freq = s->value("schedule_frequency", "daily");
        TQString timeStr = s->value("schedule_time", "02:00");
        TQString profileId = s->value("schedule_profile_id", "");
        
        TQString profileName = i18n("Full Scan");
        if (!profileId.isEmpty() && mainWindow->profileManager()) {
            TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
            for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
                if ((*it).id == profileId) {
                    profileName = (*it).name;
                    break;
                }
            }
        }
        
        TQString freqStr;
        if (freq == "weekly") freqStr = i18n("Weekly");
        else if (freq == "monthly") freqStr = i18n("Monthly");
        else freqStr = i18n("Daily");
        
        m_lblScheduleText->setText(TQString(i18n("Next scheduled scan: %1 at %2 (%3)"))
                                   .arg(freqStr).arg(timeStr).arg(profileName));
        m_lblScheduleText->setPaletteForegroundColor(TQt::darkGreen);
        m_lblScheduleIcon->setPixmap(IconUtils::load(secu_ok_png, secu_ok_png_len, 16, 16));
    } else {
        m_lblScheduleText->setText(i18n("Automatic scheduled scans are disabled"));
        m_lblScheduleText->setPaletteForegroundColor(TQt::gray);
        m_lblScheduleIcon->setPixmap(IconUtils::load(important_png, important_png_len, 16, 16));
    }
}

void ScanView::showEvent(TQShowEvent *event)
{
    TQWidget::showEvent(event);
    updateScheduledScanBanner();
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->settingsManager()) {
        disconnect(mainWindow->settingsManager(), TQT_SIGNAL(settingsChanged()), this, TQT_SLOT(updateScheduledScanBanner()));
        connect(mainWindow->settingsManager(), TQT_SIGNAL(settingsChanged()), this, TQT_SLOT(updateScheduledScanBanner()));
    }
}

void ScanView::saveAsProfile()
{
    if (m_targetPaths.isEmpty()) {
        KMessageBox::sorry(this, i18n("Please add at least one scan target (file or directory) to save as a profile."));
        return;
    }
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (!mainWindow || !mainWindow->profileManager()) return;
    
    bool ok = false;
    TQString name = KInputDialog::getText(
        i18n("Save as Profile"),
        i18n("Enter a name for the new scan profile:"),
        TQString::null,
        &ok,
        this
    );
    
    if (!ok) return;
    
    name = name.stripWhiteSpace();
    if (name.isEmpty()) {
        KMessageBox::sorry(this, i18n("Profile name cannot be empty."));
        return;
    }
    
    // Check for duplicates
    TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
    for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
        if ((*it).name.lower() == name.lower()) {
            KMessageBox::sorry(this, i18n("A profile with the name '%1' already exists. Please choose a unique name.").arg(name));
            return;
        }
    }
    
    ScanProfile newProfile;
    newProfile.name = name;
    newProfile.description = i18n("Custom profile created on %1.").arg(TQDate::currentDate().toString(TQt::LocalDate));
    newProfile.paths = m_targetPaths;
    newProfile.scanArchives = true;
    newProfile.heuristicScan = true;
    
    m_newProfileToSelect = name;
    mainWindow->profileManager()->addProfile(newProfile);
    
    KMessageBox::information(this, i18n("Scan profile '%1' has been saved successfully.").arg(name));
}

void ScanView::reloadProfiles()
{
    TQString selectedText = m_profileCombo->currentText();
    if (!m_newProfileToSelect.isEmpty()) {
        selectedText = m_newProfileToSelect;
        m_newProfileToSelect = TQString::null;
    }
    
    m_profileCombo->clear();
    m_profileCombo->insertItem(i18n("No Profile (Manual)"));
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->profileManager()) {
        TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
        int newIdx = 0;
        int i = 1;
        for (TQValueList<ScanProfile>::ConstIterator it = profiles.begin(); it != profiles.end(); ++it) {
            m_profileCombo->insertItem((*it).name);
            if ((*it).name == selectedText) {
                newIdx = i;
            }
            i++;
        }
        m_profileCombo->setCurrentItem(newIdx);
        onProfileSelected(newIdx);
    }
}

void ScanView::updateSaveProfileButtonState()
{
    if (!m_btnSaveProfile) return;
    
    int index = m_profileCombo->currentItem();
    
    if (m_targetPaths.isEmpty()) {
        m_btnSaveProfile->setEnabled(false);
        return;
    }
    
    if (index == 0) {
        m_btnSaveProfile->setEnabled(true);
        return;
    }
    
    ClamMainWindow *mainWindow = dynamic_cast<ClamMainWindow*>(topLevelWidget());
    if (mainWindow && mainWindow->profileManager()) {
        TQValueList<ScanProfile> profiles = mainWindow->profileManager()->getProfiles();
        if (index - 1 >= 0 && index - 1 < (int)profiles.count()) {
            ScanProfile p = profiles[index - 1];
            
            TQStringList current = m_targetPaths;
            TQStringList original = p.paths;
            current.sort();
            original.sort();
            
            if (current == original) {
                m_btnSaveProfile->setEnabled(false);
            } else {
                m_btnSaveProfile->setEnabled(true);
            }
            return;
        }
    }
    
    m_btnSaveProfile->setEnabled(false);
}

#include "scan_view.moc"
