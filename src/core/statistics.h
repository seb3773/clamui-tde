/*
 * statistics.h - Statistics calculator
 */

#ifndef CLAM_STATISTICS_H
#define CLAM_STATISTICS_H

#include <ntqobject.h>
#include <ntqvaluelist.h>
#include <ntqstring.h>
#include "log_manager.h"

struct StatsData {
    int totalScans;
    int filesScanned;
    int threatsFound;
    int cleanScans;
    double avgDurationSeconds;
    
    StatsData() : totalScans(0), filesScanned(0), threatsFound(0), cleanScans(0), avgDurationSeconds(0.0) {}
};

struct TrendDataPoint {
    TQString label;
    double scans;
    double threats;
    
    TrendDataPoint() : scans(0.0), threats(0.0) {}
    TrendDataPoint(const TQString &l) : label(l), scans(0.0), threats(0.0) {}
};

class Statistics : public TQObject
{
    TQ_OBJECT

public:
    enum TimeFrame {
        TODAY,
        WEEK,
        MONTH,
        ALL_TIME
    };

    Statistics(LogManager *logManager, TQObject *parent = 0);
    ~Statistics();

    StatsData getStats(TimeFrame timeframe) const;
    TQValueList<TrendDataPoint> getTrendData(TimeFrame timeframe) const;
    bool isSystemProtected() const;
    TQString getLastScanTime() const;

private:
    LogManager *m_logManager;
};

#endif /* CLAM_STATISTICS_H */
