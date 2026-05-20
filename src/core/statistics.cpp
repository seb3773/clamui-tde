/*
 * statistics.cpp - Statistics calculator implementation
 */

#include "statistics.h"
#include <ntqdatetime.h>
#include <ntqregexp.h>
#include <tdelocale.h>

Statistics::Statistics(LogManager *logManager, TQObject *parent)
    : TQObject(parent), m_logManager(logManager)
{
}

Statistics::~Statistics()
{
}

StatsData Statistics::getStats(TimeFrame timeframe) const
{
    StatsData data;
    if (!m_logManager) return data;
    
    TQValueList<LogEntry> logs = m_logManager->getLogs(0); // 0 = Scan logs
    TQDateTime now = TQDateTime::currentDateTime();
    TQDateTime limit;
    
    switch (timeframe) {
        case TODAY:
            limit = now.addDays(-1);
            break;
        case WEEK:
            limit = now.addDays(-7);
            break;
        case MONTH:
            limit = now.addMonths(-1);
            break;
        case ALL_TIME:
        default:
            limit = TQDateTime(TQDate(1970, 1, 1));
            break;
    }
    
    double totalDuration = 0.0;
    
    for (TQValueList<LogEntry>::ConstIterator it = logs.begin(); it != logs.end(); ++it) {
        if ((*it).timestamp < limit) continue;
        
        data.totalScans++;
        totalDuration += (*it).duration;
        data.filesScanned += (*it).filesScanned;
        
        if ((*it).status == 0) {
            data.cleanScans++;
        } else if ((*it).status == 1) { // infected
            data.threatsFound += (*it).threatsFound > 0 ? (*it).threatsFound : 1;
        }
    }
    
    if (data.totalScans > 0) {
        data.avgDurationSeconds = totalDuration / data.totalScans;
    }
    
    return data;
}

bool Statistics::isSystemProtected() const
{
    // Check if scans ran recently, updates ran, etc.
    if (!m_logManager) return false;
    
    TQValueList<LogEntry> scans = m_logManager->getLogs(0);
    TQValueList<LogEntry> updates = m_logManager->getLogs(1); // 1 = Update
    
    TQDateTime now = TQDateTime::currentDateTime();
    bool recentScan = false;
    bool recentUpdate = false;
    
    if (!scans.isEmpty() && scans.first().timestamp.daysTo(now) < 7) {
        recentScan = true;
    }
    
    if (!updates.isEmpty() && updates.first().timestamp.daysTo(now) < 3) {
        recentUpdate = true;
    }
    
    return recentScan && recentUpdate;
}

TQValueList<TrendDataPoint> Statistics::getTrendData(TimeFrame timeframe) const
{
    TQValueList<TrendDataPoint> trend;
    if (!m_logManager) return trend;
    
    TQValueList<LogEntry> logs = m_logManager->getLogs(0); // Scans
    TQDateTime now = TQDateTime::currentDateTime();
    TQDateTime limit;
    
    int numBuckets = 7;
    int daysPerBucket = 1;
    TQString formatStr = "ddd"; // e.g. Mon, Tue
    
    switch (timeframe) {
        case TODAY:
            // Group by hours or just single day? 
            // We'll just return 1 bucket for today, or group by every 4 hours. Let's do 6 buckets of 4 hours
            numBuckets = 6;
            formatStr = "hh:00";
            limit = now.addDays(-1);
            break;
        case WEEK:
            numBuckets = 7;
            daysPerBucket = 1;
            formatStr = "ddd";
            limit = now.addDays(-7);
            break;
        case MONTH:
            numBuckets = 4;
            daysPerBucket = 7;
            formatStr = "dd MMM";
            limit = now.addMonths(-1);
            break;
        case ALL_TIME:
        default:
            numBuckets = 6;
            daysPerBucket = 30; // ~6 months
            formatStr = "MMM yyyy";
            limit = now.addMonths(-6);
            break;
    }
    
    // Initialize buckets
    TQMap<int, TrendDataPoint> buckets;
    for (int i = 0; i < numBuckets; ++i) {
        TrendDataPoint pt;
        if (timeframe == TODAY) {
            pt.label = now.addSecs(- (numBuckets - 1 - i) * 4 * 3600).toString(formatStr);
        } else {
            pt.label = now.addDays(- (numBuckets - 1 - i) * daysPerBucket).toString(formatStr);
        }
        buckets.insert(i, pt);
    }
    
    // Populate
    for (TQValueList<LogEntry>::ConstIterator it = logs.begin(); it != logs.end(); ++it) {
        if ((*it).timestamp < limit) continue;
        
        int bucketIndex = 0;
        if (timeframe == TODAY) {
            int secsDiff = (*it).timestamp.secsTo(now);
            bucketIndex = numBuckets - 1 - (secsDiff / (4 * 3600));
        } else {
            int daysDiff = (*it).timestamp.daysTo(now);
            bucketIndex = numBuckets - 1 - (daysDiff / daysPerBucket);
        }
        
        if (bucketIndex >= 0 && bucketIndex < numBuckets) {
            buckets[bucketIndex].scans++;
            if ((*it).status == 1) { // infected
                buckets[bucketIndex].threats++;
            }
        }
    }
    
    for (int i = 0; i < numBuckets; ++i) {
        trend.append(buckets[i]);
    }
    
    return trend;
}

TQString Statistics::getLastScanTime() const
{
    if (!m_logManager) return i18n("Never");
    
    TQValueList<LogEntry> logs = m_logManager->getLogs(0); // Scans
    if (logs.isEmpty()) return i18n("Never");
    
    LogEntry latest = logs.first(); // Assuming prepended
    TQDateTime now = TQDateTime::currentDateTime();
    int days = latest.timestamp.daysTo(now);
    
    TQString relStr;
    if (days == 0) relStr = i18n("Today");
    else if (days == 1) relStr = i18n("1 day ago");
    else relStr = i18n("%1 days ago").arg(days);
    
    return latest.timestamp.toString(TQt::ISODate) + " (" + relStr + ")";
}

#include "statistics.moc"
