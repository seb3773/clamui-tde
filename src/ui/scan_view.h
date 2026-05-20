/*
 * scan_view.h - Virus Scan View
 */

#ifndef CLAM_SCAN_VIEW_H
#define CLAM_SCAN_VIEW_H

#include <tqwidget.h>
#include <ntqstringlist.h>
#include <ntqdatetime.h>

class TQLabel;
class TQPushButton;
class TQProgressBar;
class TQListView;
class TQListBox;
class TQComboBox;
class Scanner;
class TQWidgetStack;

class ScanView : public TQWidget
{
    TQ_OBJECT

public:
    ScanView(TQWidget *parent, TQWidgetStack *stack);
    ~ScanView();
    void triggerQuickScan();
    void reloadIcons();

public slots:
    void addTarget();
    void addFileTarget();
    void addDirectoryTarget();
    void cancelScan();
    void startScan();
    void onProfileSelected(int index);
    void onConfigureScheduleClicked();
    void updateScheduledScanBanner();
    void saveAsProfile();
    void reloadProfiles();
    void updateSaveProfileButtonState();

private slots:
    void onScanProgress(int filesScanned, const TQString &currentFile);
    void onScanFinished(int status, int infectedCount);
    void onThreatFound(const TQString &filePath, const TQString &threatName);
    void onScanError(const TQString &errorMessage);
    void onScanLog(const TQString &logLine);

    void removeTarget();
    void runEicarTest();
    void onCounterTimeout();

private:
    Scanner *m_scanner;
    TQStringList m_targetPaths;
    TQString m_fullLog;
    TQTime m_scanStartTime;
    TQString m_lastEtaStr;
    int m_lastEtaUpdateMs;
    
    TQComboBox *m_profileCombo;
    TQListBox *m_targetList;
    
    TQPushButton *m_btnAddTarget;
    TQPushButton *m_btnRemoveTarget;
    
    TQPushButton *m_btnStart;
    TQPushButton *m_btnEicar;
    TQPushButton *m_btnCancel;
    TQProgressBar *m_progressBar;
    TQLabel *m_statusLabel;
    TQLabel *m_lblBackend;
    TQListView *m_threatList;
    
    void setupUI();
    void setUIState(bool running);

signals:
    void scanStateChanged(bool running);
    void scanFinishedEvent(int status, int infectedCount);
    void scanProgressUpdated(int percent);
    void showLogsRequested();

public:
    bool isScanRunning() const { return m_isRunning; }

protected:
    bool eventFilter(TQObject *obj, TQEvent *event);
    void showEvent(TQShowEvent *event);
    
    bool m_isRunning;
    int m_totalFiles;
    int m_pulseStep;
    class TQPopupMenu *m_addMenu;
    class FileCounterThread *m_counterThread;
    class TQTimer *m_counterTimer;
    
    TQWidget *m_scheduleBanner;
    TQLabel *m_lblScheduleIcon;
    TQLabel *m_lblScheduleText;
    TQPushButton *m_btnConfigureSchedule;
    TQPushButton *m_btnSaveProfile;
    TQString m_newProfileToSelect;
};

#endif /* CLAM_SCAN_VIEW_H */
