/*
 * statistics_view.h - Statistics View
 */

#ifndef CLAM_STATISTICS_VIEW_H
#define CLAM_STATISTICS_VIEW_H

#include <tqcombobox.h>
#include "../core/statistics.h"

class TQLabel;
class TQPushButton;
class TQWidgetStack;
class LogManager;
class TQtStackedBarChartWidget;
class TQFrame;

class StatisticsView : public TQWidget
{
    TQ_OBJECT

public:
    StatisticsView(TQWidget *parent, TQWidgetStack *stack);
    ~StatisticsView();
    void reloadIcons();

private slots:
    void refreshStats();
    void timeFrameChanged(int id);
    void quickScanClicked();
    void viewLogsClicked();

private:
    Statistics *m_stats;
    LogManager *m_logManager;
    
    TQComboBox *m_timeFrameCombo;
    TQLabel *m_lblSystemProtected;
    TQLabel *m_lblSystemProtectedIcon;
    TQLabel *m_lblSystemProtectedSub;
    TQLabel *m_lblLastScan;
    
    TQLabel *m_lblTotalScans;
    TQLabel *m_lblFilesScanned;
    TQLabel *m_lblThreatsFound;
    TQLabel *m_lblCleanScans;
    TQLabel *m_lblAvgDuration;
    
    TQLabel *m_clockIcon;
    TQLabel *m_icnQuickScan;
    TQLabel *m_icnViewLogs;
    TQPushButton *m_btnQuickScan;
    TQPushButton *m_btnViewLogs;
    
    TQLabel *m_statIcon1;
    TQLabel *m_statIcon2;
    TQLabel *m_statIcon3;
    TQLabel *m_statIcon4;
    TQLabel *m_statIcon5;
    
    TQtStackedBarChartWidget *m_chartWidget;
    
    TQFrame *m_protFrame;
    TQFrame *m_statFrame;
    TQFrame *m_chartFrame;
    TQFrame *m_actionsFrame;

    void setupUI();
    void applyTheme();
    TQWidget* createRow(TQWidget *parent, const TQPixmap &iconPixmap, const TQString &title, const TQString &subtitle, TQLabel **valueLabel, TQLabel **iconLabelOut = 0);
    
    TQPtrList<TQLabel> m_subtitleLabels;

protected:
    void showEvent(TQShowEvent *e);
    
public slots:
    void onSettingsChanged();
    void setScanRunningState(bool running);
};

#endif /* CLAM_STATISTICS_VIEW_H */
