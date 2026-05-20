/*
 * update_view.h - Virus Database Update View
 */

#ifndef CLAM_UPDATE_VIEW_H
#define CLAM_UPDATE_VIEW_H

#include <tqwidget.h>
#include <tqthread.h>
#include <ntqvaluelist.h>
#include "../core/clamav_detection.h"

class TQLabel;
class TQPushButton;
class TQProgressBar;
class TQTextEdit;
class TQTimer;
class Updater;

class TQWidgetStack;

/**
 * Background thread that collects all database status info
 * (sigtool, freshclam -V, systemctl) without blocking the UI.
 */
class DbInfoThread : public TQThread {
public:
    DbInfoThread() : m_ageDays(-1), hasUnofficial(false),
                     ssFileCount(0), ssTotalSigs(0), ssTotalSizeKB(0),
                     otherFileCount(0), otherTotalSigs(0), otherTotalSizeKB(0) {}
    
    // Results (read after finished())
    TQString dbDate;
    int m_ageDays;
    TQValueList<ClamAVDetection::DbFileInfo> dbInfos;
    TQString freshclamVersion;
    TQString serviceStatus;
    bool hasUnofficial;
    int ssFileCount;
    int ssTotalSigs;
    int ssTotalSizeKB;
    TQString ssLastModified;
    int urlhausFileCount;
    int urlhausTotalSigs;
    int urlhausTotalSizeKB;
    TQString urlhausLastModified;
    int otherFileCount;
    int otherTotalSigs;
    int otherTotalSizeKB;
    TQString otherLastModified;
    
protected:
    virtual void run();
};

class UpdateView : public TQWidget
{
    TQ_OBJECT

public:
    UpdateView(TQWidget *parent, TQWidgetStack *stack);
    ~UpdateView();
    
    void setScanRunningState(bool running);
    void reloadIcons();

protected:
    void showEvent(TQShowEvent *event);

public slots:
    void startUpdate();
    void onSettingsChanged();

private slots:
    void forceUpdate();
    void cancelUpdate();
    void onUpdateProgress(const TQString &message);
    void onUpdateFinished(int status);
    void onDbInfoReady();

signals:
    void updateFinishedEvent(int status);
    void updateStateChanged(bool running);

private:
    Updater *m_updater;
    
    TQLabel *m_statusLabel;
    TQPushButton *m_btnUpdate;
    TQPushButton *m_btnForceUpdate;
    TQPushButton *m_btnCancel;
    TQProgressBar *m_progressBar;
    TQTextEdit *m_logOutput;
    
    TQLabel *m_lblFreshclamStatus;
    TQLabel *m_lblServiceStatus;
    
    void setupUI();
    void displayCachedStatus();
    void fetchDbInfoAsync();
    
    TQLabel *m_dbWarningIcon;
    TQLabel *m_lblDbDetails;
    
    // Async db info
    DbInfoThread *m_dbInfoThread;
    TQTimer *m_dbInfoTimer;
    
    // Cached results
    TQString m_cachedDbDate;
    int m_cachedAgeDays;
    TQValueList<ClamAVDetection::DbFileInfo> m_cachedDbInfos;
    TQString m_cachedFreshclamVer;
    TQString m_cachedServiceStatus;
    bool m_cacheValid;
    bool m_cachedHasUnofficial;
    int m_cachedSsFileCount;
    int m_cachedSsTotalSigs;
    int m_cachedSsTotalSizeKB;
    TQString m_cachedSsLastModified;
    int m_cachedUrlhausFileCount;
    int m_cachedUrlhausTotalSigs;
    int m_cachedUrlhausTotalSizeKB;
    TQString m_cachedUrlhausLastModified;
    int m_cachedOtherFileCount;
    int m_cachedOtherTotalSigs;
    int m_cachedOtherTotalSizeKB;
    TQString m_cachedOtherLastModified;
};

#endif /* CLAM_UPDATE_VIEW_H */
