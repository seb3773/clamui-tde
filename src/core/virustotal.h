/*
 * virustotal.h - VirusTotal API v3 integration
 */

#ifndef CLAM_VIRUSTOTAL_H
#define CLAM_VIRUSTOTAL_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqvaluelist.h>

enum VTScanStatus {
    VT_FOUND,
    VT_NOT_FOUND,
    VT_QUEUED,
    VT_ERROR
};

struct VTScanResult {
    VTScanStatus status;
    TQString message;
    int positives;
    int total;
    TQString permalink;
    
    VTScanResult() : status(VT_ERROR), positives(0), total(0) {}
};

class VirusTotalClient : public TQObject
{
    TQ_OBJECT

public:
    VirusTotalClient(const TQString &apiKey, TQObject *parent = 0);
    ~VirusTotalClient();

    void scanHash(const TQString &hash);
    
signals:
    void scanResult(const VTScanResult &result);
    void error(const TQString &message);

private slots:
    void requestFinished();

private:
    TQString m_apiKey;
    
    // We will use a script or tdeio for simplicity
    class Private;
    Private *d;
};

#endif /* CLAM_VIRUSTOTAL_H */
