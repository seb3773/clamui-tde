/*
 * threat_classifier.cpp - Threat classification logic
 */

#include "threat_classifier.h"
#include <tdelocale.h>

ThreatSeverity ThreatClassifier::classifyThreat(const TQString &threatName)
{
    TQString lowerName = threatName.lower();
    
    // Critical Threats
    if (lowerName.contains("ransomware") || 
        lowerName.contains("rootkit") || 
        lowerName.contains("trojan") || 
        lowerName.contains("botnet") || 
        lowerName.contains("worm") ||
        lowerName.contains("backdoor") ||
        lowerName.contains("exploit")) {
        return SEVERITY_CRITICAL;
    }
    
    // High Threats
    if (lowerName.contains("virus") || 
        lowerName.contains("malware") || 
        lowerName.contains("spyware") || 
        lowerName.contains("keylogger") ||
        lowerName.contains("dropper") ||
        lowerName.contains("downloader") ||
        lowerName.contains("coinminer")) {
        return SEVERITY_HIGH;
    }
    
    // Medium Threats
    if (lowerName.contains("adware") || 
        lowerName.contains("pua") || 
        lowerName.contains("pup") || 
        lowerName.contains("riskware") ||
        lowerName.contains("hacktool") ||
        lowerName.contains("dialer") ||
        lowerName.contains("joke")) {
        return SEVERITY_MEDIUM;
    }
    
    // Low Threats
    if (lowerName.contains("tracking") || 
        lowerName.contains("cookie") || 
        lowerName.contains("eicar") ||
        lowerName.contains("phishing")) { // phishing might be high depending on context, but file-based is often low severity
        return SEVERITY_LOW;
    }
    
    // EICAR is often used for testing, categorize as LOW or UNKNOWN
    if (lowerName.contains("eicar")) {
        return SEVERITY_LOW;
    }

    // Default unknown threats
    return SEVERITY_UNKNOWN;
}

TQString ThreatClassifier::getThreatCategory(const TQString &threatName)
{
    TQString lowerName = threatName.lower();
    
    if (lowerName.contains("ransomware")) return i18n("Ransomware");
    if (lowerName.contains("trojan")) return i18n("Trojan");
    if (lowerName.contains("rootkit")) return i18n("Rootkit");
    if (lowerName.contains("worm")) return i18n("Worm");
    if (lowerName.contains("virus")) return i18n("Virus");
    if (lowerName.contains("adware")) return i18n("Adware");
    if (lowerName.contains("spyware")) return i18n("Spyware");
    if (lowerName.contains("pua") || lowerName.contains("pup")) return i18n("Potentially Unwanted Program (PUA/PUP)");
    if (lowerName.contains("coinminer")) return i18n("Cryptominer");
    if (lowerName.contains("exploit")) return i18n("Exploit");
    if (lowerName.contains("eicar")) return i18n("Test File (EICAR)");

    // Generic
    return i18n("Generic Malware");
}

TQString ThreatClassifier::getSeverityString(ThreatSeverity severity)
{
    switch (severity) {
        case SEVERITY_CRITICAL: return i18n("Critical");
        case SEVERITY_HIGH: return i18n("High");
        case SEVERITY_MEDIUM: return i18n("Medium");
        case SEVERITY_LOW: return i18n("Low");
        case SEVERITY_UNKNOWN:
        default: return i18n("Unknown");
    }
}
