/*
 * mainwindow.h - Main application window
 *
 * KMainWindow with horizontal splitter layout:
 *   Left pane:  Sidebar navigation (icon + label list)
 *   Right pane: TQWidgetStack for view switching
 *
 * Mirrors the Adw.Leaflet sidebar pattern from the GTK4 original.
 */

#ifndef CLAM_MAINWINDOW_H
#define CLAM_MAINWINDOW_H

#include <tdemainwindow.h>

class TQSplitter;
class TQWidgetStack;
class TQLabel;
class TDEAction;
class KStatusBar;
class ClamSidebar;
class ClamTray;

/* Forward-declare all view classes */
class ScanView;
class UpdateView;
class LogsView;
class ComponentsView;
class QuarantineView;
class StatisticsView;
class AuditView;

/* Forward-declare core managers */
class SettingsManager;
class LogManager;
class QuarantineManager;
class ProfileManager;

class ClamMainWindow : public TDEMainWindow
{
    TQ_OBJECT

public:
    ClamMainWindow();
    ~ClamMainWindow();
    
    LogManager* getLogManager() const { return m_logManager; }

    void setTray(ClamTray *tray);

    /* Access to shared managers */
    SettingsManager  *settingsManager() const { return m_settings; }
    LogManager       *logManager()      const { return m_logManager; }
    QuarantineManager *quarantineManager() const { return m_quarantine; }
    ProfileManager   *profileManager()  const { return m_profiles; }

public slots:
    void switchToView(int index);
    void showPreferences(int initialPage = -1);
    void slotQuickScan();
    void slotScanFile();
    void slotScanFolder();
    void slotUpdateDatabase();
    void slotAbout();
    void slotQuitApp();

    bool isScanRunning() const;
    void startScheduledScan(const TQString &profileId, bool autoQuit = true);
    void userShowedWindow();

protected:
    bool queryClose();
    void saveProperties(TDEConfig *);
    void readProperties(TDEConfig *);
    void showEvent(TQShowEvent *e);

private slots:
    void slotSidebarClicked(int index);
    void onScanStateChanged(bool running);
    void onUpdateStateChanged(bool running);
    void onScanFinishedEvent(int status, int infectedCount);
    void onUpdateFinishedEvent(int status);
    void onScanProgress(int percent);
    void updateStatusBarStats();
    void onSettingsChanged();
    void slotShowLatestLog();

private:
    void setupManagers();
    void setupActions();
    void setupUI();
    void setupStatusBar();
    void createViews();

    /* Core managers (owned) */
    SettingsManager   *m_settings;
    LogManager        *m_logManager;
    QuarantineManager *m_quarantine;
    ProfileManager    *m_profiles;

    /* UI components */
    TQSplitter      *m_splitter;
    ClamSidebar     *m_sidebar;
    TQWidgetStack   *m_viewStack;
    ClamTray        *m_tray;

    /* Views (owned by widget stack) */
    ScanView        *m_scanView;
    UpdateView      *m_updateView;
    LogsView        *m_logsView;
    ComponentsView  *m_componentsView;
    QuarantineView  *m_quarantineView;
    StatisticsView  *m_statsView;
    AuditView       *m_auditView;

    /* Actions */
    TDEAction *m_quickScanAction;
    TDEAction *m_scanFileAction;
    TDEAction *m_scanFolderAction;
    TDEAction *m_updateDbAction;
    TDEAction       *m_prefsAction;
    TDEAction       *m_aboutAction;
    
    /* Status bar widgets */
    TQLabel *m_scanStatusLabel;

    /* Scheduled scan state */
    bool m_scheduledAutoQuit;
    bool m_userShownWindow;
};

#endif /* CLAM_MAINWINDOW_H */
