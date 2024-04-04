#ifndef TCONFIG_H
#define TCONFIG_H

//QT
#include <QString>
#include <QSet>

//My
#include "Common/common.h"
#include "types.h"

namespace TradingCat
{

class Config final
{
public:
    static Config* config(const QString& configFileName = "");
    static void deleteConfig();

private:
    explicit Config(const QString& configFileName);

public:
    bool save();

    //[DATABASE]
    Common::DBConnectionInfo db_ConnectionInfo() const;

    //[Server]
    ServerInfo server_config() const;

    //[STOCKEXCHANGES]
    bool stockExcange_MEXC_enable() const;
    bool stockExcange_KUCOIN_enable() const;
    bool stockExcange_GATE_enable() const;
    bool stockExcange_BYBIT_enable() const;

    //[PROXY_N]
    const QList<Common::ProxyInfo>& proxy_info_list() const;

    QString errorString();
    bool isError() const;

private:
    const QString _configFileName;

    QString _errorString;

    //[DATABASE]
    Common::DBConnectionInfo _db_ConnectionInfo;

    //[SERVER]
    ServerInfo _server_config;

    //[STOCKEXCHANGES]
    bool _stockExcange_MEXC_enable = true;
    bool _stockExcange_KUCOIN_enable = true;
    bool _stockExcange_GATE_enable = true;
    bool _stockExcange_BYBIT_enable = true;

    //[PROXY_N]
    QList<Common::ProxyInfo> _proxy_info_list;
};

} //namespace TraidingCatBot

#endif // TCONFIG_H
