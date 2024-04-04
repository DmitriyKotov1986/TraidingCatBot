#include <QHttpServerResponse>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QSslKey>

#include "appserver.h"

using namespace TradingCat;
using namespace Common;

static QMutex klineMutex;

static quint64 KLINE_UPDATE_INTERVAL = 60 * 60 * 1000; //ms

AppServer::AppServer(const ServerInfo &serverConfig, const Common::DBConnectionInfo& dbConnectionInfo, QObject *parent)
    : QObject{parent}
    , _serverConfig(serverConfig)
    , _dbConnectionInfo(dbConnectionInfo)
{
}

AppServer::~AppServer()
{
    stop();
}

void AppServer::start()
{
    if (!Common::connectToDB(_db, _dbConnectionInfo, "AppServerTraidongCatBotDB"))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, connectDBErrorString(_db));

        return;
    }

    _usersOnline = new UsersOnline(_dbConnectionInfo);

    QObject::connect(_usersOnline, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                          SLOT( sendLogMsgSlot(Common::TDBLoger::MSG_CODE, const QString&)));

    _usersOnline->start();

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "User data loaded successfully");

    loadKLines();

    _klinesUpdateTimer = new QTimer();

    QObject::connect(_klinesUpdateTimer, SIGNAL(timeout()), SLOT(loadKLines()));

    _klinesUpdateTimer->start(KLINE_UPDATE_INTERVAL);

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "KLine data loaded successfully");

    if (addServer())
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Application server listen on: %1:%2")
                                                              .arg(_serverConfig.address.toString())
                                                              .arg(_serverConfig.port));
    }

    emit started();
}

void AppServer::stop()
{
    Q_CHECK_PTR(_server);

    if (_server != nullptr)
    {
        _server->disconnect();

        delete _server;
    }

    if (_db.isOpen())
    {
        _db.close();
    }

    delete _usersOnline;
    _usersOnline = nullptr;

    emit finished();
}

void AppServer::getKLines(const TradingCat::StockExchangeKLinesList& klines)
{
    for (const auto& kline: klines)
    {
        _usersOnline->checkKLine(kline);
    }
}

void AppServer::sendLogMsgSlot(Common::TDBLoger::MSG_CODE category, const QString &msg)
{
    emit sendLogMsg(category, msg);
}

