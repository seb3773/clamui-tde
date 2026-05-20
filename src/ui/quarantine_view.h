/*
 * quarantine_view.h - Quarantine View
 */

#ifndef CLAM_QUARANTINE_VIEW_H
#define CLAM_QUARANTINE_VIEW_H

#include <tqwidget.h>

class TQListView;
class TQPushButton;
class TQLabel;
class QuarantineManager;
class QuarantineManager;
class TQWidgetStack;

class QuarantineView : public TQWidget
{
    TQ_OBJECT

public:
    QuarantineView(TQWidget *parent, TQWidgetStack *stack, QuarantineManager *quarantineManager);
    ~QuarantineView();
    void reloadIcons();

private slots:
    void refreshList();
    void selectionChanged();
    void restoreSelected();
    void deleteSelected();
    void clearOld();

private:
    QuarantineManager *m_quarantine;
    
    TQListView *m_list;
    TQLabel *m_lblTotalSize;
    TQLabel *m_lblTotalItems;
    TQLabel *m_sizeIcon;
    
    TQPushButton *m_btnRestore;
    TQPushButton *m_btnDelete;
    TQPushButton *m_btnClearOld;
    
    void setupUI();
};

#endif /* CLAM_QUARANTINE_VIEW_H */
