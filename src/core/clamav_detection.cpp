/*
 * clamav_detection.cpp - ClamAV availability detection
 */

#include "clamav_detection.h"
#include <kstandarddirs.h>
#include <ntqdir.h>
#include <ntqfile.h>
#include <ntqtextstream.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#include <ntqmutex.h>

static TQMutex s_popenMutex;

static TQString runCommand(const TQString& cmd, int* exitCode = nullptr) {
    TQMutexLocker locker(&s_popenMutex);
    TQString output;
    TQString fullCmd = cmd;
    if (!fullCmd.contains("2>/dev/null")) {
        fullCmd += " 2>/dev/null";
    }
    FILE* pipe = popen(fullCmd.latin1(), "r");
    if (!pipe) return output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += TQString::fromLocal8Bit(buffer);
    }
    int status = pclose(pipe);
    if (exitCode) *exitCode = WEXITSTATUS(status);
    return output.stripWhiteSpace();
}

bool ClamAVDetection::isClamscanInstalled()
{
    return !TDEGlobal::dirs()->findExe("clamscan").isEmpty();
}

bool ClamAVDetection::isFreshclamInstalled()
{
    return !TDEGlobal::dirs()->findExe("freshclam").isEmpty();
}

bool ClamAVDetection::isClamdscanInstalled()
{
    return !TDEGlobal::dirs()->findExe("clamdscan").isEmpty();
}

bool ClamAVDetection::isClamdInstalled()
{
    if (!TDEGlobal::dirs()->findExe("clamd").isEmpty()) return true;
    if (TQFile("/usr/sbin/clamd").exists()) return true;
    if (TQFile("/usr/local/sbin/clamd").exists()) return true;
    return false;
}

bool ClamAVDetection::isDaemonRunning()
{
    TQString clamdscan = TDEGlobal::dirs()->findExe("clamdscan");
    if (clamdscan.isEmpty()) return false;
    
    int exitCode = -1;
    TQString output = runCommand(clamdscan + " --ping 3", &exitCode);
    return exitCode == 0 && output.contains("PONG");
}

TQString ClamAVDetection::getClamscanVersion()
{
    static TQString cachedVersion;
    if (!cachedVersion.isEmpty()) return cachedVersion;

    TQString clamscan = TDEGlobal::dirs()->findExe("clamscan");
    if (clamscan.isEmpty()) {
        cachedVersion = i18n("Not installed");
        return cachedVersion;
    }

    TQString output = runCommand(clamscan + " --version");
    cachedVersion = output.isEmpty() ? i18n("Unknown") : output;
    return cachedVersion;
}

TQString ClamAVDetection::getFreshclamVersion()
{
    static TQString cachedVersion;
    if (!cachedVersion.isEmpty()) return cachedVersion;

    TQString freshclam = TDEGlobal::dirs()->findExe("freshclam");
    if (freshclam.isEmpty()) {
        cachedVersion = i18n("Not installed");
        return cachedVersion;
    }

    TQString output = runCommand(freshclam + " --version");
    cachedVersion = output.isEmpty() ? i18n("Unknown") : output;
    return cachedVersion;
}

TQString ClamAVDetection::getDatabaseDate()
{
    TQFile dbFile("/var/lib/clamav/daily.cvd");
    if (!dbFile.exists()) {
        dbFile.setName("/var/lib/clamav/daily.cld");
    }
    
    if (dbFile.exists()) {
        if (dbFile.open(IO_ReadOnly)) {
            TQTextStream stream(&dbFile);
            TQString header = stream.readLine();
            dbFile.close();
            
            TQStringList parts = TQStringList::split(':', header);
            if (parts.count() > 2) {
                return parts[1] + " (v" + parts[2] + ")";
            }
        }
    }
    return i18n("Unknown or not downloaded");
}

