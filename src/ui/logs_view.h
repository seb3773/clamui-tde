/*
 * logs_view.h - Scan and Update Logs View
 */

#ifndef CLAM_LOGS_VIEW_H
#define CLAM_LOGS_VIEW_H

#include <tqwidget.h>

class TQListView;
class TQPushButton;
class TQComboBox;
class LogManager;
class TQWidgetStack;

class TQTextEdit;
class TQTabWidget;
class TQLabel;

class TQListViewItem;
class TQTimer;

class LogsView : public TQWidget
{
    TQ_OBJECT

public:
    LogsView(TQWidget *parent, TQWidgetStack *stack, LogManager *logManager);
    ~LogsView();
    void reloadIcons();
    void selectLatestLog();

protected:
    void hideEvent(TQHideEvent *event);

private slots:
    void refreshLogs();
    void clearLogs();
    void filterChanged(int index);
    
    // Historical Tab
    void logSelectionChanged(TQListViewItem *item);
    void exportCSV();
    void exportJSON();
    
    // Daemon Tab
    void toggleDaemonLogs();
    void pollDaemonLogs();

private:
    LogManager *m_logManager;
    
    TQTabWidget *m_tabWidget;
    
    // Historical Tab
    TQComboBox *m_filterCombo;
    TQListView *m_logList;
    TQTextEdit *m_logDetails;
    TQPushButton *m_btnClear;
    TQPushButton *m_btnExportCSV;
    TQPushButton *m_btnExportJSON;
    
    // Daemon Tab
    TQLabel *m_lblDaemonStatus;
    TQPushButton *m_btnPlayStop;
    TQTextEdit *m_daemonTextEdit;
    TQTimer *m_daemonTimer;
    bool m_daemonLive;
    
    void setupUI();
};

#endif /* CLAM_LOGS_VIEW_H */
