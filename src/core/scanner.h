/*
 * scanner.h - ClamAV Scanner implementation using TQProcess
 */

#ifndef CLAM_SCANNER_H
#define CLAM_SCANNER_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqstringlist.h>
#include <ntqprocess.h>

class Scanner : public TQObject
{
    TQ_OBJECT

public:
    enum ScanStatus {
        CLEAN,
        INFECTED,
        ERROR,
        CANCELLED
    };

    Scanner(TQObject *parent = 0);
    ~Scanner();

    void startScan(const TQStringList &paths, const TQStringList &exclusions = TQStringList(),
                   bool scanArchives = true, bool heuristic = true,
                   const TQString &backend = "auto", int maxFileSizeMB = 25,
                   int niceLevel = 0);
    void cancelScan();
    bool isRunning() const;
    int filesScanned() const { return m_filesScanned; }

signals:
    void scanProgress(int filesScanned, const TQString &currentFile);
    void scanFinished(int status, int infectedCount);
    void threatFound(const TQString &filePath, const TQString &threatName);
    void scanError(const TQString &errorMessage);
    void scanLog(const TQString &logLine);

private slots:
    void processOutput();
    void processError();
    void processExited();

private:
    TQProcess *m_process;
    int m_filesScanned;
    int m_infectedCount;
    bool m_cancelled;
    
    void parseLine(const TQString &line);
};

#endif /* CLAM_SCANNER_H */
