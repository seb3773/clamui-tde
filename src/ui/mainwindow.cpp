/*
 * mainwindow.cpp - Main application window implementation
 */

#include "mainwindow.h"
#include "../tray.h"
#include "sidebar.h"

/* Views */
#include "audit_view.h"
#include "components_view.h"
#include "logs_view.h"
#include "preferences/prefs_dialog.h"
#include "about_dialog.h"
#include "quarantine_view.h"
#include "scan_view.h"
#include "statistics_view.h"
#include "update_view.h"
#include <tdeapplication.h>
#include "icon_utils.h"

/* Core managers */
#include "../core/log_manager.h"
#include "../core/profiles.h"
#include "../core/notification_manager.h"
#include "../core/quarantine_manager.h"
#include "../core/settings_manager.h"

#include <kstatusbar.h>
#include <ntqtimer.h>
#include <tdeaction.h>
#include "../core/clamav_detection.h"
#include <tdeconfig.h>
#include <tdelocale.h>
#include <tdemenubar.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>
#include <tdestdaccel.h>

#include <ntqlayout.h>
#include <ntqlabel.h>
#include <ntqsplitter.h>
#include <ntqfile.h>
#include <ntqtextstream.h>
#include <ntqwidgetstack.h>

/* View indices — must match sidebar order */
enum ViewIndex {
  VIEW_SCAN = 0,
  VIEW_UPDATE,
  VIEW_LOGS,
  VIEW_COMPONENTS,
  VIEW_QUARANTINE,
  VIEW_STATISTICS,
  VIEW_AUDIT,
  VIEW_COUNT
};

/* ================================================================== */
/* Construction / Destruction                                         */
/* ================================================================== */

ClamMainWindow::ClamMainWindow()
    : TDEMainWindow(0, "ClamMainWindow"), m_settings(0), m_logManager(0),
      m_quarantine(0), m_profiles(0), m_splitter(0), m_sidebar(0),
      m_viewStack(0), m_tray(0), m_scanView(0), m_updateView(0), m_logsView(0),
      m_quarantineView(0), m_statsView(0), m_auditView(0), m_quickScanAction(0),
      m_scanFileAction(0), m_updateDbAction(0), m_prefsAction(0),
      m_scheduledAutoQuit(false), m_userShownWindow(false), m_dbAgeChecked(false) {
  setCaption(i18n("TDE ClamUI"));
  setIcon(IconUtils::load(main_icon_png, main_icon_png_len));
  setMinimumSize(1100, 650);
  resize(1100, 650);

  setupManagers();
  setupActions();
  setupUI();
  setupStatusBar();
  createViews();

  /* Select scan view by default */
  switchToView(VIEW_SCAN);
}

ClamMainWindow::~ClamMainWindow() {
  delete m_profiles;
  delete m_quarantine;
  delete m_logManager;
  delete m_settings;
}

/* ================================================================== */
/* Manager Initialization                                             */
/* ================================================================== */

void ClamMainWindow::setupManagers() {
  m_settings = new SettingsManager();
  m_logManager = new LogManager();
  m_quarantine = new QuarantineManager();
  m_profiles = new ProfileManager();
}

/* ================================================================== */
/* Actions & Menus                                                    */
/* ================================================================== */

