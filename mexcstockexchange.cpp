//QT
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>

#include "mexcstockexchange.h"

using namespace TradingCat;
using namespace Common;

static const QUrl DEFAULT_SYMBOL_URL{"https://api.mexc.com/api/v3/defaultSymbols"};
static const qsizetype KLINES_SEND_COUNT = 100;

MexcStockExchange::MexcStockExchange(const Common::DBConnectionInfo& dbConnectionInfo, Common::HTTPSSLQuery::ProxyList proxyList, QObject *parent)
    : _dbConnectionInfo(dbConnectionInfo)
    , _proxyList(proxyList)
{
    qRegisterMetaType<TradingCat::StockExchangeKLinesList>("StockExchangeKLinesList");

    _headers.insert(QByteArray{"X-MEXC-APIKEY"}, QByteArray{""});
    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});
    _headers.insert(QByteArray{"User-Agent"}, QCoreApplication::applicationName().toUtf8());

    _id.name = "MEXC";

    _klineTypes.insert(KLineType::MIN1);
    _klineTypes.insert(KLineType::MIN5);
 //   _klineTypes.insert(KLineType::MIN15);
 //   _klineTypes.insert(KLineType::MIN30);
 //   _klineTypes.insert(KLineType::MIN60);
 //   _klineTypes.insert(KLineType::HOUR4);
 //   _klineTypes.insert(KLineType::HOUR8);
 //   _klineTypes.insert(KLineType::DAY1);
}

void MexcStockExchange::start()
{
    if (!Common::connectToDB(_db, _dbConnectionInfo, "MEXCDB"))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, connectDBErrorString(_db));

        return;
    }

    loadLastStopKLine();

    _httpSSLQuery = new HTTPSSLQuery(_proxyList);

    QObject::connect(_httpSSLQuery, SIGNAL(getAnswer(const QByteArray&, quint64)),
                     SLOT(getAnswerHTTP(const QByteArray&, quint64)));
    QObject::connect(_httpSSLQuery, SIGNAL(errorOccurred(QNetworkReply::NetworkError, quint64, const QString&, quint64)),
                     SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, quint64, const QString&, quint64)));

    sendUpdateMoney();

    //Update money list
    _updateMoneyTimer = new QTimer();

    QObject::connect(_updateMoneyTimer, SIGNAL(timeout()), SLOT(sendUpdateMoney()));

    _updateMoneyTimer->start(1000 * 60 * 10);

    sendUpdateMoney();
}

void MexcStockExchange::stop()
{
    for (const auto& kline: _moneyKLine)
    {
        kline->stop();

        delete kline;
    }

    _moneyKLine.clear();

    delete _httpSSLQuery;

    delete _updateMoneyTimer;

    emit finished();
}

void MexcStockExchange::getAnswerHTTP(const QByteArray &answer, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    switch (_currentRequestType)
    {
    case RequestType::DEFAULT_SYMBOLS:
    {
        const auto moneyList = parseDefaultSymbol(answer);
        makeMoney(moneyList);

        break;
    }
    default:
        Q_ASSERT(false);
    }

    _currentRequestID = 0;
    _currentRequestType = RequestType::NONE;
}

void MexcStockExchange::errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString &msg, quint64 id)
{
    if (id !=  _currentRequestID)
    {
        return;
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("MEXC: HTTP error. Message: %2. Request id: %3")
                                                         .arg(code)
                                                         .arg(msg)
                                                         .arg(id));
    switch (_currentRequestType)
    {
    case RequestType::DEFAULT_SYMBOLS:
    {
        sendUpdateMoney();
        break;
    }
    default:
        Q_ASSERT(false);
    }

    _currentRequestID = 0;
    _currentRequestType = RequestType::NONE;
}

void MexcStockExchange::errorOccurredMoneyKLine(const KLineID &id, const QString &msg)
{
    emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("MEXC: Money KLine error. Money: %1. Interval: %2. Message: %3")
                                                        .arg(id.symbol)
                                                        .arg(KLineTypeToString(id.type))
                                                        .arg(msg));
}

void MexcStockExchange::getKLinesMoney(const KLinesList &klines)
{
    for (const auto& kline: klines)
    {
        StockExchangeKLine tmp;
        tmp.kline = kline;
        tmp.stockExcahnge = _id;

        _klines.emplaceBack(tmp);
    }

    if (_klines.size() > KLINES_SEND_COUNT)
    {
        emit getKLines(_klines);

        _klines.clear();
    }
}

