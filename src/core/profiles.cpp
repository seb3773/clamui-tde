/*
 * profiles.cpp - Scan Profile management implementation
 */

#include "profiles.h"
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <tdelocale.h>
#include <ntqfile.h>
#include <ntqdir.h>
#include <ntqtextstream.h>
#include <ntquuid.h>
#include "../libs/json/tqtjson.h"

ProfileManager::ProfileManager(TQObject *parent)
    : TQObject(parent)
{
    m_storageFile = TDEGlobal::dirs()->saveLocation("appdata") + "profiles.json";
    loadProfiles();
    if (m_profiles.isEmpty()) {
        createDefaultProfiles();
    }
}

ProfileManager::~ProfileManager()
{
}

void ProfileManager::createDefaultProfiles()
{
    ScanProfile quick;
    quick.id = "quick-scan-profile";
    quick.name = i18n("System Scan");
    quick.description = i18n("Scans common system directories for threats.");
    quick.paths << "/bin" << "/sbin" << "/usr/bin" << "/usr/sbin" << "/etc" << "/lib" << "/tmp";
    addProfile(quick);
    
    ScanProfile full;
    full.id = "full-scan-profile";
    full.name = i18n("Full Scan");
    full.description = i18n("Scans the entire filesystem. May take a long time.");
    full.paths << "/";
    full.exclusions << "/dev" << "/proc" << "/sys" << "/run" << "/mnt" << "/media";
    addProfile(full);
    
    ScanProfile home;
    home.id = "home-scan-profile";
    home.name = i18n("Home Folder Scan");
    home.description = i18n("Scans your personal files.");
    home.paths << TQDir::homeDirPath();
    addProfile(home);
}

void ProfileManager::loadProfiles()
{
    m_profiles.clear();
    
    TQFile f(m_storageFile);
    if (!f.open(IO_ReadOnly)) return;
    
    TQTextStream stream(&f);
    TQString jsonStr = stream.read();
    f.close();
    
    TQtJsonDocument doc = TQtJsonDocument::fromJson(jsonStr);
    if (!doc.isArray()) return;
    
    TQtJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        TQtJsonObject obj = arr.at(i).toObject();
        ScanProfile p;
        p.id = obj.value("id").toString();
        p.name = obj.value("name").toString();
        p.description = obj.value("description").toString();
        
        TQtJsonArray pathsArr = obj.value("paths").toArray();
        for (int j = 0; j < pathsArr.size(); ++j) {
            p.paths.append(pathsArr.at(j).toString());
        }
        
        TQtJsonArray exclArr = obj.value("exclusions").toArray();
        for (int j = 0; j < exclArr.size(); ++j) {
            p.exclusions.append(exclArr.at(j).toString());
        }
        
        p.scanArchives = obj.value("scanArchives").toBool(true);
        p.heuristicScan = obj.value("heuristicScan").toBool(true);
        
        m_profiles.append(p);
    }
}

void ProfileManager::saveProfiles()
{
    TQValueList<TQVariant> arr;
    
    for (TQValueList<ScanProfile>::ConstIterator it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        TQMap<TQString, TQVariant> obj;
        obj.insert("id", (*it).id);
        obj.insert("name", (*it).name);
        obj.insert("description", (*it).description);
        
        TQValueList<TQVariant> paths;
        for (TQStringList::ConstIterator pit = (*it).paths.begin(); pit != (*it).paths.end(); ++pit) {
            paths.append(*pit);
        }
        obj.insert("paths", TQVariant(paths));
        
        TQValueList<TQVariant> exclusions;
        for (TQStringList::ConstIterator eit = (*it).exclusions.begin(); eit != (*it).exclusions.end(); ++eit) {
            exclusions.append(*eit);
        }
        obj.insert("exclusions", TQVariant(exclusions));
        
        obj.insert("scanArchives", (*it).scanArchives);
        obj.insert("heuristicScan", (*it).heuristicScan);
        
        arr.append(TQVariant(obj));
    }
    
    TQtJsonDocument doc = TQtJsonDocument::fromValue(TQtJsonValue::fromVariant(arr));
    
    TQFile f(m_storageFile);
    if (f.open(IO_WriteOnly)) {
        TQTextStream stream(&f);
        stream << doc.toJson(true);
        f.close();
    }
}

void ProfileManager::addProfile(const ScanProfile &profile)
{
    ScanProfile p = profile;
    if (p.id.isEmpty()) {
        p.id = TQUuid::createUuid().toString();
    }
    m_profiles.append(p);
    saveProfiles();
    emit profilesUpdated();
}

void ProfileManager::updateProfile(const ScanProfile &profile)
{
    for (TQValueList<ScanProfile>::Iterator it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if ((*it).id == profile.id) {
            *it = profile;
            saveProfiles();
            emit profilesUpdated();
            return;
        }
    }
}

void ProfileManager::removeProfile(const TQString &id)
{
    for (TQValueList<ScanProfile>::Iterator it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if ((*it).id == id) {
            m_profiles.remove(it);
            saveProfiles();
            emit profilesUpdated();
            return;
        }
    }
}

TQValueList<ScanProfile> ProfileManager::getProfiles() const
{
    return m_profiles;
}

ScanProfile ProfileManager::getProfile(const TQString &id) const
{
    for (TQValueList<ScanProfile>::ConstIterator it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if ((*it).id == id) {
            return *it;
        }
    }
    return ScanProfile();
}

#include "profiles.moc"