void ClamMainWindow::setupActions() {
  /* File menu actions */
  m_quickScanAction = new TDEAction(i18n("&System Scan"), TQIconSet(IconUtils::load(start_png, start_png_len, 16, 16)),
                                    0, // No shortcut
                                    this, TQT_SLOT(slotQuickScan()),
                                    actionCollection(), "quick_scan");

  m_scanFileAction = new TDEAction(i18n("Scan &File..."), TQIconSet(IconUtils::load(file_png, file_png_len, 16, 16)),
                                   0, // No shortcut
                                   this, TQT_SLOT(slotScanFile()),
                                   actionCollection(), "scan_file");

  m_scanFolderAction = new TDEAction(i18n("Scan F&older..."), TQIconSet(IconUtils::load(folder_png, folder_png_len, 16, 16)),
                                     0, // No shortcut
                                     this, TQT_SLOT(slotScanFolder()),
                                     actionCollection(), "scan_folder");

  m_updateDbAction = new TDEAction(
      i18n("&Update Database"), TQIconSet(IconUtils::load(refresh_png, refresh_png_len, 16, 16)),
      0, // No shortcut
      this, TQT_SLOT(slotUpdateDatabase()), actionCollection(), "update_db");

  m_aboutAction =
      new TDEAction(i18n("&About ClamUI"), TQIconSet(IconUtils::load(about_png, about_png_len, 16, 16)),
                    0, this, TQT_SLOT(slotAbout()), actionCollection(), "about_app");

  m_prefsAction = KStdAction::preferences(this, TQT_SLOT(showPreferences()),
                                          actionCollection());
  m_prefsAction->setIconSet(TQIconSet(IconUtils::load(config_png, config_png_len, 16, 16)));

  TDEAction *quitAction = KStdAction::quit(this, TQT_SLOT(slotQuitApp()), actionCollection());
  quitAction->setIconSet(TQIconSet(IconUtils::load(quit_png, quit_png_len, 16, 16)));

  /* Build menu bar */
  TDEPopupMenu *fileMenu = new TDEPopupMenu(this);
  m_quickScanAction->plug(fileMenu);
  m_scanFileAction->plug(fileMenu);
  m_scanFolderAction->plug(fileMenu);
  fileMenu->insertSeparator();
  m_updateDbAction->plug(fileMenu);
  fileMenu->insertSeparator();
  m_prefsAction->plug(fileMenu);
  fileMenu->insertSeparator();
  quitAction->plug(fileMenu);

  menuBar()->insertItem(i18n("&File"), fileMenu);

  TDEPopupMenu *helpMenu = new TDEPopupMenu(this);
  m_aboutAction->plug(helpMenu);
  menuBar()->insertItem(i18n("&Help"), helpMenu);
}

/* ================================================================== */
/* UI Layout                                                          */
/* ================================================================== */

void ClamMainWindow::setupUI() {
  m_splitter = new TQSplitter(TQt::Horizontal, this);
  setCentralWidget(m_splitter);

  /* Left pane: sidebar navigation */
  m_sidebar = new ClamSidebar(m_splitter);
  m_sidebar->setMinimumWidth(180);
  m_sidebar->setMaximumWidth(240);

  connect(m_sidebar, TQT_SIGNAL(viewSelected(int)), this,
          TQT_SLOT(slotSidebarClicked(int)));

  /* Right pane: view stack */
  m_viewStack = new TQWidgetStack(m_splitter);

  /* Splitter proportions: sidebar 20%, content 80% */
  TQValueList<int> sizes;
  sizes.append(200);
  sizes.append(720);
  m_splitter->setSizes(sizes);
}

void ClamMainWindow::setupStatusBar() {
  statusBar()->insertItem("", 0, 1, true); /* Stretch spacer */
  
  statusBar()->insertItem(i18n("ClamAV: Checking..."), 1, 0);
  statusBar()->insertItem(i18n("DB: Checking..."), 2, 0);
  statusBar()->insertItem(i18n("Daemon: Checking..."), 3, 0);
  
  // Scanning indicator (hidden by default)
  m_scanStatusLabel = new TQLabel(i18n(" Scanning... "), statusBar());
  m_scanStatusLabel->setPaletteForegroundColor(TQColor("#E67E00")); // Orange
  TQFont f = m_scanStatusLabel->font();
  f.setBold(true);
  m_scanStatusLabel->setFont(f);
  m_scanStatusLabel->hide();
  statusBar()->addWidget(m_scanStatusLabel, 0, true); // permanent = right side
  
  TQTimer *statusTimer = new TQTimer(this);
  connect(statusTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(updateStatusBarStats()));
  statusTimer->start(10000); // 10 seconds
  
  TQTimer::singleShot(50, this, TQT_SLOT(updateStatusBarStats()));
}

