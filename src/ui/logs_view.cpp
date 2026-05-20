/*
 * logs_view.cpp - Scan and Update Logs View
 */

#include "logs_view.h"
#include "../core/log_manager.h"

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqlistview.h>
#include <tqcombobox.h>
#include <tqtabwidget.h>
#include <tqtextedit.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tqdatetime.h>
#include <stdlib.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <malloc.h>

#include <tqtimer.h>
#include <tdefiledialog.h>
#include <tdemessagebox.h>
#include <kiconloader.h>
#include <tqtooltip.h>
#include <kstandarddirs.h>
#include "icon_utils.h"

LogsView::LogsView(TQWidget *parent, TQWidgetStack *stack, LogManager *logManager)
    : TQWidget(parent), m_logManager(logManager), m_daemonLive(false)
{
    if (!m_logManager) {
        m_logManager = new LogManager(this); // Fallback
    }
    connect(m_logManager, TQT_SIGNAL(logsUpdated()), this, TQT_SLOT(refreshLogs()));
    
    m_daemonTimer = new TQTimer(this);
    connect(m_daemonTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(pollDaemonLogs()));
    
    setupUI();
    refreshLogs();
    pollDaemonLogs();
}

LogsView::~LogsView()
{
}

void LogsView::hideEvent(TQHideEvent *event)
{
    // Deselect log entry for UI consistency
    m_logList->clearSelection();
    
    // Free memory held by the detail pane
    m_logDetails->clear();
    m_logDetails->setText(i18n("Select a log entry to view details."));
    
    // Force glibc to return freed pages to the OS
    malloc_trim(0);
    
    TQWidget::hideEvent(event);
}

