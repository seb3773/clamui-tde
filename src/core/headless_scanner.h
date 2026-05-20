/*
 * headless_scanner.h - CLI/headless scanner implementation for scheduled scans
 */

#ifndef CLAM_HEADLESS_SCANNER_H
#define CLAM_HEADLESS_SCANNER_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <tqdatetime.h>
#include "scanner.h"
#include "profiles.h"
#include "settings_manager.h"
#include "log_manager.h"

class HeadlessScanner : public TQObject
{
    TQ_OBJECT

public:
    HeadlessScanner(const TQString &profileId, TQObject *parent = 0);
    ~HeadlessScanner();

    void start();

private slots:
    void onThreatFound(const TQString &filePath, const TQString &threatName);
    void onScanFinished(int status, int infectedCount);
    void onScanLog(const TQString &line);
    void onScanError(const TQString &msg);

private:
    TQString m_profileId;
    Scanner *m_scanner;
    ProfileManager *m_profileManager;
    SettingsManager *m_settingsManager;
    LogManager *m_logManager;
    
    TQString m_fullLog;
    int m_threatCount;
    TQTime m_startTime;
};

#endif // CLAM_HEADLESS_SCANNER_H
