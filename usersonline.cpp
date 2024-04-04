#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>
#include <QMutexLocker>

#include "usersonline.h"

using namespace TradingCat;
using namespace Common;

static const quint64 CONNECTION_TIMEOUT = 60 * 1000;
static const quint64 SAVE_USER_DATA_INTERVAL = 60 * 10 * 1000;
static const quint64 HISTORY_KLINE_COUNT = 60;
static const quint64 REVIEW_HISTORY_KLINE_COUNT = 12 * 24;
static const qsizetype MAX_DETECT_EVENT = 5;

static QMutex onlineMutex;
static QMutex userDataMutex;

UsersOnline::UsersOnline(const Common::DBConnectionInfo &dbConnectionInfo, QObject *parent /* = nullptr*/)
    : QObject{parent}
    , _dbConnectionInfo(dbConnectionInfo)
{
}

UsersOnline::~UsersOnline()
{
    if (_db.isOpen())
    {
        _db.close();
    }

    for (auto user: _usersData)
    {
        delete user;
    }
    _usersData.clear();

    delete _connetionTimeoutTimer;

    delete _history;
}

bool UsersOnline::start()
{
    if (!Common::connectToDB(_db, _dbConnectionInfo, "UsersOnlineTraidongCatBotDB"))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, connectDBErrorString(_db));

        return false;
    }

    _connetionTimeoutTimer = new QTimer();

    QObject::connect(_connetionTimeoutTimer, SIGNAL(timeout()), SLOT(connectionTimeout()));

    _connetionTimeoutTimer->start(CONNECTION_TIMEOUT);

    if (!loadUserData())
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, "Cannot load users data");

        return false;
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Users data load successfull. Total users: %1").arg(_usersData.size()));

    _history = new History(_dbConnectionInfo);
    if (_history->isError())
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Cannot load history. Error: ").arg(_history->errorString()));

        return false;
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("History load succesfully"));

    _saveUserDataTimer = new QTimer();

    QObject::connect(_saveUserDataTimer, SIGNAL(timeout()), SLOT(saveAllOnlineUserData()));

    _saveUserDataTimer->start(SAVE_USER_DATA_INTERVAL);

    return true;
}

static int getID()
{
    static int id = 0;

    static QMutex newIDMutex;

    QMutexLocker<QMutex> locker(&newIDMutex);

    return ++id;
}

std::optional<UsersOnline::NewSessionData> UsersOnline::checkUser(const QString &user, const QString &password)
{
    QMutexLocker<QMutex> userDataLocker(&userDataMutex);

    auto userData_it = _usersData.find(user);
    if (userData_it == _usersData.end())
    {
        return {};
    }

    auto& userData = userData_it.value();
    if (userData->password != password)
    {
        return {};
    }

    userData->lastLogin = QDateTime::currentDateTime();

    auto tmp = new SessionData();
    tmp->lastData = QDateTime::currentDateTime();
    tmp->user = user;
    tmp->password = password;
    tmp->filter = userData->config.filter;

    UsersOnline::NewSessionData result;
    result.config = userData->config;
    result.id = getID();

    QMutexLocker<QMutex> locker(&onlineMutex);
    _onlineUsers.insert(result.id, tmp);

    emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("User is connect. User: %1 SessionID: %2").arg(user).arg(result.id));

    return std::make_optional(std::move(result));
}

bool UsersOnline::isOnline(int sessionID)
{
    QMutexLocker<QMutex> locker(&onlineMutex);
    auto onlineUser_it = _onlineUsers.find(sessionID);
    if (onlineUser_it == _onlineUsers.end())
    {
        return false;
    }

    onlineUser_it.value()->lastData = QDateTime::currentDateTime();

    return true;
}


bool UsersOnline::logout(int sessionID)
{
    QMutexLocker<QMutex> locker(&onlineMutex);

    auto onlineUser_it = _onlineUsers.find(sessionID);
    if (onlineUser_it == _onlineUsers.end())
    {
        return false;
    }

    QMutexLocker<QMutex> userDataLocker(&userDataMutex);

    auto onlineUser = onlineUser_it.value();
    auto userData = _usersData.find(onlineUser_it.value()->user).value();

    userData->password = onlineUser->password;
    userData->config.filter = onlineUser->filter;

    saveUserData(*userData);

    _onlineUsers.erase(onlineUser_it);

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Logout. User: %1 SessionID: %2").arg(userData->user).arg(sessionID));

    return true;
}

