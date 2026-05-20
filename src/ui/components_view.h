#ifndef CLAM_COMPONENTS_VIEW_H
#define CLAM_COMPONENTS_VIEW_H

#include <tqwidget.h>

class TQLabel;
class TQPushButton;
class TQWidgetStack;
class TQVBoxLayout;
class TQFrame;

class ComponentRow : public TQWidget
{
    TQ_OBJECT

public:
    ComponentRow(const TQString &title, const TQPixmap &iconPixmap, TQWidget *parent);
    
    void setStatus(bool installed, const TQString &versionStr = TQString::null,
                 int daemonStatus = -1);
    void applyTheme(bool isDark);
    void setInstructions(const TQString &ubuntuCmd, const TQString &fedoraCmd, const TQString &archCmd, const TQString &notes);
    void reloadIcon(const TQPixmap &iconPixmap);

protected:
    void mousePressEvent(TQMouseEvent *e);

private slots:
    void toggleExpanded();

private:
    TQLabel *m_mainIcon;
    TQLabel *m_titleLabel;
    TQLabel *m_subtitleLabel;
    TQLabel *m_statusIcon;
    TQLabel *m_statusLabel;
    TQLabel *m_expandIcon;
    
    TQWidget *m_bodyWidget;
    TQVBoxLayout *m_bodyLayout;
    bool m_isExpanded;
    bool m_hasInstructions;
};

class ComponentsView : public TQWidget
{
    TQ_OBJECT

public:
    ComponentsView(TQWidget *parent, TQWidgetStack *stack);
    ~ComponentsView();

protected:
    void showEvent(TQShowEvent *event) override;

private slots:
    void refreshStatus();

private:
    void setupUI();
    void applyTheme();
    
    TQFrame *m_listFrame;
    
    ComponentRow *m_rowClamscan;
    ComponentRow *m_rowFreshclam;
    ComponentRow *m_rowClamdscan;
    ComponentRow *m_rowClamd;
    
    TQPushButton *m_refreshBtn;
    bool m_firstShow;
    
public slots:
    void onSettingsChanged();
    void reloadIcons();
};

#endif /* CLAM_COMPONENTS_VIEW_H */
