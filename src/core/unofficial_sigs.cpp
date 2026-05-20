/*
 * unofficial_sigs.cpp - ClamAV unofficial signatures downloader and verifier implementation
 */

#include "unofficial_sigs.h"
#include "settings_manager.h"
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <ntqdir.h>
#include <ntqfile.h>
#include <ntqfileinfo.h>
#include <ntqprocess.h>
#include <ntqapplication.h>
#include <sys/stat.h>
#include <unistd.h>

static TQString findExecutableInPath(const TQString &cmd)
{
    if (cmd.contains('/') && TQFile::exists(cmd)) {
        return cmd;
    }
    
    char *pathEnv = getenv("PATH");
    if (!pathEnv) return TQString::null;
    
    TQStringList paths = TQStringList::split(':', pathEnv);
    for (TQStringList::Iterator it = paths.begin(); it != paths.end(); ++it) {
        TQString fullPath = *it + "/" + cmd;
        if (TQFile::exists(fullPath)) {
            if (access(fullPath.local8Bit(), X_OK) == 0) {
                return fullPath;
            }
        }
    }
    
    return TQString::null;
}

UnofficialSigsUpdater::UnofficialSigsUpdater(TQObject *parent)
    : TQObject(parent)
{
    m_workDir = "/var/lib/tdeclamui/unofficial-sigs";
    m_clamavDir = "/var/lib/clamav";
    m_gpgDir = m_workDir + "/gpg";
    m_sanesecurityEnabled = false;
    m_urlhausEnabled = false;
}

UnofficialSigsUpdater::~UnofficialSigsUpdater()
{
}

static bool copyFile(const TQString &src, const TQString &dst)
{
    TQFile srcFile(src);
    TQFile dstFile(dst);
    if (!srcFile.open(IO_ReadOnly)) return false;
    if (!dstFile.open(IO_WriteOnly)) return false;
    
    char buffer[4096];
    int bytesRead;
    while ((bytesRead = srcFile.readBlock(buffer, sizeof(buffer))) > 0) {
        if (dstFile.writeBlock(buffer, bytesRead) != bytesRead) {
            dstFile.close();
            srcFile.close();
            TQFile::remove(dst);
            return false;
        }
    }
    return true;
}