ClamAVDetection::DaemonStatus ClamAVDetection::getDaemonStatus()
{
    const char* services[] = { "clamav-daemon.service", "clamd@scan.service", "clamd.service" };
    bool sawService = false;

    for (int i = 0; i < 3; ++i) {
        int exitCode = -1;
        TQString output = runCommand(TQString("systemctl is-active ") + services[i], &exitCode);
        if (!output.isEmpty() && output != "unknown") {
            if (output == "active") return DaemonRunning;
            if (output == "inactive" || output == "failed" || output == "activating" || output == "deactivating") {
                sawService = true;
            }
        }
    }

    int pgrepExit = -1;
    runCommand("pgrep -x clamd", &pgrepExit);
    if (pgrepExit == 0) {
        return DaemonRunning;
    }

    if (sawService) return DaemonStopped;

    if (isClamdInstalled()) {
        return DaemonStopped;
    }

    return DaemonNotInstalled;
}

TQValueList<ClamAVDetection::DbFileInfo> ClamAVDetection::getDatabaseDetails()
{
    TQValueList<DbFileInfo> result;
    
    const char *dbNames[] = { "daily", "main", "bytecode" };
    const char *exts[] = { ".cvd", ".cld" };
    
    TQString sigtool = TDEGlobal::dirs()->findExe("sigtool");
    
    for (int i = 0; i < 3; ++i) {
        DbFileInfo info;
        info.name = dbNames[i];
        info.version = 0;
        info.signatures = 0;
        info.sizeMB = 0;
        
        // Find the database file
        TQString dbPath;
        for (int e = 0; e < 2; ++e) {
            TQString path = TQString("/var/lib/clamav/%1%2").arg(dbNames[i]).arg(exts[e]);
            if (TQFile::exists(path)) {
                dbPath = path;
                break;
            }
        }
        
        if (dbPath.isEmpty()) continue;
        
        // Get file size
        struct stat st;
        if (::stat(dbPath.local8Bit(), &st) == 0) {
            info.sizeMB = (int)(st.st_size / 1024); // KB
        }
        
        // Use sigtool for detailed info
        if (!sigtool.isEmpty()) {
            TQString output = runCommand(sigtool + " --info " + dbPath);
            // Parse sigtool output
            TQStringList lines = TQStringList::split('\n', output);
            for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
                TQString line = (*it).stripWhiteSpace();
                if (line.startsWith("Build time:")) {
                    info.buildDate = line.mid(11).stripWhiteSpace();
                } else if (line.startsWith("Version:")) {
                    info.version = line.mid(8).stripWhiteSpace().toInt();
                } else if (line.startsWith("Signatures:")) {
                    info.signatures = line.mid(11).stripWhiteSpace().toInt();
                }
            }
        }
        
        result.append(info);
    }
    
    return result;
}

int ClamAVDetection::getDatabaseAgeDays()
{
    // Check mtime of daily database file
    const char *exts[] = { ".cvd", ".cld" };
    for (int e = 0; e < 2; ++e) {
        TQString path = TQString("/var/lib/clamav/daily%1").arg(exts[e]);
        struct stat st;
        if (::stat(path.local8Bit(), &st) == 0) {
            time_t now = time(NULL);
            int days = (int)((now - st.st_mtime) / 86400);
            return days;
        }
    }
    return -1; // Not found
}

TQString ClamAVDetection::getFreshclamServiceStatus()
{
    int exitCode = -1;
    runCommand("systemctl is-active --quiet clamav-freshclam.service 2>/dev/null", &exitCode);
    if (exitCode != 0) {
        return "Inactive";
    }
    TQString pid = runCommand("systemctl show -p MainPID --value clamav-freshclam.service 2>/dev/null");
    if (!pid.isEmpty() && pid != "0") {
        return TQString("Active (PID %1)").arg(pid);
    }
    return "Active";
}

