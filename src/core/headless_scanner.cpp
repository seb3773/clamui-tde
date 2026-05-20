/*
 * headless_scanner.cpp - CLI/headless scanner implementation for scheduled scans
 */

#include "headless_scanner.h"
#include <tdeapplication.h>
#include <tdelocale.h>
#include <stdio.h>

HeadlessScanner::HeadlessScanner(const TQString &profileId, TQObject *parent)
    : TQObject(parent), m_profileId(profileId), m_threatCount(0)
{
    m_scanner = new Scanner(this);
    m_profileManager = new ProfileManager(this);
    m_settingsManager = new SettingsManager(this);
    m_logManager = new LogManager(this);

    connect(m_scanner, TQT_SIGNAL(threatFound(const TQString&, const TQString&)),
            this, TQT_SLOT(onThreatFound(const TQString&, const TQString&)));
    connect(m_scanner, TQT_SIGNAL(scanFinished(int, int)),
            this, TQT_SLOT(onScanFinished(int, int)));
    connect(m_scanner, TQT_SIGNAL(scanLog(const TQString&)),
            this, TQT_SLOT(onScanLog(const TQString&)));
    connect(m_scanner, TQT_SIGNAL(scanError(const TQString&)),
            this, TQT_SLOT(onScanError(const TQString&)));
}

HeadlessScanner::~HeadlessScanner()
{
}

void HeadlessScanner::start()
{
    ScanProfile profile = m_profileManager->getProfile(m_profileId);
    if (profile.id.isEmpty()) {
        fprintf(stderr, "Error: Profile ID '%s' not found.\n", m_profileId.latin1());
        tqApp->quit();
        return;
    }

    TQString backend = m_settingsManager->value("scan_backend", "auto");
    int maxFileSizeMB = m_settingsManager->value("max_file_size", "25").toInt();
    int niceLevel = m_settingsManager->value("scan_priority", "0").toInt();

    m_fullLog = i18n("Starting scheduled scan using profile: %1\nTargets:\n").arg(profile.name);
    for (TQStringList::ConstIterator it = profile.paths.begin(); it != profile.paths.end(); ++it) {
        m_fullLog += " - " + (*it) + "\n";
    }
    m_fullLog += "\n";

    m_startTime.start();
    m_scanner->startScan(profile.paths, profile.exclusions, profile.scanArchives,
                          profile.heuristicScan, backend, maxFileSizeMB, niceLevel);
}

void HeadlessScanner::onThreatFound(const TQString &filePath, const TQString &threatName)
{
    m_threatCount++;
    m_fullLog += i18n("Threat found: %1 -> %2\n").arg(filePath).arg(threatName);
    printf("Threat: %s -> %s\n", filePath.latin1(), threatName.latin1());
}

void HeadlessScanner::onScanLog(const TQString &line)
{
    m_fullLog += line + "\n";
}

void HeadlessScanner::onScanError(const TQString &msg)
{
    m_fullLog += i18n("Error: %1\n").arg(msg);
    fprintf(stderr, "Scan Error: %s\n", msg.latin1());
}

void HeadlessScanner::onScanFinished(int status, int infectedCount)
{
    double duration = m_startTime.elapsed() / 1000.0;
    int logStatus = 0;
    
    switch (status) {
        case Scanner::CLEAN:
            m_fullLog += i18n("\nScan complete. No threats found.\n");
            m_fullLog += "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: 0\n";
            logStatus = 0;
            break;
        case Scanner::INFECTED:
            m_fullLog += i18n("\nScan complete. %1 threat(s) found!\n").arg(m_threatCount);
            m_fullLog += "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(m_threatCount) + "\n";
            logStatus = 1;
            break;
        case Scanner::ERROR:
            m_fullLog += i18n("\nScan failed.\n");
            m_fullLog += "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(m_threatCount) + "\n";
            logStatus = 2;
            break;
        case Scanner::CANCELLED:
            m_fullLog += i18n("\nScan cancelled.\n");
            m_fullLog += "\nScanned files: " + TQString::number(m_scanner->filesScanned())
                       + "\nInfected files: " + TQString::number(m_threatCount) + "\n";
            logStatus = 2;
            break;
    }

    m_logManager->addLog(0, logStatus, m_fullLog, duration);
    printf("Scan finished. Status: %d, Infected: %d\n", status, m_threatCount);
    tqApp->quit();
}

#include "headless_scanner.moc"
