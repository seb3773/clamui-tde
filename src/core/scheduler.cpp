/*
 * scheduler.cpp - Scheduled scans implementation
 *
 * Manages systemd user timers (preferred) or cron jobs (fallback)
 * for automatic scheduled scans. All process execution uses
 * TQProcess instead of system() for proper C++ integration.
 */

#include "scheduler.h"
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <ntqfile.h>
#include <ntqdir.h>
#include <ntqtextstream.h>
#include <ntqstringlist.h>
#include <ntqprocess.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tdeapplication.h>

Scheduler::Scheduler(TQObject *parent)
    : TQObject(parent)
{
}

Scheduler::~Scheduler()
{
}

/* --- Utility: recursive directory creation in pure C++ --- */
static bool mkdirRecursive(const TQString &path)
{
    TQDir dir(path);
    if (dir.exists()) return true;
    
    /* Ensure parent exists first */
    TQString parent = TQFileInfo(path).dirPath(true);
    if (!TQDir(parent).exists()) {
        if (!mkdirRecursive(parent)) return false;
    }
    return TQDir(parent).mkdir(TQFileInfo(path).fileName());
}

/* --- Utility: run a process synchronously, return exit code --- */
static int runProcess(const TQString &program, const TQStringList &args)
{
    TQProcess proc;
    proc.addArgument(program);
    for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
        proc.addArgument(*it);
    }
    if (!proc.start()) return -1;
    
    /* Block until process finishes (TQProcess in TQt3 is async, poll) */
    while (proc.isRunning()) {
        tqApp->processEvents();
        usleep(10000); /* 10ms */
    }
    return proc.exitStatus();
}

/* --- Utility: run a process and capture stdout --- */
static TQString runProcessOutput(const TQString &program, const TQStringList &args)
{
    TQProcess proc;
    proc.addArgument(program);
    for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
        proc.addArgument(*it);
    }
    if (!proc.start()) return TQString::null;
    
    while (proc.isRunning()) {
        tqApp->processEvents();
        usleep(10000);
    }
    
    TQString output;
    while (proc.canReadLineStdout()) {
        output += proc.readLineStdout() + "\n";
    }
    return output;
}

bool Scheduler::isSystemdAvailable() const
{
    return TQFile::exists("/bin/systemctl") || TQFile::exists("/usr/bin/systemctl");
}

bool Scheduler::isCronAvailable() const
{
    return !TDEGlobal::dirs()->findExe("crontab").isEmpty();
}

TQString Scheduler::buildCommand(const TQString &profileId) const
{
    /* Primary: resolve via /proc/self/exe (always correct, even from build dir) */
    TQString exe;
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        exe = TQString::fromLocal8Bit(buf);
    }
    
    /* Fallback: search PATH */
    if (exe.isEmpty()) {
        exe = TDEGlobal::dirs()->findExe("tdeclamui");
    }
    
    if (exe.isEmpty()) return TQString::null;
    
    return TQString("%1 --scan-profile %2").arg(exe).arg(profileId);
}

bool Scheduler::createSchedule(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery)
{
    if (isSystemdAvailable()) {
        return createSystemdTimer(frequency, time, profileId, skipOnBattery);
    } else if (isCronAvailable()) {
        return createCronJob(frequency, time, profileId, skipOnBattery);
    }
    return false;
}

bool Scheduler::removeSchedule()
{
    bool ok = true;
    if (isSystemdAvailable()) {
        ok = removeSystemdTimer();
    }
    if (isCronAvailable()) {
        ok = ok && removeCronJob();
    }
    return ok;
}

bool Scheduler::isScheduled() const
{
    if (isSystemdAvailable()) {
        TQStringList args;
        args << "--user" << "is-enabled" << "--quiet" << "clamui-scan.timer";
        int ret = runProcess("systemctl", args);
        if (ret == 0) return true;
    }
    
    if (isCronAvailable()) {
        TQStringList args;
        args << "-l";
        TQString crontab = runProcessOutput("crontab", args);
        if (crontab.contains("tdeclamui")) return true;
    }
    
    return false;
}