std::optional<UsersOnline::KLinesDetectedList> UsersOnline::data(int sessionID)
{
    QMutexLocker<QMutex> locker(&onlineMutex);

    auto onlineUser_it = _onlineUsers.find(sessionID);
    if (onlineUser_it == _onlineUsers.end())
    {
        return {};
    }

    auto onlineUser = onlineUser_it.value();
    auto result = onlineUser->klinesDetectedList;
    onlineUser->klinesDetectedList.detectedKLines.clear();
    onlineUser->klinesDetectedList.isFull = false;
    onlineUser->lastData = QDateTime::currentDateTime();

    return std::make_optional(result);
}

void UsersOnline::checkKLine(const StockExchangeKLine &kline)
{
    _history->add(kline);

    const auto currentDateTime = QDateTime::currentDateTime();

    //отбрасываем слишком старые свечи
    if (kline.kline.closeTime.msecsTo(currentDateTime) > static_cast<quint64>(kline.kline.id.type) * 10)
    {
        return;
    }

    const double delta = deltaKLine(kline.kline);
    const double volume = volumeKLine(kline.kline);

    QMutexLocker<QMutex> locker(&onlineMutex);
    for (auto onlineUser_it = _onlineUsers.begin(); onlineUser_it != _onlineUsers.end(); ++onlineUser_it)
    {
        auto& onlineUser = onlineUser_it.value();

        if (onlineUser->klinesDetectedList.detectedKLines.size() > MAX_DETECT_EVENT)
        {
            if (!onlineUser->klinesDetectedList.isFull)
            {
                emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE,
                                QString("Too many detect KLines for user. Some events was been skiped. Session ID: %1 User: %2")
                                    .arg(onlineUser_it.key())
                                    .arg(onlineUser_it.value()->user));
            }

            onlineUser->klinesDetectedList.isFull = true;

            continue;
        }

        if (onlineUser->filter.filter(kline.stockExcahnge, kline.kline.id, delta, volume))
        {
            KLineDetect tmp;
            tmp.stockExchangeID = kline.stockExcahnge;
            tmp.delta = delta;
            tmp.volume = volume;
            tmp.history = _history->getHistory(kline.stockExcahnge, kline.kline.id,
                                               currentDateTime.addMSecs(-(HISTORY_KLINE_COUNT * static_cast<quint64>(kline.kline.id.type))));

            Q_ASSERT(!tmp.history.isEmpty());

            auto reviewKLineID = kline.kline.id;
            reviewKLineID.type = KLineType::MIN5;
            tmp.reviewHistory = _history->getHistory(kline.stockExcahnge, reviewKLineID,
                currentDateTime.addMSecs(-(REVIEW_HISTORY_KLINE_COUNT * static_cast<quint64>(reviewKLineID.type))));

            onlineUser->klinesDetectedList.detectedKLines.emplaceBack(std::move(tmp));

            emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                            QString("Detect KLine for user. Session ID: %1 User: %2 StockExchange: %3 Money: %4 Interval: %5")
                                .arg(onlineUser_it.key())
                                .arg(onlineUser_it.value()->user)
                                .arg(kline.stockExcahnge.name)
                                .arg(kline.kline.id.symbol)
                                .arg(KLineTypeToString(kline.kline.id.type)));
        }
    }
}

UsersOnline::UserOnlineDataList UsersOnline::usersOnline() const
{
    UserOnlineDataList result;

    QMutexLocker<QMutex> locker(&onlineMutex);
    for (const auto user: _onlineUsers)
    {
        UserOnlineData tmp;
        tmp.user = user->user;

        result.emplaceBack(std::move(tmp));
    }

    return result;
}

void UsersOnline::connectionTimeout()
{
    QList<int> timeoutSessionIDList;

    {
        QMutexLocker<QMutex> locker(&onlineMutex);
        for (auto onlineUser_it = _onlineUsers.begin(); onlineUser_it != _onlineUsers.end(); ++onlineUser_it)
        {
            if (onlineUser_it.value()->lastData.msecsTo(QDateTime::currentDateTime()) > CONNECTION_TIMEOUT)
            {
                const auto sessionID = onlineUser_it.key();
                timeoutSessionIDList.push_back(sessionID);

                emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE,
                                QString("Connection timeout. SessionID: %1").arg(sessionID));
            }
        }
    }

    for (const auto sessionID: timeoutSessionIDList)
    {
        logout(sessionID);
    }
}

void UsersOnline::saveAllOnlineUserData()
{
    if (_onlineUsers.isEmpty())
    {
        sendEmptyQuery();

        return;
    }

    QMutexLocker<QMutex> locker(&onlineMutex);

    for (auto onlineUser_it = _onlineUsers.begin(); onlineUser_it != _onlineUsers.end();  ++onlineUser_it)
    {
        QMutexLocker<QMutex> userDataLocker(&userDataMutex);

        auto onlineUser = onlineUser_it.value();
        auto userData = _usersData.find(onlineUser_it.value()->user).value();

        userData->password = onlineUser->password;
        userData->config.filter = onlineUser->filter;

        saveUserData(*userData);

        emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Save user data. User: %1 SessionID: %2").arg(userData->user).arg(onlineUser_it .key()));
    }
}

