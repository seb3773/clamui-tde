/*
 * system_audit.h - System security audit checks
 */

#ifndef CLAM_SYSTEM_AUDIT_H
#define CLAM_SYSTEM_AUDIT_H

#include <ntqobject.h>
#include <ntqstring.h>

enum AuditStatus {
    AuditPass = 0,
    AuditWarning = 1,
    AuditFail = 2,
    AuditUnknown = 3,
    AuditSkipped = 4,
    AuditChecking = 5
};

struct AuditCheckResult {
    TQString name;
    AuditStatus status;
    TQString detail;
    TQString recommendation;
    TQString install_command;
    TQString info_url;
    TQString launch_command;
    TQString launch_label;
};

struct AuditSectionResult {
    TQString category;
    TQString title;
    TQString icon_name;
    TQValueList<AuditCheckResult> checks;
    
    AuditStatus overallStatus() const {
        if (checks.isEmpty()) return AuditUnknown;
        AuditStatus worst = AuditPass;
        for (TQValueList<AuditCheckResult>::ConstIterator it = checks.begin(); it != checks.end(); ++it) {
            if ((*it).status == AuditFail) return AuditFail;
            if ((*it).status == AuditWarning) worst = AuditWarning;
            else if ((*it).status == AuditUnknown && worst == AuditPass) worst = AuditUnknown;
        }
        return worst;
    }
};

struct AuditReport {
    TQValueList<AuditSectionResult> sections;
    double timestamp;
};

class SystemAudit : public TQObject
{
    TQ_OBJECT

public:
    SystemAudit(TQObject *parent = 0);
    ~SystemAudit();

    AuditSectionResult checkClamAVHealth() const;
    AuditSectionResult checkFirewall() const;
    AuditSectionResult checkMacFramework() const;
    AuditSectionResult checkAutoUpdates() const;
    AuditSectionResult checkIntrusionDetection() const;
    AuditSectionResult checkSSHHardening() const;
    AuditSectionResult checkHomePermissions() const;
    AuditSectionResult checkKernelHardening() const;
    AuditSectionResult checkPasswordPolicy() const;
    AuditSectionResult checkTDEHealth() const;

    // Deep scans
    void runLynisAudit();
    void runRootkitCheck();
    bool isBinaryAvailable(const TQString &binary) const;
    
    // Serialization
    void saveAuditReport(const AuditReport &report);
    AuditReport loadAuditReport() const;
    
signals:
    void auditProgress(const TQString &message);
    void auditComplete(const AuditReport &report);
    void sectionComplete(const AuditSectionResult &section);
};

#endif /* CLAM_SYSTEM_AUDIT_H */