bool Scheduler::createSystemdTimer(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery)
{
    TQString configDir = TQDir::homeDirPath() + "/.config/systemd/user";
    if (!mkdirRecursive(configDir)) return false;
    
    TQString exe = buildCommand(profileId);
    if (exe.isEmpty()) return false;
    
    TQString serviceContent = 
        "[Unit]\n"
        "Description=TDE ClamUI Scheduled Scan\n"
        "After=network.target\n";
        
    if (skipOnBattery) {
        serviceContent += "ConditionACPower=true\n";
    }
    
    serviceContent += 
        "\n"
        "[Service]\n"
        "Type=oneshot\n"
        "ExecStart=" + exe + " --nofork\n"
        "Nice=19\n"
        "IOSchedulingClass=idle\n\n"
        "[Install]\n"
        "WantedBy=default.target\n";
        
    TQFile serviceFile(configDir + "/clamui-scan.service");
    if (serviceFile.open(IO_WriteOnly)) {
        TQTextStream s(&serviceFile);
        s << serviceContent;
        serviceFile.close();
    } else {
        return false;
    }
    
    int hour = 2, minute = 0;
    TQStringList parts = TQStringList::split(":", time);
    if (parts.count() == 2) {
        hour = parts[0].toInt();
        minute = parts[1].toInt();
    }
    
    TQString hh = TQString::number(hour).rightJustify(2, '0');
    TQString mm = TQString::number(minute).rightJustify(2, '0');
    
    TQString calendarSpec;
    if (frequency == "weekly") {
        calendarSpec = TQString("Mon *-*-* %1:%2:00").arg(hh).arg(mm);
    } else if (frequency == "monthly") {
        calendarSpec = TQString("*-*-01 %1:%2:00").arg(hh).arg(mm);
    } else {
        calendarSpec = TQString("*-*-* %1:%2:00").arg(hh).arg(mm);
    }
    
    TQString timerContent =
        "[Unit]\n"
        "Description=TDE ClamUI Scheduled Scan Timer\n\n"
        "[Timer]\n"
        "OnCalendar=" + calendarSpec + "\n"
        "Persistent=true\n\n"
        "[Install]\n"
        "WantedBy=timers.target\n";
        
    TQFile timerFile(configDir + "/clamui-scan.timer");
    if (timerFile.open(IO_WriteOnly)) {
        TQTextStream s(&timerFile);
        s << timerContent;
        timerFile.close();
    } else {
        return false;
    }
    
    /* Reload systemd and enable the timer */
    TQStringList reloadArgs;
    reloadArgs << "--user" << "daemon-reload";
    runProcess("systemctl", reloadArgs);
    
    TQStringList enableArgs;
    enableArgs << "--user" << "enable" << "--now" << "clamui-scan.timer";
    int ret = runProcess("systemctl", enableArgs);
    return (ret == 0);
}

bool Scheduler::removeSystemdTimer()
{
    /* Disable and stop the timer */
    TQStringList disableArgs;
    disableArgs << "--user" << "disable" << "--now" << "clamui-scan.timer";
    runProcess("systemctl", disableArgs);
    
    /* Remove unit files */
    TQString configDir = TQDir::homeDirPath() + "/.config/systemd/user";
    TQFile::remove(configDir + "/clamui-scan.service");
    TQFile::remove(configDir + "/clamui-scan.timer");
    
    /* Reload daemon */
    TQStringList reloadArgs;
    reloadArgs << "--user" << "daemon-reload";
    runProcess("systemctl", reloadArgs);
    return true;
}

/* --- Cron helpers using TQProcess --- */

static TQString getCrontab()
{
    TQStringList args;
    args << "-l";
    return runProcessOutput("crontab", args);
}

static bool setCrontab(const TQString &content)
{
    /* Write to temp file, then load via crontab */
    TQString tmpPath = TQDir::homeDirPath() + "/.clamui-crontab.tmp";
    TQFile tmpFile(tmpPath);
    if (!tmpFile.open(IO_WriteOnly)) return false;
    TQTextStream s(&tmpFile);
    s << content;
    tmpFile.close();
    
    TQStringList args;
    args << tmpPath;
    int ret = runProcess("crontab", args);
    
    TQFile::remove(tmpPath);
    return (ret == 0);
}

bool Scheduler::createCronJob(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery)
{
    TQString command = buildCommand(profileId);
    if (command.isEmpty()) return false;
    
    if (skipOnBattery) {
        command = "if type on_ac_power >/dev/null 2>&1; then on_ac_power || exit 0; fi; " + command;
    }
    
    int hour = 2, minute = 0;
    TQStringList parts = TQStringList::split(":", time);
    if (parts.count() == 2) {
        hour = parts[0].toInt();
        minute = parts[1].toInt();
    }
    
    TQString cronSpec;
    if (frequency == "weekly") {
        cronSpec = TQString("%1 %2 * * 1").arg(minute).arg(hour);
    } else if (frequency == "monthly") {
        cronSpec = TQString("%1 %2 1 * *").arg(minute).arg(hour);
    } else {
        cronSpec = TQString("%1 %2 * * *").arg(minute).arg(hour);
    }
    
    TQString currentCrontab = getCrontab();
    TQStringList lines = TQStringList::split('\n', currentCrontab);
    TQStringList newLines;
    
    bool skipNext = false;
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        if (skipNext) {
            skipNext = false;
            continue;
        }
        if ((*it).stripWhiteSpace() == "# TDE ClamUI Scheduled Scan") {
            skipNext = true;
            continue;
        }
        newLines.append(*it);
    }
    
    newLines.append("# TDE ClamUI Scheduled Scan");
    newLines.append(TQString("%1 %2").arg(cronSpec).arg(command));
    
    TQString newCrontab = newLines.join("\n") + "\n";
    return setCrontab(newCrontab);
}

bool Scheduler::removeCronJob()
{
    TQString currentCrontab = getCrontab();
    if (currentCrontab.isEmpty()) return true;
    
    TQStringList lines = TQStringList::split('\n', currentCrontab);
    TQStringList newLines;
    
    bool skipNext = false;
    for (TQStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it) {
        if (skipNext) {
            skipNext = false;
            continue;
        }
        if ((*it).stripWhiteSpace() == "# TDE ClamUI Scheduled Scan") {
            skipNext = true;
            continue;
        }
        newLines.append(*it);
    }
    
    TQString newCrontab = newLines.join("\n").stripWhiteSpace();
    if (newCrontab.isEmpty()) {
        /* Remove crontab entirely */
        TQStringList args;
        args << "-r";
        runProcess("crontab", args);
        return true;
    }
    return setCrontab(newCrontab + "\n");
}

#include "scheduler.moc"