bool UsersOnline::loadUserData()
{
    const auto queryText =
        QString("SELECT ID, USER, PASSWORD, CONFIG, LAST_LOGIN "
                "FROM %1.APP_USERS")
            .arg(_db.databaseName());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return false;
    }

    _usersData.clear();
    while(query.next())
    {
        auto tmp = new UserData;
        tmp->user = query.value("USER").toString();
        tmp->ID = query.value("ID").toULongLong();
        tmp->password = query.value("PASSWORD").toString();
        tmp->lastLogin = query.value("LAST_LOGIN").toDateTime();

        const auto config = query.value("CONFIG").toString();
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(config.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Error parse user config. User: %1. Error: %2")
                                .arg(tmp->user)
                                .arg(error.errorString()));

        }
        else
        {
            tmp->config.filter = Filter(doc.object()["Filter"].toArray());
        }

        _usersData.insert(query.value("USER").toString(), tmp);
    }

    _db.commit();

    return true;
}

void UsersOnline::saveUserData(const UserData &userData)
{
    if (userData.ID == 0)
    {
        return;
    }

    QJsonObject jsonConfig;
    jsonConfig.insert("Filter", userData.config.filter.getFilter());

    const auto queryText =
        QString("UPDATE %1.APP_USERS "
                "SET "
                "PASSWORD = '%2', "
                "CONFIG = '%3', "
                "LAST_LOGIN = '%4' "
                "WHERE ID = %5 ")
            .arg(_db.databaseName())
            .arg(userData.password)
            .arg(QJsonDocument(jsonConfig).toJson(QJsonDocument::Compact))
            .arg(userData.lastLogin.toString(DATETIME_FORMAT))
            .arg(userData.ID);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return;
    }

    _db.commit();
}

void UsersOnline::sendEmptyQuery()
{
    const auto queryText =
        QString("SELECT ID "
                "FROM %1.APP_USERS LIMIT 1")
            .arg(_db.databaseName());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return;
    }

    _db.commit();
}

bool UsersOnline::config(int sessionID, const QJsonObject &newConfig)
{
    QString userName;
    const auto newFilter = Filter(newConfig["Filter"].toArray());
    {
        QMutexLocker<QMutex> locker(&onlineMutex);

        auto onlineUser_it = _onlineUsers.find(sessionID);
        if (onlineUser_it == _onlineUsers.end())
        {
            return false;
        }

        auto onlineUser = onlineUser_it.value();
        onlineUser->filter = newFilter;
        userName = onlineUser->user;
    }

    QMutexLocker<QMutex> userDataLocker(&userDataMutex);
    auto userData_it = _usersData.find(userName);
    if (userData_it != _usersData.end())
    {
        userData_it.value()->config.filter = newFilter;
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Chenge filter. User: %1 SessionID: %2").arg(userName).arg(sessionID));

    return true;
}

bool UsersOnline::newUser(const QString &user, const QString &password)
{
    QMutexLocker<QMutex> userDataLocker(&userDataMutex);

    _db.transaction();
    QSqlQuery query(_db);

    QString queryText;
    if (_usersData.contains(user))
    {
        queryText =
            QString("DELETE FROM %1.APP_USERS "
                    "WHERE USER = '%2'")
                .arg(_db.databaseName())
                .arg(user);

        if (!query.exec(queryText))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

            _db.rollback();

            return false;
        }

        emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("User data will be replace. User: %1").arg(user));
    }

    QJsonObject jsonDefaulConfig;
    jsonDefaulConfig.insert("Filter", Filter().getFilter());

    queryText =
        QString("INSERT INTO %1.APP_USERS "
                "(USER, PASSWORD, CONFIG, CREATE_USER, LAST_LOGIN) "
                "VALUES "
                "('%2', '%3', '%4', '%5', '%6')")
            .arg(_db.databaseName())
            .arg(user)
            .arg(password)
            .arg(QJsonDocument(jsonDefaulConfig).toJson(QJsonDocument::Compact))
            .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
            .arg(QDateTime::fromString("2000-01-01 00:00:00.001", DATETIME_FORMAT).toString(DATETIME_FORMAT));

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return false;
    }

    _db.commit();

    loadUserData();

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Added user. User: %1").arg(user));

    return true;
}

