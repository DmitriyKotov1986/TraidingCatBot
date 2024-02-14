#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

#include "Common/common.h"

#include "config.h"

using namespace TraidingCatBot;
using namespace Common;

//static
static Config* config_ptr = nullptr;

Config* Config::config(const QString& configFileName)
{
    if (config_ptr == nullptr)
    {
        config_ptr = new Config(configFileName);
    }

    return config_ptr;
};

void Config::deleteConfig()
{
    Q_CHECK_PTR(config_ptr);

    if (config_ptr != nullptr)
    {
        delete config_ptr;

        config_ptr= nullptr;
    }
}

//class
Config::Config(const QString& configFileName) :
    _configFileName(configFileName)
{
    if (_configFileName.isEmpty())
    {
        _errorString = "Configuration file name cannot be empty";

        return;
    }
    if (!QFileInfo(_configFileName).exists())
    {
        _errorString = "Configuration file not exist. File name: " + _configFileName;

        return;
    }

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg("Reading configuration from " +  _configFileName);

    QSettings ini(_configFileName, QSettings::IniFormat);

    ini.beginGroup("STOCKEXCHANGES");

    _stockExcange_MEXC_enable = ini.value("MEXC_enable").toBool();

    ini.endGroup();
}

bool Config::save()
{
    QSettings ini(_configFileName, QSettings::IniFormat);

    if (!ini.isWritable()) {
        _errorString = "Can not write configuration file " +  _configFileName;

        return false;
    }

    ini.beginGroup("STOCKEXCHANGES");

    ini.setValue("MEXC_enable", _stockExcange_MEXC_enable);

    ini.endGroup();

    ini.sync();

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg("Save configuration to " +  _configFileName);

    return true;
}

bool Config::stockExcange_MEXC_enable() const
{
    return _stockExcange_MEXC_enable;
}

QString Config::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}


bool Config::isError() const
{
    return !_errorString.isEmpty();
}
