#ifndef APPSERVER_H
#define APPSERVER_H

//QT
#include <QObject>
#include <QHttpServer>
#include <QSqlDatabase>
#include <QJsonArray>
#include <QHash>
#include <QList>
#include <QTimer>
#include <QMutex>

//My
#include "types.h"
#include "history.h"
#include "filter.h"
#include "usersonline.h"
#include "Common/tdbloger.h"
#include "Common/common.h"

namespace TradingCat
{

class AppServer
    : public QObject
{
    Q_OBJECT

public:
    AppServer() = delete;

    explicit AppServer(const ServerInfo& serverConfig, const Common::DBConnectionInfo& dbConnectionInfo, QObject *parent = nullptr);
    ~AppServer();

public slots:
    void start();
    void stop();

    void getKLines(const TradingCat::StockExchangeKLinesList& klines);

private slots:
    void sendLogMsgSlot(Common::TDBLoger::MSG_CODE category, const QString& msg);
    void loadKLines();

signals:
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg);
    void started();
    void finished();

private:
    int getConnectionID();

    bool addServer();

    bool filter(const QString& userName, const StockExchangeID& stockExchangeID, const KLineID& klineID, double delta, double volume);

    //answers
    QString checkUser(const QString& user, const QString& password);
    QString logout(int sessionID);
    QString klines(int sessionID);
    QString data(int sessionID);
    QString config(int sessionID, const QByteArray& newConfig);
    QString newUser(const QString& user, const QString& password);
    QString serverStatus();

private:
    const ServerInfo _serverConfig;
    const Common::DBConnectionInfo _dbConnectionInfo;
    QSqlDatabase _db;

    UsersOnline *_usersOnline = nullptr;

    QJsonArray _klines;
    QTimer *_klinesUpdateTimer = nullptr;

    QHttpServer *_server = nullptr;


};

} //namespace TraidingCatBot

#endif // APPSERVER_H
