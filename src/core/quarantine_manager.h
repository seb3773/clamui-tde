/*
 * quarantine_manager.h - SQLite3 Quarantine Manager
 */

#ifndef CLAM_QUARANTINE_MANAGER_H
#define CLAM_QUARANTINE_MANAGER_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqdatetime.h>
#include <ntqvaluelist.h>
#include <sqlite3.h>

struct QuarantineEntry {
    int id;
    TQString originalPath;
    TQString threatName;
    TQDateTime detectionDate;
    TQ_LONG fileSize;
    TQString sha256;
};

class QuarantineManager : public TQObject
{
    TQ_OBJECT

public:
    QuarantineManager(TQObject *parent = 0);
    ~QuarantineManager();

    bool quarantineFile(const TQString &filePath, const TQString &threatName);
    bool restoreFile(int id);
    bool deleteFile(int id);
    
    TQValueList<QuarantineEntry> getEntries() const;
    void clearOldEntries(int daysOld = 30);

signals:
    void quarantineUpdated();
    void error(const TQString &message);

private:
    sqlite3 *m_db;
    TQString m_dbPath;
    TQString m_storageDir;
    
    bool initDatabase();
    TQString calculateHash(const TQString &filePath) const;
    TQString secureMove(const TQString &source, const TQString &hash) const;
    bool copyFile(const TQString &src, const TQString &dest) const;
};

#endif /* CLAM_QUARANTINE_MANAGER_H */