void ClamMainWindow::updateStatusBarStats() {
  TQString cv = ClamAVDetection::getClamscanVersion();
  if (cv.isEmpty()) cv = i18n("Not installed");
  statusBar()->changeItem(i18n("ClamAV: %1").arg(cv), 1);
  
  TQString db = ClamAVDetection::getDatabaseDate();
  if (db.isEmpty()) db = i18n("Unknown");
  statusBar()->changeItem(i18n("DB: %1").arg(db), 2);
  
  ClamAVDetection::DaemonStatus ds = ClamAVDetection::getDaemonStatus();
  TQString dsStr = i18n("Unknown");
  if (ds == ClamAVDetection::DaemonRunning) dsStr = TQString("<font color=\"#3498db\">%1</font>").arg(i18n("Running"));
  else if (ds == ClamAVDetection::DaemonStopped) dsStr = TQString("<font color=\"#e67e22\">%1</font>").arg(i18n("Stopped"));
  else dsStr = i18n("Not installed");
  
  TQString fullText = i18n("Daemon: %1").arg(dsStr);
  statusBar()->changeItem(TQString("<nobr> - %1</nobr>").arg(fullText), 3);
}

/* ================================================================== */
/* View Creation                                                      */
/* ================================================================== */

void ClamMainWindow::createViews() {
  m_scanView = new ScanView(this, m_viewStack);
  connect(m_scanView, TQT_SIGNAL(scanStateChanged(bool)), this, TQT_SLOT(onScanStateChanged(bool)));
  connect(m_scanView, TQT_SIGNAL(scanFinishedEvent(int, int)), this, TQT_SLOT(onScanFinishedEvent(int, int)));
  connect(m_scanView, TQT_SIGNAL(scanProgressUpdated(int)), this, TQT_SLOT(onScanProgress(int)));
  connect(m_scanView, TQT_SIGNAL(showLogsRequested()), this, TQT_SLOT(slotShowLatestLog()));
  
  m_updateView = new UpdateView(this, m_viewStack);
  connect(m_updateView, TQT_SIGNAL(updateFinishedEvent(int)), this, TQT_SLOT(onUpdateFinishedEvent(int)));
  connect(m_updateView, TQT_SIGNAL(updateStateChanged(bool)), this, TQT_SLOT(onUpdateStateChanged(bool)));
  m_logsView = new LogsView(this, m_viewStack, m_logManager);
  m_componentsView = new ComponentsView(this, m_viewStack);
  m_quarantineView = new QuarantineView(this, m_viewStack, m_quarantine);
  m_statsView = new StatisticsView(this, m_viewStack);
  m_auditView = new AuditView(this, m_viewStack);
  
  connect(m_settings, TQT_SIGNAL(settingsChanged()), m_statsView, TQT_SLOT(onSettingsChanged()));
  connect(m_settings, TQT_SIGNAL(settingsChanged()), m_componentsView, TQT_SLOT(onSettingsChanged()));
  connect(m_settings, TQT_SIGNAL(settingsChanged()), m_updateView, TQT_SLOT(onSettingsChanged()));
  connect(m_settings, TQT_SIGNAL(settingsChanged()), this, TQT_SLOT(onSettingsChanged()));

  m_viewStack->addWidget(m_scanView, VIEW_SCAN);
  m_viewStack->addWidget(m_updateView, VIEW_UPDATE);
  m_viewStack->addWidget(m_logsView, VIEW_LOGS);
  m_viewStack->addWidget(m_componentsView, VIEW_COMPONENTS);
  m_viewStack->addWidget(m_quarantineView, VIEW_QUARANTINE);
  m_viewStack->addWidget(m_statsView, VIEW_STATISTICS);
  m_viewStack->addWidget(m_auditView, VIEW_AUDIT);
}