bool UnofficialSigsUpdater::runUpdate(TQStringList &realLogOutput, bool force)
{
    IncrementalLogger logOutput(realLogOutput);
    bool enableSanesecurity = m_sanesecurityEnabled;

    if (!force) {
        SettingsManager settings;
        if (!settings.valueBool("enable_unofficial_sigs", false)) {
            logOutput << "Unofficial signatures support is disabled in settings.";
            return true;
        }
    }

    logOutput << "Starting unofficial signatures update (Sanesecurity)... [5%]";

    // 1. Create working directories
    TQString tdeclamuiDir = "/var/lib/tdeclamui";
    if (!TQFileInfo(tdeclamuiDir).exists()) {
        TQDir().mkdir(tdeclamuiDir);
    }
    if (!TQFileInfo(m_workDir).exists()) {
        TQDir().mkdir(m_workDir);
    }
    if (!TQFileInfo(m_gpgDir).exists()) {
        TQDir().mkdir(m_gpgDir);
    }
    chmod(m_gpgDir.local8Bit(), 0700);

    // 2. Download public GPG key if not present
    TQString pubKeyPath = m_workDir + "/publickey.gpg";
    if (!TQFile::exists(pubKeyPath)) {
        logOutput << "Downloading Sanesecurity public key...";
        if (!downloadFile("https://www.sanesecurity.com/publickey.gpg", pubKeyPath, logOutput)) {
            logOutput << "Error: Failed to download Sanesecurity GPG key.";
            return false;
        }
    }

    // 3. Import public key into GPG keyring
    logOutput << "Importing GPG public key...";
    TQStringList importArgs;
    importArgs << "--no-default-keyring"
               << "--homedir" << m_gpgDir
               << "--keyring" << m_gpgDir + "/ss-keyring.gpg"
               << "--import" << pubKeyPath;
    int gpgRet = -1;
    if (!runCommand("gpg", importArgs, logOutput, &gpgRet) || gpgRet != 0) {
        logOutput << "Error: GPG public key import failed.";
        return false;
    }

    // 4. Download and verify databases
    const char* sanesecurity_dbs[] = {
        "sanesecurity.ftm",
        "sigwhitelist.ign2",
        "junk.ndb",
        "jurlbl.ndb",
        "phish.ndb",
        "rogue.hdb",
        "scam.ndb",
        "spamattach.hdb",
        "spamimg.hdb",
        "blurl.ndb",
        "malwarehash.hsb",
        "badmacro.ndb",
        "spam.ldb"
    };
    int numDbs = enableSanesecurity ? 13 : 0;

    bool overallSuccess = true;
    bool anyUpdated = false;

    for (int i = 0; i < numDbs; ++i) {
        TQString dbName = sanesecurity_dbs[i];
        logOutput << "-----------------------------------------------";
        logOutput << TQString("Processing database: %1 [%2%]").arg(dbName).arg(5 + (i * 90) / numDbs);

        // Download database and its .sig using rsync
        TQString localDbPath = m_workDir + "/" + dbName;
        TQString localSigPath = m_workDir + "/" + dbName + ".sig";

        TQStringList rsyncArgs;
        rsyncArgs << "-tu"
                  << "--timeout=30"
                  << "--contimeout=15"
                  << "rsync://rsync.sanesecurity.net/sanesecurity/" + dbName
                  << m_workDir + "/";

        TQStringList rsyncSigArgs;
        rsyncSigArgs << "-tu"
                     << "--timeout=30"
                     << "--contimeout=15"
                     << "rsync://rsync.sanesecurity.net/sanesecurity/" + dbName + ".sig"
                     << m_workDir + "/";

        int rsyncRet = -1;
        logOutput << "Downloading " + dbName + " via rsync...";
        if (!runCommand("rsync", rsyncArgs, logOutput, &rsyncRet) || rsyncRet != 0) {
            logOutput << "Warning: Failed to download database " + dbName + " via rsync.";
            overallSuccess = false;
            continue;
        }

        logOutput << "Downloading signature for " + dbName + " via rsync...";
        if (!runCommand("rsync", rsyncSigArgs, logOutput, &rsyncRet) || rsyncRet != 0) {
            logOutput << "Warning: Failed to download signature for " + dbName + " via rsync.";
            overallSuccess = false;
            continue;
        }

        // Verify signature using GPG
        logOutput << "Verifying GPG signature for " + dbName + "...";
        TQStringList verifyArgs;
        verifyArgs << "--trust-model" << "always"
                   << "--no-default-keyring"
                   << "--homedir" << m_gpgDir
                   << "--keyring" << m_gpgDir + "/ss-keyring.gpg"
                   << "--verify" << localSigPath
                   << localDbPath;
        int verifyRet = -1;
        if (!runCommand("gpg", verifyArgs, logOutput, &verifyRet) || verifyRet != 0) {
            logOutput << "Error: GPG verification failed for " + dbName;
            overallSuccess = false;
            continue;
        }
        logOutput << "GPG verification successful.";

        // Run integrity test using clamscan
        logOutput << "Testing database integrity with clamscan...";
        TQStringList testArgs;
        testArgs << "--database=" + localDbPath
                 << "/etc/hosts";
        int testRet = -1;
        if (!runCommand("clamscan", testArgs, logOutput, &testRet) || testRet != 0) {
            logOutput << "Error: ClamAV integrity test failed for " + dbName;
            overallSuccess = false;
            continue;
        }
        logOutput << "ClamAV database test passed.";

        // Check if database is different from production
        TQString prodDbPath = m_clamavDir + "/" + dbName;
        bool copyNeeded = true;
        if (TQFile::exists(prodDbPath)) {
            TQFileInfo localInfo(localDbPath);
            TQFileInfo prodInfo(prodDbPath);
            if (localInfo.lastModified() == prodInfo.lastModified() && localInfo.size() == prodInfo.size()) {
                copyNeeded = false;
            }
        }

        if (copyNeeded) {
            logOutput << "Deploying " + dbName + " to " + prodDbPath + "...";
            TQString tempProdPath = prodDbPath + ".tmp";
            if (TQFile::exists(tempProdPath)) {
                TQFile::remove(tempProdPath);
            }
            if (copyFile(localDbPath, tempProdPath)) {
                chmod(tempProdPath.local8Bit(), 0644);
                if (::rename(tempProdPath.local8Bit(), prodDbPath.local8Bit()) == 0) {
                    anyUpdated = true;
                } else {
                    logOutput << "Error: Failed to rename temporary file to " + prodDbPath;
                    overallSuccess = false;
                }
            } else {
                logOutput << "Error: Failed to copy " + dbName + " to temporary production file.";
                overallSuccess = false;
            }
        } else {
            logOutput << dbName + " is already up to date in production.";
        }
    }

    // 5. Download and verify URLhaus database if enabled
    bool enableUrlhaus = m_urlhausEnabled;
    if (enableUrlhaus || force) {
        if (enableUrlhaus) {
            logOutput << "-----------------------------------------------";
            logOutput << "Processing database: urlhaus.ndb [95%]";
            TQString dbName = "urlhaus.ndb";
            TQString localDbPath = m_workDir + "/" + dbName;
            TQString url = "https://urlhaus.abuse.ch/downloads/urlhaus.ndb";

            logOutput << "Downloading " + dbName + " via HTTP...";
            if (downloadFile(url, localDbPath, logOutput)) {
                // Run integrity test using clamscan
                logOutput << "Testing database integrity with clamscan...";
                TQStringList testArgs;
                testArgs << "--database=" + localDbPath
                         << "/etc/hosts";
                int testRet = -1;
                if (runCommand("clamscan", testArgs, logOutput, &testRet) && testRet == 0) {
                    logOutput << "ClamAV database test passed.";

                    // Check if database is different from production
                    TQString prodDbPath = m_clamavDir + "/" + dbName;
                    bool copyNeeded = true;
                    if (TQFile::exists(prodDbPath)) {
                        TQFileInfo localInfo(localDbPath);
                        TQFileInfo prodInfo(prodDbPath);
                        if (localInfo.lastModified() == prodInfo.lastModified() && localInfo.size() == prodInfo.size()) {
                            copyNeeded = false;
                        }
                    }

                    if (copyNeeded) {
                        logOutput << "Deploying " + dbName + " to " + prodDbPath + "...";
                        TQString tempProdPath = prodDbPath + ".tmp";
                        if (TQFile::exists(tempProdPath)) {
                            TQFile::remove(tempProdPath);
                        }
                        if (copyFile(localDbPath, tempProdPath)) {
                            chmod(tempProdPath.local8Bit(), 0644);
                            if (::rename(tempProdPath.local8Bit(), prodDbPath.local8Bit()) == 0) {
                                anyUpdated = true;
                            } else {
                                logOutput << "Error: Failed to rename temporary file to " + prodDbPath;
                                overallSuccess = false;
                            }
                        } else {
                            logOutput << "Error: Failed to copy " + dbName + " to temporary production file.";
                            overallSuccess = false;
                        }
                    } else {
                        logOutput << dbName + " is already up to date in production.";
                    }
                } else {
                    logOutput << "Error: ClamAV integrity test failed for " + dbName;
                    overallSuccess = false;
                }
            } else {
                logOutput << "Warning: Failed to download database " + dbName + " via HTTP.";
                overallSuccess = false;
            }
        }
    }

    if (anyUpdated) {
        logOutput << "Reloading ClamAV daemon databases...";
        TQStringList reloadArgs;
        reloadArgs << "--reload";
        int reloadRet = -1;
        runCommand("clamdscan", reloadArgs, logOutput, &reloadRet);
    }

    logOutput << "-----------------------------------------------";
    logOutput << "Unofficial signatures update completed. [100%]";
    return overallSuccess;
}

