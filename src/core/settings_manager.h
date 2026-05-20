/*
 * settings_manager.h - KConfig-based Settings manager
 */

#ifndef CLAM_SETTINGS_MANAGER_H
#define CLAM_SETTINGS_MANAGER_H

#include <tdeconfig.h>
#include <ntqstring.h>
#include <ntqvariant.h>
#include <ntqobject.h>

class SettingsManager : public TQObject
{
    TQ_OBJECT

public:
    SettingsManager(TQObject *parent = 0);
    ~SettingsManager();

    TQString value(const TQString &key, const TQString &defaultValue = TQString::null) const;
    void setValue(const TQString &key, const TQString &value);
    
    int valueInt(const TQString &key, int defaultValue = 0) const;
    void setValueInt(const TQString &key, int value);
    
    bool valueBool(const TQString &key, bool defaultValue = false) const;
    void setValueBool(const TQString &key, bool value);

public slots:
    void save();
    void load();

signals:
    void settingsChanged();

private:
    TDEConfig *m_config;
};

#endif /* CLAM_SETTINGS_MANAGER_H */