MexcStockExchange::MoneySymbols MexcStockExchange::parseDefaultSymbol(const QByteArray &data)
{
    MoneySymbols result;

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("MEXC: JSON parse error. Message: %1")
                                                             .arg(error.errorString()));

        return result;
    }

    const auto errCode = doc["code"].toString();
    if (errCode != "")
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("MEXC: Stock excenge server error. Code: %1. Message: %2")
                                                             .arg(errCode)
                                                             .arg(doc["msg"].toString()));

        return result;
    }

    const auto symbolsList = doc["data"].toArray();
    for (const auto& symbol: symbolsList)
    {
        const auto& tmp = symbol.toString();
        if (!tmp.isEmpty())
        {
            result.insert(tmp);
        }
    }

    return result;
}

void MexcStockExchange::makeMoney(const MoneySymbols& symbolsList)
{
    if (symbolsList.isEmpty())
    {
        return;
    }

    quint64 i = 0;
    quint64 addCount = 0;
    for (const auto& symbol: symbolsList)
    {
        if (symbol.last(4) != "USDT")
        {
            continue;
        }

        for (const auto klineType: _klineTypes)
        {
            //Добавляем KLine
            KLineID klineID;
            klineID.symbol = symbol;
            klineID.type = klineType;

            if (_moneyKLine.contains(klineID))
            {
                break;
            }

            quint32 interval = static_cast<qint64>(klineType);

            auto moneyLastStopKLine_it = _moneyLastStopKLine.find(klineID);
            auto mexcKLine = new MexcKLine(klineID,
                moneyLastStopKLine_it != _moneyLastStopKLine.end() ? moneyLastStopKLine_it.value() : QDateTime::currentDateTime().addDays(-1),
                _httpSSLQuery);

            QObject::connect(mexcKLine, SIGNAL(getKLines(const TradingCat::KLinesList&)),
                         SLOT(getKLinesMoney(const TradingCat::KLinesList&)));

            QObject::connect(mexcKLine, SIGNAL(errorOccurred(const TradingCat::KLineID&, const QString&)),
                             SLOT(errorOccurredMoneyKLine(const TradingCat::KLineID&, const QString&)));

            QObject::connect(mexcKLine, SIGNAL(delisting(const TradingCat::KLineID&)),
                             SLOT(delisting(const TradingCat::KLineID&)));

            _moneyKLine.insert(klineID, mexcKLine);

            if (klineID.type == KLineType::MIN1)
            {
                QTimer::singleShot((interval / symbolsList.size()) * i  * 10 + 10000, [mexcKLine](){ mexcKLine->start(); });
            }
            else
            {
                QTimer::singleShot((interval / symbolsList.size()) * i + 10000, [mexcKLine](){ mexcKLine->start(); });
            }

            ++addCount;
        }

        ++i;
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Total money on MEXC: %1 Added: %2").arg(i).arg(addCount));
}

void MexcStockExchange::sendUpdateMoney()
{
    Q_CHECK_PTR(_httpSSLQuery);

    _currentRequestID = _httpSSLQuery->send(DEFAULT_SYMBOL_URL, HTTPSSLQuery::RequestType::GET, {}, _headers);
    _currentRequestType = RequestType::DEFAULT_SYMBOLS;
}

void MexcStockExchange::delisting(const KLineID &id)
{
    for (const auto klineType: _klineTypes)
    {
        //Добавляем KLine
        KLineID klineID;
        klineID.symbol = id.symbol;
        klineID.type = klineType;

        const auto moneyKLine_it = _moneyKLine.find(klineID);
        Q_ASSERT(moneyKLine_it != _moneyKLine.end());

        moneyKLine_it.value()->stop();
        moneyKLine_it.value()->deleteLater();

        _moneyKLine.erase(moneyKLine_it);
    }

    emit sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("GATE: Delisting money: %1").arg(id.symbol));
}


void MexcStockExchange::loadLastStopKLine()
{
    const auto queryText =
        QString("SELECT MONEY, KLINE_INTERVAL, LAST_CLOSE_DATE_TIME "
                "FROM %1.KLINES "
                "WHERE STOCK_EXCHANGE = '%2'")
            .arg(_db.databaseName())
            .arg(_id.name);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return;
    }

    _moneyLastStopKLine.clear();
    while(query.next())
    {
        KLineID id;
        id.symbol = query.value("MONEY").toString();
        id.type = stringToKLineType(query.value("KLINE_INTERVAL").toString());
        _moneyLastStopKLine.insert(id, query.value("LAST_CLOSE_DATE_TIME").toDateTime());
    }

    _db.commit();
}