bool UnofficialSigsUpdater::downloadFile(const TQString &url, const TQString &outputPath, IncrementalLogger &logOutput)
{
    TQString curlPath = findExecutableInPath("curl");
    if (!curlPath.isEmpty()) {
        TQStringList args;
        args << "-s" << "-L" << "--connect-timeout" << "15" << "-m" << "60" << url << "-o" << outputPath;
        int ret = -1;
        if (runCommand("curl", args, logOutput, &ret) && ret == 0) {
            return true;
        }
    }
    TQString wgetPath = findExecutableInPath("wget");
    if (!wgetPath.isEmpty()) {
        TQStringList args;
        args << "-q" << "--connect-timeout=15" << "--timeout=60" << url << "-O" << outputPath;
        int ret = -1;
        if (runCommand("wget", args, logOutput, &ret) && ret == 0) {
            return true;
        }
    }
    logOutput << "Failed to download: " + url + " (neither curl nor wget succeeded)";
    return false;
}

bool UnofficialSigsUpdater::runCommand(const TQString &cmd, const TQStringList &args, IncrementalLogger &logOutput, int *exitCode)
{
    TQString resolvedCmd = findExecutableInPath(cmd);
    if (resolvedCmd.isEmpty()) {
        if (cmd == "gpg" && TQFile::exists("/usr/bin/gpg")) resolvedCmd = "/usr/bin/gpg";
        else if (cmd == "rsync" && TQFile::exists("/usr/bin/rsync")) resolvedCmd = "/usr/bin/rsync";
        else if (cmd == "clamscan" && TQFile::exists("/usr/bin/clamscan")) resolvedCmd = "/usr/bin/clamscan";
        else if (cmd == "clamdscan" && TQFile::exists("/usr/bin/clamdscan")) resolvedCmd = "/usr/bin/clamdscan";
        else {
            logOutput << "Executable not found: " + cmd;
            return false;
        }
    }

    TQProcess proc;
    proc.addArgument(resolvedCmd);
    for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
        proc.addArgument(*it);
    }

    if (!proc.start()) {
        logOutput << "Failed to start command: " + cmd;
        return false;
    }

    while (proc.isRunning()) {
        while (proc.canReadLineStdout()) {
            TQString line = proc.readLineStdout().stripWhiteSpace();
            if (!line.isEmpty()) {
                logOutput << line;
            }
        }
        while (proc.canReadLineStderr()) {
            TQString line = proc.readLineStderr().stripWhiteSpace();
            if (!line.isEmpty()) {
                logOutput << "ERR: " + line;
            }
        }
        if (tqApp && tqApp->type() != TQApplication::Tty) {
            tqApp->processEvents();
        }
        usleep(10000);
    }

    while (proc.canReadLineStdout()) {
        TQString line = proc.readLineStdout().stripWhiteSpace();
        if (!line.isEmpty()) {
            logOutput << line;
        }
    }
    while (proc.canReadLineStderr()) {
        TQString line = proc.readLineStderr().stripWhiteSpace();
        if (!line.isEmpty()) {
            logOutput << "ERR: " + line;
        }
    }

    if (exitCode) {
        *exitCode = proc.exitStatus();
    }
    return true;
}

