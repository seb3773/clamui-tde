/*
 * log_manager.h - Log manager for scan results and system audits
 *
 * Architecture: metadata kept in memory (small), full details stored
 * in individual files under logs/ and loaded on demand.
 */

#ifndef CLAM_LOG_MANAGER_H
#define CLAM_LOG_MANAGER_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqdatetime.h>
#include <ntqvaluelist.h>

struct LogEntry {
    TQString id;
    TQDateTime timestamp;
    int type; // 0=Scan, 1=Update, 2=Audit
    int status;
    TQString summary; // Short summary for list display (kept in memory)
    double duration; // Operation duration in seconds
    int filesScanned; // Extracted at log creation time
    int threatsFound; // Extracted at log creation time
    
    // NOTE: full details are NOT loaded into memory.
    // Use LogManager::getLogDetails(id) to read them on demand.
};

class LogManager : public TQObject
{
    TQ_OBJECT

public:
    LogManager(TQObject *parent = 0);
    ~LogManager();

    void addLog(int type, int status, const TQString &details, double duration = 0.0);
    TQValueList<LogEntry> getLogs(int type = -1) const;
    
    /** Load full details for a specific log entry (lazy / on-demand) */
    TQString getLogDetails(const TQString &id) const;
    
    void clearLogs();
    
    // Writes to debug.log if enabled in settings
    void writeDebug(const TQString &message);

signals:
    void logsUpdated();

private:
    TQString m_logDir;
    
    void initStorage();
    void loadIndex();
    void saveIndex();
    
    /** Build a short summary string from the full details */
    static TQString buildSummary(int type, int status, const TQString &details);
    
    TQValueList<LogEntry> m_logs;
};

#endif /* CLAM_LOG_MANAGER_H */
