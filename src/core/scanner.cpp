/*
 * scanner.cpp - ClamAV Scanner implementation
 */

#include "scanner.h"
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <ntqfile.h>

Scanner::Scanner(TQObject *parent)
    : TQObject(parent), m_process(0), m_filesScanned(0), m_infectedCount(0), m_cancelled(false)
{
    m_process = new TQProcess(this);
    connect(m_process, TQT_SIGNAL(readyReadStdout()), this, TQT_SLOT(processOutput()));
    connect(m_process, TQT_SIGNAL(readyReadStderr()), this, TQT_SLOT(processError()));
    connect(m_process, TQT_SIGNAL(processExited()), this, TQT_SLOT(processExited()));
}

Scanner::~Scanner()
{
    if (m_process && m_process->isRunning()) {
        m_process->kill();
    }
}

void Scanner::startScan(const TQStringList &paths, const TQStringList &exclusions,
                        bool scanArchives, bool heuristic,
                        const TQString &backend, int maxFileSizeMB,
                        int niceLevel)
{
    if (isRunning()) return;

    m_filesScanned = 0;
    m_infectedCount = 0;
    m_cancelled = false;

    // Determine which binary to use based on backend preference
    TQString scanBin;
    bool useDaemon = false;

    if (backend == "daemon") {
        scanBin = TDEGlobal::dirs()->findExe("clamdscan");
        if (scanBin.isEmpty()) {
            emit scanError("clamdscan not found, falling back to clamscan");
            scanBin = TDEGlobal::dirs()->findExe("clamscan");
        } else {
            useDaemon = true;
        }
    } else if (backend == "clamscan") {
        scanBin = TDEGlobal::dirs()->findExe("clamscan");
    } else {
        // auto: prefer clamdscan if available
        scanBin = TDEGlobal::dirs()->findExe("clamdscan");
        if (!scanBin.isEmpty()) {
            useDaemon = true;
        } else {
            scanBin = TDEGlobal::dirs()->findExe("clamscan");
        }
    }

    if (scanBin.isEmpty()) {
        emit scanError("No ClamAV scanner executable found (clamscan / clamdscan)");
        return;
    }

    m_process->clearArguments();
    
    // Apply process priority via nice/ionice (clamscan only, not daemon)
    if (!useDaemon && niceLevel > 0) {
        // ionice: class 3 = idle (only use disk when nobody else needs it)
        m_process->addArgument("ionice");
        m_process->addArgument("-c");
        m_process->addArgument("3");
        // nice: 10 = low, 19 = very low
        m_process->addArgument("nice");
        m_process->addArgument("-n");
        m_process->addArgument(TQString::number(niceLevel == 1 ? 10 : 19));
    }
    
    m_process->addArgument(scanBin);
    m_process->addArgument("--recursive");
    m_process->addArgument("--no-summary");

    if (!useDaemon) {
        // clamscan-specific options
        m_process->addArgument(TQString("--max-filesize=%1M").arg(maxFileSizeMB));
        m_process->addArgument(TQString("--max-scansize=%1M").arg(maxFileSizeMB));

        if (scanArchives) {
            m_process->addArgument("--scan-archive=yes");
        } else {
            m_process->addArgument("--scan-archive=no");
        }

        if (heuristic) {
            m_process->addArgument("--heuristic-alerts=yes");
            m_process->addArgument("--heuristic-scan-precedence=yes");
        } else {
            m_process->addArgument("--heuristic-alerts=no");
        }

        // Exclusions
        for (TQStringList::ConstIterator it = exclusions.begin(); it != exclusions.end(); ++it) {
            if (!(*it).stripWhiteSpace().isEmpty()) {
                m_process->addArgument("--exclude-dir=" + (*it).stripWhiteSpace());
            }
        }
    } else {
        // clamdscan: fewer options, but pass --multiscan for speed
        m_process->addArgument("--multiscan");
        m_process->addArgument("--fdpass");
    }

    for (TQStringList::ConstIterator it = paths.begin(); it != paths.end(); ++it) {
        m_process->addArgument(*it);
    }

    if (!m_process->start()) {
        emit scanError("Failed to start scanner process");
    }
}

void Scanner::cancelScan()
{
    if (isRunning()) {
        m_cancelled = true;
        m_process->kill();
    }
}

bool Scanner::isRunning() const
{
    return m_process && m_process->isRunning();
}

void Scanner::processOutput()
{
    while (m_process->canReadLineStdout()) {
        TQString line = m_process->readLineStdout();
        emit scanLog(line); // Emit raw line for logs
        line = line.stripWhiteSpace();
        if (!line.isEmpty()) {
            parseLine(line);
        }
    }
}

void Scanner::processError()
{
    while (m_process->canReadLineStderr()) {
        TQString line = m_process->readLineStderr();
        emit scanLog(line); // Emit raw line for logs
    }
}

void Scanner::processExited()
{
    if (m_cancelled) {
        emit scanFinished(CANCELLED, m_infectedCount);
        return;
    }
    
    if (!m_process->normalExit()) {
        emit scanFinished(ERROR, m_infectedCount);
        return;
    }

    int exitCode = m_process->exitStatus();
    // clamscan exit codes: 0 = clean, 1 = infected, 2 = error
    if (exitCode == 0) {
        emit scanFinished(CLEAN, 0);
    } else if (exitCode == 1) {
        emit scanFinished(INFECTED, m_infectedCount);
    } else {
        emit scanFinished(ERROR, m_infectedCount);
    }
}

void Scanner::parseLine(const TQString &line)
{
    // Simple parsing for clamscan output
    if (line.endsWith(" FOUND")) {
        int colonIdx = line.find(':');
        if (colonIdx != -1) {
            TQString path = line.left(colonIdx).stripWhiteSpace();
            TQString threat = line.mid(colonIdx + 1, line.length() - colonIdx - 7).stripWhiteSpace();
            m_infectedCount++;
            m_filesScanned++;
            emit threatFound(path, threat);
            emit scanProgress(m_filesScanned, path);
        }
    } else if (line.endsWith(" OK")) {
        int colonIdx = line.find(':');
        if (colonIdx != -1) {
            TQString path = line.left(colonIdx).stripWhiteSpace();
            m_filesScanned++;
            emit scanProgress(m_filesScanned, path);
        }
    } else if (line.endsWith(" ERROR") || line.endsWith(" Empty file")) {
        int colonIdx = line.find(':');
        if (colonIdx != -1) {
            TQString path = line.left(colonIdx).stripWhiteSpace();
            m_filesScanned++;
            emit scanProgress(m_filesScanned, path);
        }
    } else {
        emit scanProgress(m_filesScanned, line);
    }
}

#include "scanner.moc"
