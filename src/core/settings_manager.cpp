/*
 * settings_manager.cpp - KConfig-based Settings manager implementation
 */

#include "settings_manager.h"

SettingsManager::SettingsManager(TQObject *parent)
    : TQObject(parent)
{
    m_config = new TDEConfig("tdeclamuirc");
    load();
}

SettingsManager::~SettingsManager()
{
    save();
    delete m_config;
}

void SettingsManager::load()
{
    m_config->reparseConfiguration();
}

void SettingsManager::save()
{
    m_config->sync();
    emit settingsChanged();
}

TQString SettingsManager::value(const TQString &key, const TQString &defaultValue) const
{
    m_config->setGroup("General");
    return m_config->readEntry(key, defaultValue);
}

void SettingsManager::setValue(const TQString &key, const TQString &value)
{
    m_config->setGroup("General");
    m_config->writeEntry(key, value);
}

int SettingsManager::valueInt(const TQString &key, int defaultValue) const
{
    m_config->setGroup("General");
    return m_config->readNumEntry(key, defaultValue);
}

void SettingsManager::setValueInt(const TQString &key, int value)
{
    m_config->setGroup("General");
    m_config->writeEntry(key, value);
}

bool SettingsManager::valueBool(const TQString &key, bool defaultValue) const
{
    m_config->setGroup("General");
    return m_config->readBoolEntry(key, defaultValue);
}

void SettingsManager::setValueBool(const TQString &key, bool value)
{
    m_config->setGroup("General");
    m_config->writeEntry(key, value);
}

#include "settings_manager.moc"