bool ClamAVDetection::getUnofficialDetails(int &ssCount, int &ssSigs, int &ssSizeKB, TQString &ssLastModified,
                                           int &urlhausCount, int &urlhausSigs, int &urlhausSizeKB, TQString &urlhausLastModified,
                                           int &otherCount, int &otherSigs, int &otherSizeKB, TQString &otherLastModified)
{
    ssCount = 0; ssSigs = 0; ssSizeKB = 0; ssLastModified = "";
    urlhausCount = 0; urlhausSigs = 0; urlhausSizeKB = 0; urlhausLastModified = "";
    otherCount = 0; otherSigs = 0; otherSizeKB = 0; otherLastModified = "";

    TQDir dir("/var/lib/clamav");
    if (!dir.exists()) return false;

    TQStringList files = dir.entryList(TQDir::Files);
    if (files.isEmpty()) return false;

    static const char* ss_only_dbs[] = {
        "junk.ndb", "jurlbl.ndb", "phish.ndb", "rogue.hdb", "scam.ndb",
        "spamattach.hdb", "spamimg.hdb", "blurl.ndb", "malwarehash.hsb",
        "badmacro.ndb", "spam.ldb"
    };

    TQDateTime ssMaxTime;
    TQDateTime urlhausMaxTime;
    TQDateTime otherMaxTime;

    for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
        TQString name = *it;
        if (name.endsWith(".ndb") || name.endsWith(".hdb") || name.endsWith(".ldb") ||
            name.endsWith(".ign2") || name.endsWith(".ftm") || name.endsWith(".hsb") ||
            name.endsWith(".wdb") || name.endsWith(".fp")) {
            
            bool isUrlhaus = (name == "urlhaus.ndb");
            bool isSS = false;
            if (!isUrlhaus) {
                for (size_t i = 0; i < 11; ++i) {
                    if (name == ss_only_dbs[i]) {
                        isSS = true;
                        break;
                    }
                }
            }

            int fileSizeKB = 0;
            TQDateTime fileTime;
            TQString absPath = dir.absPath() + "/" + name;
            struct stat st;
            if (::stat(absPath.local8Bit(), &st) == 0) {
                fileSizeKB = (st.st_size / 1024);
                fileTime.setTime_t(st.st_mtime);
            }

            // Count signatures by reading line-by-line
            int sigs = 0;
            TQFile file(absPath);
            if (file.open(IO_ReadOnly)) {
                TQTextStream ts(&file);
                while (!ts.atEnd()) {
                    TQString line = ts.readLine().stripWhiteSpace();
                    if (!line.isEmpty() && !line.startsWith("#")) {
                        sigs++;
                    }
                }
                file.close();
            }

            if (isUrlhaus) {
                urlhausCount++;
                urlhausSigs += sigs;
                urlhausSizeKB += fileSizeKB;
                if (!fileTime.isNull() && (urlhausMaxTime.isNull() || fileTime > urlhausMaxTime)) {
                    urlhausMaxTime = fileTime;
                }
            } else if (isSS) {
                ssCount++;
                ssSigs += sigs;
                ssSizeKB += fileSizeKB;
                if (!fileTime.isNull() && (ssMaxTime.isNull() || fileTime > ssMaxTime)) {
                    ssMaxTime = fileTime;
                }
            } else {
                otherCount++;
                otherSigs += sigs;
                otherSizeKB += fileSizeKB;
                if (!fileTime.isNull() && (otherMaxTime.isNull() || fileTime > otherMaxTime)) {
                    otherMaxTime = fileTime;
                }
            }
        }
    }

    if (!ssMaxTime.isNull()) {
        ssLastModified = ssMaxTime.toString("yyyy-MM-dd hh:mm:ss");
    }
    if (!urlhausMaxTime.isNull()) {
        urlhausLastModified = urlhausMaxTime.toString("yyyy-MM-dd hh:mm:ss");
    }
    if (!otherMaxTime.isNull()) {
        otherLastModified = otherMaxTime.toString("yyyy-MM-dd hh:mm:ss");
    }

    return (ssCount > 0 || urlhausCount > 0 || otherCount > 0);
}
