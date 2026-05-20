/*
 * quarantine_manager.cpp - Quarantine Manager implementation
 */

#include "quarantine_manager.h"
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <ntqdir.h>
#include <ntqfile.h>
#include <openssl/sha.h>
#include <tdelocale.h>
#include "settings_manager.h"

QuarantineManager::QuarantineManager(TQObject *parent)
    : TQObject(parent), m_db(0)
{
    SettingsManager settings;
    TQString appDataDir = TDEGlobal::dirs()->saveLocation("appdata");
    TQString defaultQuar = appDataDir + "quarantine";
    
    m_dbPath = appDataDir + "quarantine.db";
    m_storageDir = settings.value("quarantine_path", defaultQuar);
    
    if (!m_storageDir.endsWith("/")) {
        m_storageDir += "/";
    }
    
    TQDir dir(m_storageDir);
    if (!dir.exists()) {
        dir.mkdir(m_storageDir);
    }
    
    initDatabase();
}

QuarantineManager::~QuarantineManager()
{
    if (m_db) {
        sqlite3_close(m_db);
    }
}

bool QuarantineManager::initDatabase()
{
    if (sqlite3_open(m_dbPath.local8Bit(), &m_db) != SQLITE_OK) {
        emit error(i18n("Failed to open quarantine database."));
        return false;
    }
    
    const char *sql = "CREATE TABLE IF NOT EXISTS quarantine ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "original_path TEXT NOT NULL,"
                      "threat_name TEXT NOT NULL,"
                      "detection_date DATETIME DEFAULT CURRENT_TIMESTAMP,"
                      "file_size INTEGER,"
                      "sha256 TEXT NOT NULL UNIQUE"
                      ");";
                      
    char *errMsg = 0;
    if (sqlite3_exec(m_db, sql, 0, 0, &errMsg) != SQLITE_OK) {
        emit error(TQString(i18n("Database initialization error: %1")).arg(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

TQString QuarantineManager::calculateHash(const TQString &filePath) const
{
    TQFile file(filePath);
    if (!file.open(IO_ReadOnly)) return TQString::null;
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    char buffer[8192];
    int bytesRead;
    while ((bytesRead = file.readBlock(buffer, sizeof(buffer))) > 0) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    TQString result;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        TQString hex;
        hex.sprintf("%02x", hash[i]);
        result += hex;
    }
    return result;
}

TQString QuarantineManager::secureMove(const TQString &source, const TQString &hash) const
{
    TQString destPath = m_storageDir + hash;
    // Simple move for now. Ideally this would XOR the file.
    if (copyFile(source, destPath)) {
        TQFile::remove(source);
        return destPath;
    }
    return TQString::null;
}

bool QuarantineManager::copyFile(const TQString &src, const TQString &dest) const
{
    TQFile srcFile(src);
    TQFile destFile(dest);
    
    if (!srcFile.open(IO_ReadOnly)) return false;
    if (!destFile.open(IO_WriteOnly)) return false;
    
    char buffer[8192];
    int bytesRead;
    while ((bytesRead = srcFile.readBlock(buffer, sizeof(buffer))) > 0) {
        if (destFile.writeBlock(buffer, bytesRead) != bytesRead) {
            return false;
        }
    }
    return true;
}

bool QuarantineManager::quarantineFile(const TQString &filePath, const TQString &threatName)
{
    TQFile file(filePath);
    if (!file.exists()) return false;
    
    TQ_LONG size = file.size();
    TQString hash = calculateHash(filePath);
    if (hash.isEmpty()) return false;
    
    TQString storedPath = secureMove(filePath, hash);
    if (storedPath.isEmpty()) {
        emit error(i18n("Failed to move file to quarantine."));
        return false;
    }
    
    const char *sql = "INSERT INTO quarantine (original_path, threat_name, detection_date, file_size, sha256) "
                      "VALUES (?, ?, datetime('now'), ?, ?)";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.local8Bit(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, threatName.local8Bit(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, size);
        sqlite3_bind_text(stmt, 4, hash.local8Bit(), -1, SQLITE_TRANSIENT);
        
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            emit error(i18n("Failed to record quarantine entry in database."));
            return false;
        }
        
        emit quarantineUpdated();
        return true;
    }
    
    return false;
}

TQValueList<QuarantineEntry> QuarantineManager::getEntries() const
{
    TQValueList<QuarantineEntry> entries;
    const char *sql = "SELECT id, original_path, threat_name, detection_date, file_size, sha256 FROM quarantine ORDER BY detection_date DESC";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            QuarantineEntry entry;
            entry.id = sqlite3_column_int(stmt, 0);
            entry.originalPath = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 1));
            entry.threatName = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 2));
            
            // Format datetime directly or let caller format
            entry.detectionDate = TQDateTime::fromString(
                TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 3)).replace(" ", "T"),
                TQt::ISODate
            );
            
            entry.fileSize = sqlite3_column_int64(stmt, 4);
            entry.sha256 = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 5));
            entries.append(entry);
        }
        sqlite3_finalize(stmt);
    }
    return entries;
}

