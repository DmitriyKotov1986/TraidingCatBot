#ifndef USERSONLINE_H
#define USERSONLINE_H

//STL
#include <optional>

//Qt
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QSqlDatabase>

//My
#include "types.h"
#include "filter.h"
#include "history.h"
#include "Common/common.h"
#include "Common/tdbloger.h"

namespace TradingCat
{

class UsersOnline
    : public QObject
{
    Q_OBJECT

public:
    struct UserConfig
    {
        Filter filter;
    };

    struct NewSessionData
    {
        UserConfig config;
        int id = 0;
    };

    struct UserData
    {
        quint64 ID = 0;
        QString user;
        QString password;
        UserConfig config;
        QDateTime lastLogin;
    };

    struct KLineDetect
    {
        StockExchangeID stockExchangeID;
        History::KLineHistory history;
        History::KLineHistory reviewHistory;
        double delta;
        double volume;
    };

    struct KLinesDetectedList
    {
        QList<KLineDetect> detectedKLines;
        bool isFull = false;
    };

    struct UserOnlineData
    {
        QString user;
    };

    using UserOnlineDataList = QList<UserOnlineData>;

public:
    explicit UsersOnline(const Common::DBConnectionInfo& dbConnectionInfo, QObject *parent = nullptr);
    ~UsersOnline();

    bool start();

    std::optional<NewSessionData> checkUser(const QString& user, const QString& password);
    bool isOnline(int sessionID);
    bool logout(int sessionID);
    std::optional<KLinesDetectedList> data(int sessionID);
    bool config(int sessionID, const QJsonObject &newConfig);
    bool newUser(const QString &user, const QString &password);

    void checkKLine(const TradingCat::StockExchangeKLine& kline);

    UserOnlineDataList usersOnline() const;

signals:
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg);

private slots:
    void connectionTimeout();
    void saveAllOnlineUserData();

private:
    bool loadUserData();
    void saveUserData(const UserData &userData);
    void addUserToDB(const QString& user, const QString& password);
    void sendEmptyQuery();

private:
    struct SessionData
    {
        QString user;
        QString password;
        QDateTime lastData = QDateTime::currentDateTime();
        KLinesDetectedList klinesDetectedList;
        Filter filter;
    };

private:
    const Common::DBConnectionInfo _dbConnectionInfo;
    QSqlDatabase _db;

    History *_history = nullptr;

    QHash<QString, UserData*> _usersData; //данные пользователей

    QHash<int, SessionData*> _onlineUsers;
    QHash<int, SessionData*> _offlineUsers;

    QTimer *_connetionTimeoutTimer = nullptr;
    QTimer *_saveUserDataTimer = nullptr;
};

} //namespace TraidingCatBot

#endif // USERSONLINE_H