QString AppServer::checkUser(const QString &user, const QString &password)
{
    Q_CHECK_PTR(_usersOnline);

    QJsonObject json;
    if (user.isEmpty() || password.isEmpty())
    {
        json.insert("Result", "FAIL");
        json.insert("Message", "User or password is empty");

        emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("User or password is empty"));
    }
    else
    {
        const auto newSessionData = _usersOnline->checkUser(user, password);
        if (!newSessionData.has_value())
        {
            json.insert("Result", "FAIL");
            json.insert("Message", "User or password is incorrect");

            emit sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE,
                            QString("User or password is incorrect. User: %1 Password: %2").arg(user).arg(password));
        }
        else
        {
            const auto& sessionData = newSessionData.value();
            json.insert("Result", "OK");
            json.insert("Filter", sessionData.config.filter.getFilter());
            json.insert("SessionID", sessionData.id);
        }
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::logout(int sessionID)
{
    QJsonObject json;

    if (!_usersOnline->logout(sessionID))
    {
        json.insert("Result", "LOGOUT");
        json.insert("Message", "User is not login");
    }
    else
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                        QString("User logout. Session ID: %1")
                            .arg(sessionID));

        json.insert("Result", "OK");
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::klines(int sessionID)
{
    QJsonObject json;

    if (!_usersOnline->isOnline(sessionID))
    {
        json.insert("Result", "LOGOUT");
        json.insert("Message", "User is not login");
    }
    else
    {
        QMutexLocker<QMutex> locker(&klineMutex);

        json.insert("Result", "OK");
        json.insert("KLines", _klines);
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::data(int sessionID)
{
    QJsonObject json;

    const auto klinesDetectedList = _usersOnline->data(sessionID);
    if (!klinesDetectedList.has_value())
    {

        json.insert("Result", "LOGOUT");
        json.insert("Message", "User is not login");
    }
    else
    {
        json.insert("Result", "OK");

        QJsonArray detectKLines;
        for (const auto& klineData: klinesDetectedList.value().detectedKLines)
        {
            QJsonObject tmp;
            tmp.insert("StockExchange", klineData.stockExchangeID.name);
            tmp.insert("Delta", klineData.delta);
            tmp.insert("Volume", klineData.volume);

            QJsonArray history;
            for (const auto& kline: klineData.history)
            {
                QJsonObject klineJson;
                klineJson.insert("Money", kline.id.symbol);
                klineJson.insert("Interval", KLineTypeToString(kline.id.type));
                klineJson.insert("OpenTime", kline.openTime.toString(DATETIME_FORMAT));
                klineJson.insert("CloseTime", kline.closeTime.toString(DATETIME_FORMAT));
                klineJson.insert("Open", kline.open);
                klineJson.insert("Close", kline.close);
                klineJson.insert("High", kline.high);
                klineJson.insert("Low", kline.low);
                klineJson.insert("Volume", kline.volume);
                klineJson.insert("QuoteAssetVolume", kline.quoteAssetVolume);

                history.push_back(klineJson);
            }

            tmp.insert("History", history);

            QJsonArray reviewHistory;
            for (const auto& kline: klineData.reviewHistory)
            {
                QJsonObject klineJson;
                klineJson.insert("Money", kline.id.symbol);
                klineJson.insert("Interval", KLineTypeToString(kline.id.type));
                klineJson.insert("OpenTime", kline.openTime.toString(DATETIME_FORMAT));
                klineJson.insert("CloseTime", kline.closeTime.toString(DATETIME_FORMAT));
                klineJson.insert("Open", kline.open);
                klineJson.insert("Close", kline.close);
                klineJson.insert("High", kline.high);
                klineJson.insert("Low", kline.low);
                klineJson.insert("Volume", kline.volume);
                klineJson.insert("QuoteAssetVolume", kline.quoteAssetVolume);

                reviewHistory.push_back(klineJson);
            }

            tmp.insert("ReviewHistory", reviewHistory);

            detectKLines.push_back(tmp);
        }

        json.insert("DetectKLines", detectKLines);

        if (klinesDetectedList.value().isFull)
        {
            QJsonObject msg;
            msg.insert("Level", "INFO");
            msg.insert("Message", "Too many events detected. Some events were missed. Check your filter settings.");

            QJsonArray messages;
            messages.push_back(msg);

            json.insert("UserMessages", messages);
        }
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::config(int sessionID, const QByteArray &newConfig)
{
    QJsonObject json;

    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(newConfig, &error);
    if (error.error != QJsonParseError::NoError)
    {
        json.insert("Result", "FAIL");
        json.insert("Message", QString("Cannot parse JSON: %1").arg(error.errorString()));
    }
    else if (!_usersOnline->config(sessionID, jsonDoc.object()))
    {

        json.insert("Result", "FAIL");
        json.insert("Message", "Incorrect configuration");
    }
    else
    {
        json.insert("Result", "OK");
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::newUser(const QString &user, const QString &password)
{
    QJsonObject json;

    if (!_usersOnline->newUser(user, password))
    {
        json.insert("Result", "FAIL");
        json.insert("Message", "User already exist");
    }
    else
    {
        json.insert("Result", "OK");
    }

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

QString AppServer::serverStatus()
{
    QJsonObject json;

    json.insert("Result", "OK");

    QJsonArray usersOnline_json;
    const auto usersOnline =_usersOnline->usersOnline();
    for (const auto& user: usersOnline)
    {
        QJsonObject user_json;
        user_json.insert("User", user.user);

        usersOnline_json.push_back(user_json);
    }

    json.insert("UsersOnline", usersOnline_json);

    const QString answer = QJsonDocument(json).toJson(QJsonDocument::Compact);

    return answer;
}

void  AppServer::loadKLines()
{
    QMutexLocker<QMutex> locker(&klineMutex);

    const auto queryText =
        QString("SELECT STOCK_EXCHANGE, MONEY, KLINE_INTERVAL "
                "FROM %1.KLINES "
                "WHERE KLINE_INTERVAL  = '1m'")
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

    _klines = QJsonArray();
    while(query.next())
    {
        QJsonObject kline;
        kline.insert("StockExchange", query.value("STOCK_EXCHANGE").toString());
        kline.insert("Money", query.value("MONEY").toString());
        kline.insert("Interval", query.value("KLINE_INTERVAL").toString());

        _klines.push_back(kline);
    }

    QJsonObject klineMEXC;
    klineMEXC.insert("StockExchange", "MEXC");
    klineMEXC.insert("Money", "ALL");
    klineMEXC.insert("Interval", KLineTypeToString(KLineType::MIN1));

    _klines.push_back(klineMEXC);

    QJsonObject klineKUCOIN;
    klineKUCOIN.insert("StockExchange", "KUCOIN");
    klineKUCOIN.insert("Money", "ALL");
    klineKUCOIN.insert("Interval", KLineTypeToString(KLineType::MIN1));

    _klines.push_back(klineKUCOIN);

    QJsonObject klineGATE;
    klineGATE.insert("StockExchange", "GATE");
    klineGATE.insert("Money", "ALL");
    klineGATE.insert("Interval", KLineTypeToString(KLineType::MIN1));

    _klines.push_back(klineGATE);

    QJsonObject klineBYBIT;
    klineBYBIT.insert("StockExchange", "BYBIT");
    klineBYBIT.insert("Money", "ALL");
    klineBYBIT.insert("Interval", KLineTypeToString(KLineType::MIN1));

    _klines.push_back(klineBYBIT);


    _db.commit();

    return;
}

bool AppServer::addServer()
{
    _server = new QHttpServer();

    _server->route("/login/<arg>/<arg>",
                   [this](const QString& user, const QString& password)
                   {
                       return checkUser(user, password);
                   });

    _server->route("/klines/<arg>",
                   [this](int sessionID)
                   {
                       return klines(sessionID);
                   });

    _server->route("/data/<arg>",
                   [this](int sessionID)
                   {
                       return data(sessionID);
                   });

    _server->route("/logout/<arg>",
                   [this](int sessionID)
                   {
                       return logout(sessionID);
                   });

    _server->route("/config/<arg>",
                   [this](int sessionID, const QHttpServerRequest &request)
                   {
                       return config(sessionID, request.body());
                   });

    _server->route("/newuser/<arg>/<arg>",
                   [this](const QString& user, const QString& password)
                   {
                       return newUser(user, password);
                   });

    _server->route("/status",
                   [this]()
                   {
                       return serverStatus();
                   });


    _server->afterRequest([](QHttpServerResponse &&resp)
                          {
                              resp.addHeader("Server", "TraidingCatBot server");
                              resp.addHeader("Cross-Origin-Embedder-Policy","require-corp");
                              resp.addHeader("Cross-Origin-Opener-Policy", "same-origin");
                              resp.addHeader("Content-Type", "application/json");
#ifdef QT_DEBUG
                              resp.addHeader("Access-Control-Allow-Origin", "*");
                              resp.addHeader("Access-Control-Allow-Headers", "*");
#endif

                              return std::move(resp);
                          });

    if (!_serverConfig.crtFileName.isEmpty() && !_serverConfig.keyFileName.isEmpty())
    {
        const auto sslCertificateChain = QSslCertificate::fromPath(_serverConfig.crtFileName);
        if (sslCertificateChain.empty())
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Couldn't retrieve SSL certificate from file: %1").arg(_serverConfig.crtFileName));

            return false;
        }

        QFile privateKeyFile(_serverConfig.keyFileName);
        if (!privateKeyFile.open(QIODevice::ReadOnly))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Couldn't open file for reading: %1. Error: %2")
                                                                .arg(_serverConfig.keyFileName)
                                                                .arg(privateKeyFile.errorString()));
            return false;
        }

        _server->sslSetup(sslCertificateChain.front(), QSslKey(&privateKeyFile, QSsl::Rsa));

        privateKeyFile.close();
    }

    if (!_server->listen(_serverConfig.address, _serverConfig.port))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Cannot listen application server on: %1:%2")
                                                            .arg(_serverConfig.address.toString())
                                                            .arg(_serverConfig.port));

        return false;
    }

    return true;
}