/* ================================================================== */
/* View Switching                                                     */
/* ================================================================== */

void ClamMainWindow::switchToView(int index) {
  if (index < 0 || index >= VIEW_COUNT)
    return;

  m_viewStack->raiseWidget(index);
  m_sidebar->setCurrentIndex(index);
}

void ClamMainWindow::slotSidebarClicked(int index) { switchToView(index); }

void ClamMainWindow::slotShowLatestLog() {
  switchToView(VIEW_LOGS);
  m_logsView->selectLatestLog();
}

/* ================================================================== */
/* Action Slots                                                       */
/* ================================================================== */

void ClamMainWindow::slotQuickScan() {
  switchToView(VIEW_SCAN);
  m_scanView->triggerQuickScan();
}

void ClamMainWindow::slotScanFile() {
  switchToView(VIEW_SCAN);
  m_scanView->addFileTarget();
}

void ClamMainWindow::slotScanFolder() {
  switchToView(VIEW_SCAN);
  m_scanView->addDirectoryTarget();
}

void ClamMainWindow::slotUpdateDatabase() {
  switchToView(VIEW_UPDATE);
  m_updateView->startUpdate();
}

void ClamMainWindow::showPreferences(int initialPage) {
  PrefsDialog dlg(this, m_settings, m_profiles, initialPage);
  dlg.exec();
}

void ClamMainWindow::slotAbout() {
  AboutDialog dlg(this);
  dlg.exec();
}

void ClamMainWindow::slotQuitApp() {
  if (isScanRunning()) {
    int res = KMessageBox::warningContinueCancel(this,
        i18n("A scan is currently running. If you quit now, the scan will be cancelled. Are you sure you want to quit?"),
        i18n("Scan in Progress"),
        KGuiItem(i18n("Stop Scan and Quit"), "process-stop"));
    if (res != KMessageBox::Continue) {
        return;
    }
    if (m_scanView) m_scanView->cancelScan();
  }
  kapp->quit();
}

bool ClamMainWindow::isScanRunning() const {
  if (m_scanView) {
    return m_scanView->isScanRunning();
  }
  return false;
}

void ClamMainWindow::onScanStateChanged(bool running) {
  // Disable all scan-related actions during scan
  m_quickScanAction->setEnabled(!running);
  m_scanFileAction->setEnabled(!running);
  m_scanFolderAction->setEnabled(!running);
  m_updateDbAction->setEnabled(!running);
  
  // Status bar scanning indicator
  if (running) {
    m_scanStatusLabel->show();
  } else {
    m_scanStatusLabel->hide();
  }
  
  // Update the UpdateView buttons
  if (m_updateView) {
    m_updateView->setScanRunningState(running);
  }
  
  // Update the AuditView buttons
  if (m_auditView) {
    m_auditView->setScanRunningState(running);
  }
  
  // Update the StatisticsView buttons
  if (m_statsView) {
    m_statsView->setScanRunningState(running);
  }
  
  // Update tray icon
  if (m_tray) {
    m_tray->setStatus(running ? 1 : 0);
  }
}

void ClamMainWindow::onUpdateStateChanged(bool running) {
  // Disable all scan-related actions during database update
  m_quickScanAction->setEnabled(!running);
  m_scanFileAction->setEnabled(!running);
  m_scanFolderAction->setEnabled(!running);
  m_updateDbAction->setEnabled(!running);
  
  // Disable statistics scan button during update
  if (m_statsView) {
    m_statsView->setScanRunningState(running);
  }
}

void ClamMainWindow::onScanProgress(int percent) {
  if (m_tray) {
    m_tray->updateScanProgress(percent);
  }
  
  // Update status bar label
  if (percent >= 0) {
    m_scanStatusLabel->setText(i18n(" Scanning... %1% ").arg(percent));
  }
}

