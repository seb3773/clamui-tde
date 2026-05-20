/*
 * threat_classifier.h - Threat classification logic
 */

#ifndef CLAM_THREAT_CLASSIFIER_H
#define CLAM_THREAT_CLASSIFIER_H

#include <ntqstring.h>

enum ThreatSeverity {
    SEVERITY_CRITICAL,
    SEVERITY_HIGH,
    SEVERITY_MEDIUM,
    SEVERITY_LOW,
    SEVERITY_UNKNOWN
};

class ThreatClassifier
{
public:
    static ThreatSeverity classifyThreat(const TQString &threatName);
    static TQString getThreatCategory(const TQString &threatName);
    static TQString getSeverityString(ThreatSeverity severity);
};

#endif /* CLAM_THREAT_CLASSIFIER_H */