bool UnofficialSigsUpdater::runOfficialUpdate(TQStringList &logOutput, bool force, const TQString &mirror)
{
    IncrementalLogger logger(logOutput);
    logger << "Starting official database update...";

    if (force) {
        logger << "Stopping clamav-freshclam service...";
        TQStringList stopArgs;
        stopArgs << "stop" << "clamav-freshclam.service";
        int stopRet = -1;
        runCommand("systemctl", stopArgs, logger, &stopRet);

        logger << "Creating secure backup of official databases...";
        TQString backupDir = "/var/lib/tdeclamui/backup";
        TQStringList mkdirArgs;
        mkdirArgs << "-p" << backupDir;
        int mkdirRet = -1;
        runCommand("mkdir", mkdirArgs, logger, &mkdirRet);

        TQStringList chmodArgs;
        chmodArgs << "700" << backupDir;
        runCommand("chmod", chmodArgs, logger);

        // Copy existing CVD and CLD files to backup dir
        TQDir clamDir(m_clamavDir);
        if (clamDir.exists()) {
            TQStringList files = clamDir.entryList("*.cvd *.cld", TQDir::Files);
            for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
                TQString src = m_clamavDir + "/" + *it;
                TQString dst = backupDir + "/" + *it;
                copyFile(src, dst);
            }
        }
    }

    logger << "Running freshclam...";
    TQStringList freshArgs;
    freshArgs << "--verbose";

    int freshRet = -1;
    bool runOk = runCommand("freshclam", freshArgs, logger, &freshRet);
    bool officialFailed = false;

    // Check logs for rate limits
    for (TQStringList::ConstIterator it = logOutput.begin(); it != logOutput.end(); ++it) {
        TQString line = (*it).lower();
        if (line.contains("cool-down") || line.contains("rate limit") || line.contains("429") || line.contains("403")) {
            officialFailed = true;
        }
    }

    bool freshSuccess = runOk && (freshRet == 0) && !officialFailed;

    if (force) {
        // Check if databases are OK (at least daily or main file is non-empty)
        bool dbOk = false;
        TQDir clamDir(m_clamavDir);
        if (clamDir.exists()) {
            TQStringList files = clamDir.entryList("daily.cvd daily.cld main.cvd main.cld", TQDir::Files);
            for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
                TQFileInfo fi(m_clamavDir + "/" + *it);
                if (fi.size() > 0) {
                    dbOk = true;
                    break;
                }
            }
        }

        if (!freshSuccess || !dbOk) {
            logger << "Update failed or database missing. Restoring backup...";
            TQString backupDir = "/var/lib/tdeclamui/backup";
            TQDir bDir(backupDir);
            if (bDir.exists()) {
                TQStringList files = bDir.entryList(TQDir::Files);
                for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
                    TQString src = backupDir + "/" + *it;
                    TQString dst = m_clamavDir + "/" + *it;
                    copyFile(src, dst);
                }
            }
        }

        // Clean up backup directory
        logger << "Cleaning up backup directory...";
        TQString backupDir = "/var/lib/tdeclamui/backup";
        TQStringList rmArgs;
        rmArgs << "-rf" << backupDir;
        runCommand("rm", rmArgs, logger);

        logger << "Restarting clamav-freshclam service...";
        TQStringList startArgs;
        startArgs << "start" << "clamav-freshclam.service";
        runCommand("systemctl", startArgs, logger);
    }

    return freshSuccess;
}

#include "unofficial_sigs.moc"
