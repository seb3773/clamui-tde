/*
 * scheduler.h - Scheduled scans manager
 */

#ifndef CLAM_SCHEDULER_H
#define CLAM_SCHEDULER_H

#include <ntqobject.h>
#include <ntqstring.h>

class Scheduler : public TQObject
{
    TQ_OBJECT

public:
    Scheduler(TQObject *parent = 0);
    ~Scheduler();

    bool isSystemdAvailable() const;
    bool isCronAvailable() const;
    
    bool createSchedule(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery = false);
    bool removeSchedule();
    bool isScheduled() const;

private:
    bool createSystemdTimer(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery);
    bool removeSystemdTimer();
    
    bool createCronJob(const TQString &frequency, const TQString &time, const TQString &profileId, bool skipOnBattery);
    bool removeCronJob();
    
    TQString buildCommand(const TQString &profileId) const;
};

#endif /* CLAM_SCHEDULER_H */