void LogsView::setupUI()
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 11, 6);
    
    TQLabel *titleLabel = new TQLabel(i18n("<h2>Event Logs</h2>"), this);
    mainLayout->addWidget(titleLabel);
    
    TQLabel *descLabel = new TQLabel(i18n("View virus scan logs, database updates history, and live ClamAV daemon events."), this);
    descLabel->setAlignment(TQt::WordBreak | TQt::AlignLeft);
    mainLayout->addWidget(descLabel);
    
    TQFrame *line = new TQFrame(this);
    line->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    mainLayout->addWidget(line);
    
    mainLayout->addSpacing(10);
    
    m_tabWidget = new TQTabWidget(this);
    mainLayout->addWidget(m_tabWidget, 1);
    
    // -----------------------------------------
    // Tab 1: Historical Logs
    // -----------------------------------------
    TQWidget *historyTab = new TQWidget(m_tabWidget);
    TQVBoxLayout *historyLayout = new TQVBoxLayout(historyTab, 10, 6);
    
    TQHBoxLayout *filterLayout = new TQHBoxLayout(historyLayout, 6);
    filterLayout->addWidget(new TQLabel(i18n("Filter:"), historyTab));
    
    m_filterCombo = new TQComboBox(historyTab);
    m_filterCombo->insertItem(i18n("All Events"));
    m_filterCombo->insertItem(i18n("Scans"));
    m_filterCombo->insertItem(i18n("Updates"));
    m_filterCombo->insertItem(i18n("System Audits"));
    connect(m_filterCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(filterChanged(int)));
    filterLayout->addWidget(m_filterCombo);
    
    filterLayout->addStretch(1);
    
    m_btnExportCSV = new TQPushButton(IconUtils::load(save_png, save_png_len), i18n(".csv"), historyTab);
    TQToolTip::add(m_btnExportCSV, i18n("Export to CSV"));
    connect(m_btnExportCSV, TQT_SIGNAL(clicked()), this, TQT_SLOT(exportCSV()));
    filterLayout->addWidget(m_btnExportCSV);
    
    m_btnExportJSON = new TQPushButton(IconUtils::load(save_png, save_png_len), i18n(".json"), historyTab);
    TQToolTip::add(m_btnExportJSON, i18n("Export to JSON"));
    connect(m_btnExportJSON, TQT_SIGNAL(clicked()), this, TQT_SLOT(exportJSON()));
    filterLayout->addWidget(m_btnExportJSON);
    
    m_btnClear = new TQPushButton(IconUtils::load(trash_png, trash_png_len), i18n("Clear All"), historyTab);
    connect(m_btnClear, TQT_SIGNAL(clicked()), this, TQT_SLOT(clearLogs()));
    filterLayout->addWidget(m_btnClear);
    
    m_logList = new TQListView(historyTab);
    m_logList->addColumn(i18n("Time"));
    m_logList->addColumn(i18n("Type"));
    m_logList->addColumn(i18n("Status"));
    m_logList->setAllColumnsShowFocus(true);
    m_logList->setSorting(-1); // Sort by order inserted
    connect(m_logList, TQT_SIGNAL(selectionChanged(TQListViewItem*)), this, TQT_SLOT(logSelectionChanged(TQListViewItem*)));
    historyLayout->addWidget(m_logList, 1);
    
    TQLabel *detailsLabel = new TQLabel(i18n("<b>Log Details</b><br><small>Select a log entry above to view details</small>"), historyTab);
    historyLayout->addWidget(detailsLabel);
    
    m_logDetails = new TQTextEdit(historyTab);
    m_logDetails->setReadOnly(true);
    m_logDetails->setTextFormat(TQt::PlainText);
    m_logDetails->setUndoRedoEnabled(false);
    m_logDetails->setText(i18n("Select a log entry to view details."));
    TQFont font("Monospace");
    font.setStyleHint(TQFont::TypeWriter);
    font.setPointSize(font.pointSize() - 1);
    m_logDetails->setFont(font);
    historyLayout->addWidget(m_logDetails, 1);
    
    m_tabWidget->addTab(historyTab, IconUtils::load(recent_png, recent_png_len), i18n("Historical Logs"));
    
    // -----------------------------------------
    // Tab 2: ClamAV Daemon
    // -----------------------------------------
    TQWidget *daemonTab = new TQWidget(m_tabWidget);
    TQVBoxLayout *daemonLayout = new TQVBoxLayout(daemonTab, 10, 6);
    
    TQHBoxLayout *daemonStatusLayout = new TQHBoxLayout(daemonLayout, 6);
    
    TQVBoxLayout *daemonTextLayout = new TQVBoxLayout(daemonStatusLayout, 2);
    daemonTextLayout->addWidget(new TQLabel(i18n("<b>Daemon Status</b>"), daemonTab));
    m_lblDaemonStatus = new TQLabel(i18n("Checking..."), daemonTab);
    m_lblDaemonStatus->setPaletteForegroundColor(TQt::darkGray);
    daemonTextLayout->addWidget(m_lblDaemonStatus);
    
    daemonStatusLayout->addStretch(1);
    
    m_btnPlayStop = new TQPushButton(SmallIcon("player_play"), TQString::null, daemonTab);
    connect(m_btnPlayStop, TQT_SIGNAL(clicked()), this, TQT_SLOT(toggleDaemonLogs()));
    daemonStatusLayout->addWidget(m_btnPlayStop);
    
    TQLabel *daemonInfo = new TQLabel(i18n("Daemon logs will appear here.<br>Click the play button to start live updates."), daemonTab);
    daemonLayout->addWidget(daemonInfo);
    
    m_daemonTextEdit = new TQTextEdit(daemonTab);
    m_daemonTextEdit->setReadOnly(true);
    m_daemonTextEdit->setTextFormat(TQt::PlainText);
    m_daemonTextEdit->setFont(font);
    daemonLayout->addWidget(m_daemonTextEdit, 1);
    
    m_tabWidget->addTab(daemonTab, IconUtils::load(view_png, view_png_len), i18n("ClamAV Daemon"));
}

void LogsView::reloadIcons()
{
    if (m_btnExportCSV) m_btnExportCSV->setIconSet(TQIconSet(IconUtils::load(save_png, save_png_len)));
    if (m_btnExportJSON) m_btnExportJSON->setIconSet(TQIconSet(IconUtils::load(save_png, save_png_len)));
    if (m_btnClear) m_btnClear->setIconSet(TQIconSet(IconUtils::load(trash_png, trash_png_len)));
    
    if (m_tabWidget) {
        m_tabWidget->setTabIconSet(m_tabWidget->page(0), TQIconSet(IconUtils::load(recent_png, recent_png_len)));
        m_tabWidget->setTabIconSet(m_tabWidget->page(1), TQIconSet(IconUtils::load(view_png, view_png_len)));
    }
}

class LogListViewItem : public TQListViewItem {
public:
    LogListViewItem(TQListView *parent, const LogEntry &entry)
        : TQListViewItem(parent), m_entry(entry) {
        
        TQString typeStr;
        switch (entry.type) {
            case 0: typeStr = i18n("Scan"); break;
            case 1: typeStr = i18n("Update"); break;
            case 2: typeStr = i18n("Audit"); break;
            default: typeStr = i18n("System"); break;
        }
        
        TQString statusStr;
        if (entry.type == 0) { // Scan
            if (entry.status == 0) statusStr = i18n("clean");
            else if (entry.status == 1) statusStr = i18n("infected");
            else {
                if (entry.summary.contains("cancelled", false)) statusStr = i18n("cancelled");
                else statusStr = i18n("error");
            }
        } else if (entry.type == 1) { // Update
            statusStr = entry.status == 0 ? i18n("success") : i18n("error");
        } else if (entry.type == 2) { // Audit
            if (entry.status == 0) statusStr = i18n("secure");
            else if (entry.status == 1) statusStr = i18n("warning");
            else statusStr = i18n("fail");
        } else {
            statusStr = entry.status == 0 ? i18n("ok") : i18n("error");
        }
        
        setText(0, entry.timestamp.toString());
        setText(1, typeStr);
        setText(2, statusStr);
    }
    