void ClamMainWindow::onScanFinishedEvent(int status, int infectedCount) {
  // If it's a scheduled headless scan and it's clean, exit completely silently
  if (m_scheduledAutoQuit && !m_userShownWindow && status == 0) {
      tqApp->quit();
      return;
  }

  if (isHidden() || m_sidebar->currentIndex() != VIEW_SCAN) {
    TQString title = i18n("Scan Finished");
    TQString msg;
    if (status == 0) { // Scanner::CLEAN
      msg = i18n("Scan completed. No threats found.");
    } else if (status == 1) { // Scanner::INFECTED
      msg = i18n("Scan completed. %1 threats found!").arg(infectedCount);
      title = i18n("Threats Detected!");
    } else {
      msg = i18n("Scan failed or was cancelled.");
    }
    
    // Only send notification if not a silent background exit (which we handled above)
    // Or if there was an error/infection in background
    NotificationManager::instance()->sendNotification(title, msg);
  }
  
  /* Auto-quit only if we didn't find threats! 
     If we found threats (status == 1) or error (status == 2), we STAY ALIVE 
     so the user can click the tray icon (which is now red) to see the results! */
  if (m_scheduledAutoQuit && !m_userShownWindow && status == 0) {
      tqApp->quit();
  }
}

void ClamMainWindow::onUpdateFinishedEvent(int status) {
  if (isHidden() || m_sidebar->currentIndex() != VIEW_UPDATE) {
    TQString title = i18n("Database Update");
    TQString msg;
    if (status == 0) {
      msg = i18n("Virus database updated successfully.");
    } else {
      msg = i18n("Virus database update failed.");
    }
    NotificationManager::instance()->sendNotification(title, msg);
  }
}

void ClamMainWindow::onSettingsChanged() {
  IconUtils::clearCache();
  
  if (m_sidebar) {
    m_sidebar->reloadIcons();
  }
  if (m_tray) {
    m_tray->reloadIcons();
  }
  
  if (m_scanView) m_scanView->reloadIcons();
  if (m_updateView) m_updateView->reloadIcons();
  if (m_logsView) m_logsView->reloadIcons();
  if (m_quarantineView) m_quarantineView->reloadIcons();
  if (m_componentsView) m_componentsView->reloadIcons();
  if (m_statsView) m_statsView->reloadIcons();
  if (m_auditView) m_auditView->reloadIcons();
  
  // Reload actions icons
  m_quickScanAction->setIconSet(TQIconSet(IconUtils::load(start_png, start_png_len, 16, 16)));
  m_scanFileAction->setIconSet(TQIconSet(IconUtils::load(file_png, file_png_len, 16, 16)));
  m_scanFolderAction->setIconSet(TQIconSet(IconUtils::load(folder_png, folder_png_len, 16, 16)));
  m_updateDbAction->setIconSet(TQIconSet(IconUtils::load(refresh_png, refresh_png_len, 16, 16)));
  m_prefsAction->setIconSet(TQIconSet(IconUtils::load(config_png, config_png_len, 16, 16)));
  m_aboutAction->setIconSet(TQIconSet(IconUtils::load(about_png, about_png_len, 16, 16)));
}

/* ================================================================== */
/* Tray Integration                                                   */
/* ================================================================== */

void ClamMainWindow::setTray(ClamTray *tray) { m_tray = tray; }

/* ================================================================== */
/* Close Behavior                                                     */
/* ================================================================== */

bool ClamMainWindow::queryClose() {
  /* If tray is visible, minimize to tray instead of quitting */
  if (m_tray && m_settings) {
    TQString behavior = m_settings->value("close_behavior", "quit");
    bool minimizeToTray = m_settings->value("minimize_to_tray", "false") == "true";
    
    if (behavior == "minimize" || minimizeToTray) {
      hide();
      return false;
    }
  }
  
  /* Explicitly quit the application event loop */
  if (isScanRunning()) {
    int res = KMessageBox::warningContinueCancel(this,
        i18n("A scan is currently running. If you quit now, the scan will be cancelled. Are you sure you want to quit?"),
        i18n("Scan in Progress"),
        KGuiItem(i18n("Stop Scan and Quit"), "process-stop"));
    if (res != KMessageBox::Continue) {
        return false;
    }
    if (m_scanView) m_scanView->cancelScan();
  }
  
  kapp->quit();
  return true;
}

