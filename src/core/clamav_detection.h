#ifndef CLAM_DETECTION_H
#define CLAM_DETECTION_H

#include <ntqstring.h>
#include <ntqdatetime.h>
#include <ntqvaluelist.h>

class ClamAVDetection
{
public:
    enum DaemonStatus { DaemonNotInstalled = 0, DaemonStopped, DaemonRunning };

    static bool isClamscanInstalled();
    static bool isFreshclamInstalled();
    static bool isClamdscanInstalled();
    static bool isClamdInstalled();
    static bool isDaemonRunning();
    
    static DaemonStatus getDaemonStatus();
    
    static TQString getClamscanVersion();
    static TQString getFreshclamVersion();
    static TQString getDatabaseDate();
    
    /** Detailed info about a single database file (daily.cvd, main.cvd, bytecode.cvd) */
    struct DbFileInfo {
        TQString name;       // e.g. "daily", "main", "bytecode"
        TQString buildDate;  // e.g. "16 May 2026 06:25"
        int version;        // e.g. 28002
        int signatures;     // e.g. 355454
        int sizeMB;         // file size in KB (0 if not found)
    };
    
    /** Get detailed info for all database files */
    static TQValueList<DbFileInfo> getDatabaseDetails();
    
    /** Get number of days since the daily database was last updated */
    static int getDatabaseAgeDays();

    static TQString getFreshclamServiceStatus();

    static bool getUnofficialDetails(int &ssCount, int &ssSigs, int &ssSizeKB, TQString &ssLastModified,
                                     int &urlhausCount, int &urlhausSigs, int &urlhausSizeKB, TQString &urlhausLastModified,
                                     int &otherCount, int &otherSigs, int &otherSizeKB, TQString &otherLastModified);
};

#endif /* CLAM_DETECTION_H */