    TQString key(int column, bool ascending) const {
        if (column == 0) {
            return m_entry.timestamp.toString(TQt::ISODate);
        }
        return TQListViewItem::key(column, ascending);
    }
    
    const LogEntry& entry() const { return m_entry; }
private:
    LogEntry m_entry;
};

void LogsView::refreshLogs()
{
    m_logList->clear();
    m_logDetails->setText(i18n("Select a log entry to view details."));
    
    int filterType = -1;
    int idx = m_filterCombo->currentItem();
    if (idx > 0) {
        filterType = idx - 1; // 0=Scans, 1=Updates, 2=Audits
    }
    
    TQValueList<LogEntry> logs = m_logManager->getLogs(filterType);
    for (TQValueList<LogEntry>::ConstIterator it = logs.begin(); it != logs.end(); ++it) {
        new LogListViewItem(m_logList, *it);
    }
}

void LogsView::selectLatestLog()
{
    m_filterCombo->setCurrentItem(0);
    m_logList->setSorting(0, false); // Sort by Time (column 0) descending
    refreshLogs();
    TQListViewItem *item = m_logList->firstChild();
    if (item) {
        m_logList->setSelected(item, true);
        m_logList->setCurrentItem(item);
        logSelectionChanged(item);
    }
    m_tabWidget->setCurrentPage(0);
}

void LogsView::logSelectionChanged(TQListViewItem *item)
{
    if (!item) {
        m_logDetails->setText(i18n("Select a log entry to view details."));
        return;
    }
    
    LogListViewItem *logItem = static_cast<LogListViewItem*>(item);
    const LogEntry &entry = logItem->entry();
    
    // Clear previous content first to free memory (undo history etc.)
    m_logDetails->clear();
    
    // Load full details on demand
    TQString details = m_logManager->getLogDetails(entry.id);
    
    TQString fullText;
    if (entry.type == 0) fullText += i18n("SCAN LOG\n==================================================\n\n");
    else if (entry.type == 1) fullText += i18n("UPDATE LOG\n==================================================\n\n");
    else fullText += i18n("SYSTEM LOG\n==================================================\n\n");
    
    if (!entry.id.isEmpty()) fullText += i18n("ID: %1\n").arg(entry.id);
    fullText += i18n("Timestamp: %1\n").arg(entry.timestamp.toString(TQt::ISODate));
    fullText += i18n("Type: %1\n").arg(logItem->text(1).lower());
    fullText += i18n("Status: %1\n").arg(logItem->text(2).lower());
    
    // Duration
    TQString durationStr;
    if (entry.duration > 0) {
        durationStr = TQString::number(entry.duration, 'f', 2) + " seconds";
    } else {
        durationStr = "0.00 seconds";
        if (entry.type == 0 && !details.isNull()) {
            int dIdx = details.find("Time: ");
            if (dIdx != -1) {
                int dEnd = details.find('\n', dIdx);
                durationStr = details.mid(dIdx + 6, dEnd - (dIdx + 6)).stripWhiteSpace();
            }
        }
    }
    
    fullText += i18n("Duration: %1\n\n").arg(durationStr);
    
    if (entry.type == 0 && !details.isNull()) {
        fullText += i18n("Statistics Summary:\n--------------------------------------------------\n");
        TQString filesScanned = "0";
        int fIdx = details.find("Scanned files: ");
        if (fIdx != -1) {
            int fEnd = details.find('\n', fIdx);
            filesScanned = details.mid(fIdx + 15, fEnd - (fIdx + 15)).stripWhiteSpace();
        }
        fullText += i18n("  Files Scanned: %1\n").arg(filesScanned);
        fullText += i18n("  Duration: %1\n\n").arg(durationStr);
    }
    
    fullText += i18n("Summary:\n  %1\n\n").arg(entry.summary);
    fullText += i18n("--------------------------------------------------\nFull Output:\n--------------------------------------------------\n");
    fullText += details.isNull() ? i18n("(detail file not found)") : details;
    
    m_logDetails->setText(fullText);
}

