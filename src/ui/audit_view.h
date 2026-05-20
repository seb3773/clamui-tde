/*
 * audit_view.h - System Audit View
 */

#ifndef CLAM_AUDIT_VIEW_H
#define CLAM_AUDIT_VIEW_H

#include <tqwidget.h>

#include <tqvaluelist.h>

class TQPushButton;
class TQScrollView;
class TQWidget;
class TQVBoxLayout;
class TQWidgetStack;
class TQFrame;
class TQLabel;
#include "../core/system_audit.h"

class AuditView : public TQWidget
{
    TQ_OBJECT

public:
    AuditView(TQWidget *parent, TQWidgetStack *stack);
    ~AuditView();

public slots:
    void applyTheme(bool isDark);
    void onSettingsChanged();
    void reloadIcons();
    void setScanRunningState(bool running);

private slots:
    void runAudit();
    void runLynis();
    void runRootkit();
    void copyToClipboard();

private:
    SystemAudit *m_audit;
    
    TQWidget *m_headerWidget;
    TQLabel *m_lblTitle;
    TQLabel *m_lblDesc;
    TQPushButton *m_btnRefresh;
    TQPushButton *m_btnInitialRun;
    
    TQScrollView *m_scrollView;
    TQWidget *m_scrollContent;
    TQVBoxLayout *m_scrollLayout;
    
    TQValueList<TQFrame*> m_cardFrames;
    TQValueList<TQLabel*> m_subtitleLabels;
    TQValueList<TQLabel*> m_titleLabels;
    
    void setupUI();
    void clearUI();
    void buildReportUI(const AuditReport &report);
    TQFrame* createCardFrame(const TQString &title, const TQPixmap &iconPixmap, const TQString &category);
    void addCheckRow(TQFrame *card, const AuditCheckResult &check);
};

#endif /* CLAM_AUDIT_VIEW_H */
