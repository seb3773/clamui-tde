/*
 * updater.cpp - Database updater implementation
 */

#include "updater.h"
#include "settings_manager.h"
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <ntqfile.h>
#include <ntqdir.h>
#include <ntqtextstream.h>
#include <ntqapplication.h>
#include <ntqfileinfo.h>
#include <ntqtimer.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

Updater::Updater(TQObject *parent)
    : TQObject(parent), m_process(0), m_cancelled(false),
      m_runningUnofficial(false), m_officialFailed(false), m_mirror("database.clamav.net"), m_checksPerDay(12)
{
    m_process = new TQProcess(this);
    connect(m_process, TQT_SIGNAL(readyReadStdout()), this, TQT_SLOT(processOutput()));
    connect(m_process, TQT_SIGNAL(readyReadStderr()), this, TQT_SLOT(processError()));
    connect(m_process, TQT_SIGNAL(processExited()), this, TQT_SLOT(processExited()));
    
    m_timeoutTimer = new TQTimer(this);
    connect(m_timeoutTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotTimeout()));
}

Updater::~Updater()
{
    if (m_process && m_process->isRunning()) {
        m_process->kill();
    }
}

TQString Updater::findFreshclam() const
{
    TQString path = TDEGlobal::dirs()->findExe("freshclam");
    if (path.isEmpty()) {
        if (TQFile::exists("/usr/bin/freshclam")) return "/usr/bin/freshclam";
    }
    return path;
}

bool Updater::isServiceRunning() const
{
    int ret = system("systemctl is-active --quiet clamav-freshclam.service 2>/dev/null");
    return (ret == 0);
}

void Updater::sendSignalToService()
{
    TQString tdesudo = TDEGlobal::dirs()->findExe("tdesudo");
    if (!tdesudo.isEmpty()) {
        TQProcess proc;
        proc.addArgument(tdesudo);
        proc.addArgument("-i");
        proc.addArgument("security-high");
        proc.addArgument("-d");
        proc.addArgument("--comment");
        proc.addArgument(i18n("ClamUI needs root access to trigger database update"));
        proc.addArgument("--");
        proc.addArgument("systemctl");
        proc.addArgument("kill");
        proc.addArgument("-s");
        proc.addArgument("SIGUSR1");
        proc.addArgument("clamav-freshclam.service");
        proc.start();
        while (proc.isRunning()) {
            usleep(10000);
        }
    } else {
        TQProcess proc;
        proc.addArgument("systemctl");
        proc.addArgument("kill");
        proc.addArgument("-s");
        proc.addArgument("SIGUSR1");
        proc.addArgument("clamav-freshclam.service");
        proc.start();
        while (proc.isRunning()) {
            usleep(10000);
        }
    }
}

void Updater::startUpdate(bool force)
{
    if (isRunning()) return;

    m_cancelled = false;
    m_runningUnofficial = false;
    m_officialFailed = false;

    TQString freshclam = findFreshclam();
    if (freshclam.isEmpty()) {
        emit updateProgress(i18n("Error: freshclam executable not found"));
        emit updateFinished(ERROR);
        return;
    }

    if (!force && isServiceRunning()) {
        emit updateProgress(i18n("Service is running, sending update signal..."));
        sendSignalToService();
        emit updateFinished(SERVICE_RUNNING);
        return;
    }

    SettingsManager settings;
    bool enableSanesecurity = settings.valueBool("enable_unofficial_sigs", false) &&
                             settings.valueBool("enable_sanesecurity_sigs", false);
    bool enableUrlhaus = settings.valueBool("enable_unofficial_sigs", false) &&
                         settings.valueBool("enable_urlhaus_sigs", false);

    TQString tdeclamuiPath = TQFileInfo(tqApp->argv()[0]).absFilePath();

    TQStringList args;
    args << "--update-additional";
    args << "--official";
    if (force) {
        args << "--force";
    }
    if (enableSanesecurity) {
        args << "--sanesecurity";
    }
    if (enableUrlhaus) {
        args << "--urlhaus";
    }
    if (!m_mirror.isEmpty() && m_mirror != "database.clamav.net") {
        args << "--mirror" << m_mirror;
    }

    TQString tdesudo = TDEGlobal::dirs()->findExe("tdesudo");
    m_process->clearArguments();

    if (!tdesudo.isEmpty()) {
        emit updateProgress(i18n("Elevating privileges using tdesudo..."));
        m_process->addArgument(tdesudo);
        m_process->addArgument("-i");
        m_process->addArgument("security-high");
        m_process->addArgument("-d");
        m_process->addArgument("--comment");
        m_process->addArgument(i18n("ClamUI needs root access to update virus database"));
        m_process->addArgument("--");
        m_process->addArgument(tdeclamuiPath);
        for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
            m_process->addArgument(*it);
        }
    } else {
        emit updateProgress(i18n("Running update..."));
        m_process->addArgument(tdeclamuiPath);
        for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
            m_process->addArgument(*it);
        }
    }
    
    if (!m_process->start()) {
        emit updateProgress(i18n("Failed to start updater process"));
        emit updateFinished(ERROR);
    } else {
        m_timeoutTimer->start(600000); // 10 minutes watchdog
    }
}

void Updater::cancelUpdate()
{
    if (isRunning()) {
        m_cancelled = true;
        m_process->kill();
    }
}

bool Updater::isRunning() const
{
    return m_process && m_process->isRunning();
}

void Updater::processOutput()
{
    while (m_process->canReadLineStdout()) {
        TQString line = m_process->readLineStdout();
        line = line.stripWhiteSpace();
        if (!line.isEmpty()) {
            if (line.contains("cool-down") || line.contains("rate limit") || line.contains("error code 429") || line.contains("error code 403")) {
                m_officialFailed = true;
            }
            if (!line.contains("NotifyClamd") && !line.startsWith("<") && !line.startsWith(">") && !line.startsWith("*")) {
                emit updateProgress(line);
            }
        }
    }
}

void Updater::processError()
{
    while (m_process->canReadLineStderr()) {
        TQString line = m_process->readLineStderr();
        line = line.stripWhiteSpace();
        if (!line.isEmpty()) {
            if (line.contains("cool-down") || line.contains("rate limit") || line.contains("error code 429") || line.contains("error code 403")) {
                m_officialFailed = true;
            }
            if (!line.contains("NotifyClamd") && !line.startsWith("<") && !line.startsWith(">") && !line.startsWith("*")) {
                emit updateProgress("ERR: " + line);
            }
        }
    }
}

void Updater::processExited()
{
    m_timeoutTimer->stop();
    if (m_cancelled) {
        emit updateFinished(CANCELLED);
        return;
    }
    
    if (!m_process->normalExit()) {
        emit updateProgress(i18n("Update process crashed or was killed."));
        emit updateFinished(ERROR);
        return;
    }

    int exitCode = m_process->exitStatus();
    
    bool success = (exitCode == 0) && !m_officialFailed;
    if (!success) {
        emit updateProgress(TQString(i18n("Update failed or was rate limited (exit status %1).")).arg(exitCode));
        emit updateFinished(ERROR);
    } else {
        emit updateProgress(i18n("Update completed successfully."));
        emit updateFinished(SUCCESS);
    }
}

void Updater::slotTimeout()
{
    if (isRunning()) {
        emit updateProgress(i18n("Error: Update timed out after 10 minutes. Cancelling..."));
        cancelUpdate();
    }
}

#include "updater.moc"
