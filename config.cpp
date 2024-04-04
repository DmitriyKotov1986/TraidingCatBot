#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

#include "Common/common.h"

#include "config.h"

using namespace TradingCat;
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

    QStringList groups = ini.childGroups();
    if (!groups.contains("DATABASE"))
    {
        _errorString = "Configuration file not contains [DATABASE] group";

        return;
    }
    if (!groups.contains("SERVER"))
    {
        _errorString = "Configuration file not contains [SERVER] group";

        return;
    }
    if (!groups.contains("STOCK_EXCHANGES"))
    {
        _errorString = "Configuration file not contains [STOCK_EXCHANGES] group";

        return;
    }

    //Database
    ini.beginGroup("DATABASE");

    _db_ConnectionInfo.db_Driver = ini.value("Driver", "QODBC").toString();
    _db_ConnectionInfo.db_DBName = ini.value("DataBase", "").toString();
    _db_ConnectionInfo.db_UserName = ini.value("UID", "").toString();
    _db_ConnectionInfo.db_Password = ini.value("PWD", "").toString();
    _db_ConnectionInfo.db_ConnectOptions = ini.value("ConnectionOprions", "").toString();
    _db_ConnectionInfo.db_Port = ini.value("Port", "").toUInt();
    _db_ConnectionInfo.db_Host = ini.value("Host", "localhost").toString();

    ini.endGroup();

    //Database
    ini.beginGroup("SERVER");

    const auto address_str =ini.value("Address", "localhost").toString();
    _server_config.address = QHostAddress(address_str);
    if (_server_config.address.isNull())
    {
        _errorString = QString("Invalid value in [SERVER]/Address. Value: %1").arg(address_str);

        return;
    }
    _server_config.port = ini.value("Port", 0).toUInt();
    if (_server_config.port == 0)
    {
        _errorString = QString("Value in [SERVER]/Port must be number");

        return;
    }
    _server_config.maxUsers = ini.value("MaxUsers", 100).toUInt();
    if (_server_config.maxUsers == 0)
    {
        _errorString = QString("Value in [SERVER]/MaxUsers must be number");

        return;
    }
    _server_config.rootDir = ini.value("RootDir", ".").toString();
    _server_config.crtFileName = ini.value("CRTFileName", "").toString();
    _server_config.keyFileName = ini.value("KEYFileName", "").toString();

    ini.endGroup();

    //STOCK_EXCHANGES
    ini.beginGroup("STOCK_EXCHANGES");

    _stockExcange_MEXC_enable = ini.value("MEXC_enable").toBool();
    _stockExcange_KUCOIN_enable = ini.value("KUCOIN_enable").toBool();
    _stockExcange_GATE_enable = ini.value("GATE_enable").toBool();
    _stockExcange_BYBIT_enable = ini.value("BYBIT_enable").toBool();

    ini.endGroup();

    //PROXY_N
    quint16 currentProxyIndex = 0;
    for (const auto& group: groups)
    {
        if (group == QString("PROXY_%1").arg(currentProxyIndex))
        {
            ini.beginGroup(group);

            Common::ProxyInfo tmp;
            tmp.proxy_host = ini.value("Host", "localhost").toString();
            if (tmp.proxy_host.isEmpty())
            {
                _errorString = QString("Value in [%1]/Host is empty").arg(group);

                return;
            }

            bool ok = false;
            tmp.proxy_port = ini.value("Port", 51888).toUInt(&ok);
            if (_server_config.maxUsers == 0)
            {
                _errorString = QString("Value in [%1]/Port must be number").arg(group);

                return;
            }

            tmp.proxy_user = ini.value("User").toString();
            tmp.proxy_password = ini.value("Password").toString();

            ini.endGroup();

            _proxy_info_list.push_back(tmp);

            ++currentProxyIndex;
        }
    }
}

bool Config::save()
{
    QSettings ini(_configFileName, QSettings::IniFormat);

    if (!ini.isWritable()) {
        _errorString = "Can not write configuration file " +  _configFileName;

        return false;
    }

    //Database
    ini.beginGroup("DATABASE");

    ini.remove("");

    ini.setValue("Driver", _db_ConnectionInfo.db_Driver);
    ini.setValue("DataBase", _db_ConnectionInfo.db_DBName);
    ini.setValue("UID", _db_ConnectionInfo.db_UserName);
    ini.setValue("PWD", _db_ConnectionInfo.db_Password);
    ini.setValue("ConnectionOptions", _db_ConnectionInfo.db_ConnectOptions);
    ini.setValue("Port", _db_ConnectionInfo.db_Port);
    ini.setValue("Host", _db_ConnectionInfo.db_Host);

    ini.endGroup();

    //Server
    ini.beginGroup("SERVER");

    ini.remove("");

    ini.setValue("Address", _server_config.address.toString());
    ini.setValue("Port", _server_config.port);
    ini.setValue("MaxUsers", _server_config.maxUsers);
    ini.setValue("RootDir", _server_config.rootDir);

    ini.endGroup();

    //STOCK_EXCHANGES
    ini.beginGroup("STOCK_EXCHANGES");

    ini.remove("");

    ini.setValue("MEXC_enable", _stockExcange_MEXC_enable);
    ini.setValue("KUCOIN_enable", _stockExcange_KUCOIN_enable);
    ini.setValue("GATE_enable", _stockExcange_GATE_enable);
    ini.setValue("BYBIT_enable", _stockExcange_BYBIT_enable);

    ini.endGroup();

    ini.sync();

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg("Save configuration to " +  _configFileName);

    return true;
}

DBConnectionInfo Config::db_ConnectionInfo() const
{
    return _db_ConnectionInfo;
}

ServerInfo Config::server_config() const
{
    return _server_config;
}

bool Config::stockExcange_MEXC_enable() const
{
    return _stockExcange_MEXC_enable;
}

bool Config::stockExcange_KUCOIN_enable() const
{
    return _stockExcange_KUCOIN_enable;
}

bool Config::stockExcange_GATE_enable() const
{
    return _stockExcange_GATE_enable;
}

bool Config::stockExcange_BYBIT_enable() const
{
    return _stockExcange_BYBIT_enable;
}

const QList<ProxyInfo>& Config::proxy_info_list() const
{
    return _proxy_info_list;
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
