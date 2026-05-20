/*
 * log_manager.cpp - Log manager implementation
 *
 * Storage layout:
 *   logs/index.json   — small array of metadata (id, timestamp, type, status, summary, duration)
 *   logs/{id}.log     — full details text for each entry (loaded on demand)
 */

#include "log_manager.h"
#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <ntqfile.h>
#include <ntqdir.h>
#include <ntqtextstream.h>
#include <ntquuid.h>
#include <tdeconfig.h>
#include "../libs/json/tqtjson.h"

LogManager::LogManager(TQObject *parent)
    : TQObject(parent)
{
    initStorage();
    loadIndex();
}

LogManager::~LogManager()
{
}

void LogManager::initStorage()
{
    m_logDir = TDEGlobal::dirs()->saveLocation("appdata", "logs");
    TQDir dir(m_logDir);
    if (!dir.exists()) {
        dir.mkdir(m_logDir);
    }
    
    // Remove legacy monolithic history.json if present (old format, not compatible)
    TQString oldFile = m_logDir + "history.json";
    if (TQFile::exists(oldFile)) {
        TQFile::remove(oldFile);
    }
}

TQString LogManager::buildSummary(int type, int status, const TQString &details)
{
    if (type == 0) { // Scan
        if (status == 0) return "Clean scan";
        if (status == 1) {
            int idx = details.find("Infected files: ");
            if (idx != -1) {
                int endIdx = details.find('\n', idx);
                TQString val = details.mid(idx + 16, endIdx - (idx + 16)).stripWhiteSpace();
                int count = val.toInt();
                if (count > 0) return TQString("Found %1 threat(s)").arg(count);
            }
            return "Threats found";
        }
        if (details.contains("cancelled", false)) return "Scan cancelled";
        return "Scan error";
    } else if (type == 1) { // Update
        return status == 0 ? "Database update completed" : "Database update failed";
    } else if (type == 2) { // Audit
        if (status == 0) return "System is secure";
        if (status == 1) return "Security warnings found";
        return "Security vulnerabilities found";
    }
    return "Operation completed";
}

void LogManager::addLog(int type, int status, const TQString &details, double duration)
{
    LogEntry entry;
    entry.id = TQUuid::createUuid().toString();
    entry.timestamp = TQDateTime::currentDateTime();
    entry.type = type;
    entry.status = status;
    entry.summary = buildSummary(type, status, details);
    entry.duration = duration;
    
    // Extract numeric stats from details for statistics module
    entry.filesScanned = 0;
    entry.threatsFound = 0;
    if (type == 0) { // Scan
        int fIdx = details.find("Scanned files: ");
        if (fIdx != -1) {
            int fEnd = details.find('\n', fIdx);
            entry.filesScanned = details.mid(fIdx + 15, fEnd - (fIdx + 15)).stripWhiteSpace().toInt();
        }
        int iIdx = details.find("Infected files: ");
        if (iIdx != -1) {
            int iEnd = details.find('\n', iIdx);
            entry.threatsFound = details.mid(iIdx + 16, iEnd - (iIdx + 16)).stripWhiteSpace().toInt();
        }
    }
    
    m_logs.prepend(entry); // Newest first
    
    // Keep max 1000 entries
    while (m_logs.count() > 1000) {
        // Remove the detail file of the oldest entry
        TQString oldId = m_logs.last().id;
        TQFile::remove(m_logDir + oldId + ".log");
        m_logs.pop_back();
    }
    
    // Write full details to individual file
    TQFile f(m_logDir + entry.id + ".log");
    if (f.open(IO_WriteOnly)) {
        TQTextStream stream(&f);
        stream << details;
        f.close();
    }
    
    saveIndex();
    emit logsUpdated();
}

TQValueList<LogEntry> LogManager::getLogs(int type) const
{
    if (type == -1) return m_logs;
    
    TQValueList<LogEntry> filtered;
    for (TQValueList<LogEntry>::ConstIterator it = m_logs.begin(); it != m_logs.end(); ++it) {
        if ((*it).type == type) {
            filtered.append(*it);
        }
    }
    return filtered;
}

TQString LogManager::getLogDetails(const TQString &id) const
{
    TQString filePath = m_logDir + id + ".log";
    TQFile f(filePath);
    if (f.open(IO_ReadOnly)) {
        TQTextStream stream(&f);
        TQString content = stream.read();
        f.close();
        return content;
    }
    return TQString::null;
}

void LogManager::clearLogs()
{
    // Remove all detail files
    for (TQValueList<LogEntry>::ConstIterator it = m_logs.begin(); it != m_logs.end(); ++it) {
        TQFile::remove(m_logDir + (*it).id + ".log");
    }
    
    m_logs.clear();
    saveIndex();
    emit logsUpdated();
}

void LogManager::writeDebug(const TQString &message)
{
    TDEConfig config("tdeclamuirc");
    config.setGroup("General");
    if (!config.readBoolEntry("debug_logging", false)) {
        return;
    }
    
    TQString debugFile = m_logDir + "debug.log";
    TQFile f(debugFile);
    if (f.open(IO_WriteOnly | IO_Append)) {
        TQTextStream stream(&f);
        stream << TQDateTime::currentDateTime().toString(TQt::ISODate) << " | " << message << "\n";
        f.close();
    }
}

void LogManager::loadIndex()
{
    m_logs.clear();
    
    TQString file = m_logDir + "index.json";
    if (!TQFile::exists(file)) return;
    
    TQString jsonStr;
    TQFile f(file);
    if (f.open(IO_ReadOnly)) {
        TQTextStream stream(&f);
        jsonStr = stream.read();
        f.close();
    }
    
    TQtJsonDocument doc = TQtJsonDocument::fromJson(jsonStr);
    if (!doc.isArray()) return;
    
    TQtJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        TQtJsonObject obj = arr.at(i).toObject();
        LogEntry entry;
        entry.id = obj.value("id").toString();
        entry.timestamp = TQDateTime::fromString(obj.value("timestamp").toString(), TQt::ISODate);
        entry.type = obj.value("type").toDouble();
        entry.status = obj.value("status").toDouble();
        entry.summary = obj.value("summary").toString();
        entry.duration = obj.value("duration").toDouble();
        entry.filesScanned = (int)obj.value("filesScanned").toDouble();
        entry.threatsFound = (int)obj.value("threatsFound").toDouble();
        m_logs.append(entry);
    }
}

void LogManager::saveIndex()
{
    TQValueList<TQVariant> arr;
    for (TQValueList<LogEntry>::ConstIterator it = m_logs.begin(); it != m_logs.end(); ++it) {
        TQMap<TQString, TQVariant> obj;
        obj.insert("id", (*it).id);
        obj.insert("timestamp", (*it).timestamp.toString(TQt::ISODate));
        obj.insert("type", (*it).type);
        obj.insert("status", (*it).status);
        obj.insert("summary", (*it).summary);
        obj.insert("duration", (*it).duration);
        obj.insert("filesScanned", (*it).filesScanned);
        obj.insert("threatsFound", (*it).threatsFound);
        arr.append(TQVariant(obj));
    }
    
    TQtJsonDocument doc = TQtJsonDocument::fromValue(TQtJsonValue::fromVariant(arr));
    TQString file = m_logDir + "index.json";
    TQFile f(file);
    if (f.open(IO_WriteOnly)) {
        TQTextStream stream(&f);
        stream << doc.toJson(true);
        f.close();
    }
}

#include "log_manager.moc"