bool QuarantineManager::restoreFile(int id)
{
    const char *sql = "SELECT original_path, sha256 FROM quarantine WHERE id = ?";
    sqlite3_stmt *stmt;
    bool success = false;
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            TQString original = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 0));
            TQString hash = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 1));
            
            TQString storedPath = m_storageDir + hash;
            TQFile stored(storedPath);
            if (stored.exists() && copyFile(storedPath, original)) {
                stored.remove();
                
                // Delete from DB
                const char *delSql = "DELETE FROM quarantine WHERE id = ?";
                sqlite3_stmt *delStmt;
                if (sqlite3_prepare_v2(m_db, delSql, -1, &delStmt, 0) == SQLITE_OK) {
                    sqlite3_bind_int(delStmt, 1, id);
                    sqlite3_step(delStmt);
                    sqlite3_finalize(delStmt);
                }
                success = true;
                emit quarantineUpdated();
            }
        }
        sqlite3_finalize(stmt);
    }
    return success;
}

bool QuarantineManager::deleteFile(int id)
{
    const char *sql = "SELECT sha256 FROM quarantine WHERE id = ?";
    sqlite3_stmt *stmt;
    bool success = false;
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            TQString hash = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 0));
            TQFile::remove(m_storageDir + hash);
            
            const char *delSql = "DELETE FROM quarantine WHERE id = ?";
            sqlite3_stmt *delStmt;
            if (sqlite3_prepare_v2(m_db, delSql, -1, &delStmt, 0) == SQLITE_OK) {
                sqlite3_bind_int(delStmt, 1, id);
                sqlite3_step(delStmt);
                sqlite3_finalize(delStmt);
            }
            success = true;
            emit quarantineUpdated();
        }
        sqlite3_finalize(stmt);
    }
    return success;
}

void QuarantineManager::clearOldEntries(int daysOld)
{
    TQString dateLimit = TQDateTime::currentDateTime().addDays(-daysOld).toString(TQt::ISODate).replace("T", " ");
    const char *sql = "SELECT id, sha256 FROM quarantine WHERE detection_date < ?";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dateLimit.local8Bit(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            TQString hash = TQString::fromLocal8Bit((const char *)sqlite3_column_text(stmt, 1));
            TQFile::remove(m_storageDir + hash);
            
            const char *delSql = "DELETE FROM quarantine WHERE id = ?";
            sqlite3_stmt *delStmt;
            if (sqlite3_prepare_v2(m_db, delSql, -1, &delStmt, 0) == SQLITE_OK) {
                sqlite3_bind_int(delStmt, 1, id);
                sqlite3_step(delStmt);
                sqlite3_finalize(delStmt);
            }
        }
        sqlite3_finalize(stmt);
        emit quarantineUpdated();
    }
}

#include "quarantine_manager.moc"