void ClamMainWindow::saveProperties(TDEConfig *config) {
  config->writeEntry("CurrentView", m_sidebar->currentIndex());
  config->writeEntry("Width", width());
  config->writeEntry("Height", height());
}

void ClamMainWindow::readProperties(TDEConfig *config) {
  int view = config->readNumEntry("CurrentView", VIEW_SCAN);
  switchToView(view);
  int w = config->readNumEntry("Width", 920);
  int h = config->readNumEntry("Height", 640);
  resize(w, h);
}

void ClamMainWindow::startScheduledScan(const TQString &profileId, bool autoQuit)
{
  if (isScanRunning()) {
    NotificationManager::instance()->sendNotification(
        i18n("Scheduled Scan"),
        i18n("A scan is already in progress. Scheduled scan skipped."));
    if (autoQuit && !m_userShownWindow) {
      tqApp->quit();
    }
    return;
  }

  m_scheduledAutoQuit = autoQuit;

  /* Load profile and populate scan view targets */
  if (!m_profiles || !m_scanView) {
      return;
  }

  ScanProfile profile = m_profiles->getProfile(profileId);
  if (profile.id.isEmpty()) {
    NotificationManager::instance()->sendNotification(
        i18n("Scheduled Scan Error"),
        i18n("Profile not found: %1").arg(profileId));
    if (autoQuit) tqApp->quit();
    return;
  }

  /* Find profile index in combo (index 0 = Manual) */
  TQValueList<ScanProfile> profiles = m_profiles->getProfiles();
  int comboIdx = 0;
  for (int i = 0; i < (int)profiles.count(); ++i) {
    if (profiles[i].id == profileId) {
      comboIdx = i + 1; /* +1 because index 0 = "No Profile (Manual)" */
      break;
    }
  }

  /* Trigger profile selection which loads targets, then start scan */
  m_scanView->onProfileSelected(comboIdx);
  switchToView(VIEW_SCAN);
  m_scanView->startScan();
}

void ClamMainWindow::userShowedWindow()
{
  m_userShownWindow = true;
}

void ClamMainWindow::showEvent(TQShowEvent *e)
{
  /* If the window is shown, it means the user interacts with the UI, 
     so we should no longer auto-quit when a background scan finishes. */
  m_userShownWindow = true;
  if (!m_dbAgeChecked) {
    m_dbAgeChecked = true;
    TQTimer::singleShot(200, this, TQT_SLOT(checkDatabaseAge()));
  }
  TDEMainWindow::showEvent(e);
}

void ClamMainWindow::checkDatabaseAge() {
  if (isScanRunning()) return;

  int dbAge = ClamAVDetection::getDatabaseAgeDays();
  if (dbAge > 7 || dbAge == -1) {
    TQString msg;
    if (dbAge == -1) {
      msg = i18n("No virus database was found on your system.\n"
                 "Your system is currently unprotected.\n\n"
                 "Would you like to update the virus database now?");
    } else {
      msg = i18n("Your virus database is %1 days old (which is older than 7 days).\n"
                 "It is highly recommended to update it to ensure detection of the latest threats.\n\n"
                 "Would you like to update the virus database now?").arg(dbAge);
    }
    int res = KMessageBox::warningYesNo(this,
        msg,
        i18n("Outdated Virus Database"),
        KGuiItem(i18n("Update Now"), "reload"),
        KStdGuiItem::cancel());
    if (res == KMessageBox::Yes) {
      slotUpdateDatabase();
    }
  }
}

#include "mainwindow.moc"
