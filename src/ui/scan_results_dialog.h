/*
 * scan_results_dialog.h - Post-scan summary dialog
 */

#ifndef CLAM_SCAN_RESULTS_DIALOG_H
#define CLAM_SCAN_RESULTS_DIALOG_H

#include <tqdialog.h>
#include <ntqstring.h>

#include <ntqstringlist.h>

class TQLabel;
class TQListView;
class TQListViewItem;
class TQPushButton;
class QuarantineManager;

class ScanResultsDialog : public TQDialog
{
    TQ_OBJECT

public:
    ScanResultsDialog(TQWidget *parent, const TQStringList &targetPaths, int filesScanned, int durationSecs, TQListView *threatList, QuarantineManager *quarantineManager);
    ~ScanResultsDialog();

private slots:
    void quarantineAll();
    void quarantineSelected();
    void itemSelectionChanged();
    void slotGoToLogs();

private:
    TQListView *m_sourceThreatList;
    TQListView *m_resultsList;
    QuarantineManager *m_quarantineManager;
    TQPushButton *m_btnQuarantineAll;
    TQPushButton *m_btnQuarantineSelected;
    
    void updateButtons();
};

#endif /* CLAM_SCAN_RESULTS_DIALOG_H */