void LogsView::exportCSV()
{
    TQString file = KFileDialog::getSaveFileName(TQString::null, "*.csv", this, i18n("Export Logs to CSV"));
    if (file.isEmpty()) return;
    
    TQFile f(file);
    if (f.open(IO_WriteOnly)) {
        TQTextStream stream(&f);
        stream << "Time,Type,Status,Details\n";
        
        TQListViewItemIterator it(m_logList);
        while (it.current()) {
            LogListViewItem *item = static_cast<LogListViewItem*>(it.current());
            TQString details = m_logManager->getLogDetails(item->entry().id);
            if (details.isNull()) details = "";
            details.replace("\"", "\"\""); // Escape quotes
            stream << "\"" << item->text(0) << "\",\"" << item->text(1) << "\",\"" 
                   << item->text(2) << "\",\"" << details << "\"\n";
            ++it;
        }
        f.close();
        KMessageBox::information(this, i18n("Logs exported successfully."));
    } else {
        KMessageBox::error(this, i18n("Failed to open file for writing."));
    }
}

void LogsView::exportJSON()
{
    TQString file = KFileDialog::getSaveFileName(TQString::null, "*.json", this, i18n("Export Logs to JSON"));
    if (file.isEmpty()) return;
    
    TQString logDir = TDEGlobal::dirs()->saveLocation("appdata", "logs");
    TQString sourceFile = logDir + "index.json";
    
    if (TQFile::exists(sourceFile)) {
        if (TQFile::exists(file)) {
            TQFile::remove(file);
        }
        
        TQFile s(sourceFile);
        TQFile d(file);
        if (s.open(IO_ReadOnly) && d.open(IO_WriteOnly)) {
            TQTextStream is(&s);
            TQTextStream os(&d);
            os << is.read();
            s.close();
            d.close();
            KMessageBox::information(this, i18n("Logs exported successfully."));
            return;
        }
    }
    
    KMessageBox::error(this, i18n("Failed to export JSON file."));
}

void LogsView::clearLogs()
{
    if (KMessageBox::warningContinueCancel(this, 
        i18n("Are you sure you want to clear all event logs? This cannot be undone."),
        i18n("Clear Logs"),
        KStdGuiItem::clear()) == KMessageBox::Continue) 
    {
        m_logManager->clearLogs();
        m_logDetails->setText(i18n("Select a log entry to view details."));
    }
}

void LogsView::filterChanged(int /*index*/)
{
    refreshLogs();
}

void LogsView::toggleDaemonLogs()
{
    m_daemonLive = !m_daemonLive;
    if (m_daemonLive) {
        m_btnPlayStop->setIconSet(SmallIcon("player_pause"));
        m_daemonTimer->start(2000); // Poll every 2 seconds
        pollDaemonLogs();
    } else {
        m_btnPlayStop->setIconSet(SmallIcon("player_play"));
        m_daemonTimer->stop();
    }
}

void LogsView::pollDaemonLogs()
{
    bool isDaemonInstalled = (system("systemctl list-unit-files clamav-daemon.service >/dev/null 2>&1") == 0) ||
                             (system("command -v clamd >/dev/null 2>&1") == 0);
                             
    if (!isDaemonInstalled) {
        m_lblDaemonStatus->setText(i18n("Not installed"));
        m_lblDaemonStatus->setPaletteForegroundColor(TQt::gray);
        m_btnPlayStop->setEnabled(false);
        m_daemonTextEdit->setText(i18n("ClamAV daemon is not installed on this system."));
        return;
    }
    
    m_btnPlayStop->setEnabled(true);

    bool isServiceActive = (system("systemctl is-active --quiet clamav-daemon.service 2>/dev/null") == 0);
    if (isServiceActive) {
        m_lblDaemonStatus->setText(i18n("Active"));
        m_lblDaemonStatus->setPaletteForegroundColor(TQt::darkGreen);
    } else {
        m_lblDaemonStatus->setText(i18n("Stopped"));
        m_lblDaemonStatus->setPaletteForegroundColor(TQt::red);
    }
    
    system("journalctl -u clamav-daemon.service -n 100 --no-pager > /tmp/clamd_logs.tmp 2>/dev/null");
    
    TQFile f("/tmp/clamd_logs.tmp");
    if (f.open(IO_ReadOnly)) {
        TQTextStream t(&f);
        TQString logs = t.read();
        f.close();
        
        if (logs.isEmpty()) {
            logs = i18n("No daemon logs found.");
        }
        
        m_daemonTextEdit->setText(logs);
        // Scroll to bottom
        m_daemonTextEdit->setCursorPosition(m_daemonTextEdit->lines() - 1, 0);
        TQFile::remove("/tmp/clamd_logs.tmp");
    }
}

#include "logs_view.moc"
