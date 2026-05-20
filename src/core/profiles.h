/*
 * profiles.h - Scan Profile management
 */

#ifndef CLAM_PROFILES_H
#define CLAM_PROFILES_H

#include <ntqobject.h>
#include <ntqstring.h>
#include <ntqstringlist.h>
#include <ntqvaluelist.h>

struct ScanProfile {
    TQString id;
    TQString name;
    TQString description;
    TQStringList paths;
    TQStringList exclusions;
    bool scanArchives;
    bool heuristicScan;
    
    ScanProfile() : scanArchives(true), heuristicScan(true) {}
};

class ProfileManager : public TQObject
{
    TQ_OBJECT

public:
    ProfileManager(TQObject *parent = 0);
    ~ProfileManager();

    void addProfile(const ScanProfile &profile);
    void updateProfile(const ScanProfile &profile);
    void removeProfile(const TQString &id);
    
    TQValueList<ScanProfile> getProfiles() const;
    ScanProfile getProfile(const TQString &id) const;

signals:
    void profilesUpdated();

private:
    TQString m_storageFile;
    TQValueList<ScanProfile> m_profiles;
    
    void loadProfiles();
    void saveProfiles();
    void createDefaultProfiles();
};

#endif /* CLAM_PROFILES_H */
