#include <QList>
#include <QMutexLocker>
#include <QHash>

#include "history.h"

using namespace TradingCat;
using namespace Common;

static const quint32 COUNT_SAVE_KLINES = 300;

History::History(const Common::DBConnectionInfo &dbConnectionInfo)
    : _dbConnectionInfo(dbConnectionInfo)
{
    if (!Common::connectToDB(_db, _dbConnectionInfo, "HistoryTraidongCatBotDB"))
    {
        _errorString = connectDBErrorString(_db);

        return;
    }

    loadFromDB();
}

History::~History()
{
    if (_db.isOpen())
    {
        _db.close();
    }
}

void History::add(const StockExchangeKLine &kline)
{
    QMutexLocker<QMutex> locker(&_mutex);

    StockExchangeKLine tmp(kline);
    add(std::move(tmp));
}

void History::add(StockExchangeKLine &&kline)
{
    auto closeDateTime(kline.kline.closeTime);
    auto klineID(kline.kline.id);
    auto stockExchangeID(kline.stockExcahnge);
    auto kline_ptr = std::make_unique<KLine>(std::move(kline.kline));

    auto stockExchaneHistory_it = _history.find(stockExchangeID);
    if (stockExchaneHistory_it == _history.end())
    {

        KLineHistoryPtr klineHistory;
        klineHistory.emplace(std::move(closeDateTime), std::move(kline_ptr));

        KLineHistories klineHistories;
        klineHistories.emplace(std::move(klineID), std::move(klineHistory));

        _history.emplace(std::move(stockExchangeID), std::move(klineHistories));

        return;
    }

    auto& stockExchaneHistory = stockExchaneHistory_it->second;
    auto klineHistories_it = stockExchaneHistory.find(klineID);
    if (klineHistories_it == stockExchaneHistory.end())
    {
        KLineHistoryPtr klineHistory;
        klineHistory.emplace(std::move(closeDateTime), std::move(kline_ptr));

        stockExchaneHistory.emplace(std::move(klineID), std::move(klineHistory));

        return;
    }

    auto& klineHistory = klineHistories_it->second;
    klineHistory.emplace(std::move(closeDateTime), std::move(kline_ptr));

    if (klineHistory.size() > COUNT_SAVE_KLINES)
    {
        klineHistory.erase(klineHistory.begin());
    }
}

History::KLineHistory History::getHistory(const StockExchangeID &stockExchangeId, const KLineID &klineID, const QDateTime& startDateTime)
{
    QMutexLocker<QMutex> locker(&_mutex);

    History::KLineHistory result;

    auto stockExchaneHistory_it = _history.find(stockExchangeId);
    if (stockExchaneHistory_it == _history.end())
    {
        return result;
    }

    const auto& stockExchaneHistory = stockExchaneHistory_it->second;
    auto klineHistories_it = stockExchaneHistory.find(klineID);
    if (klineHistories_it == stockExchaneHistory.end())
    {
        return result;
    }

    const auto& klineHistory = klineHistories_it->second;
    if (klineHistory.empty())
    {
        Q_ASSERT(false);
        return result;
    }

    for (auto klineHistory_it = klineHistory.rbegin(); klineHistory_it != klineHistory.rend(); ++klineHistory_it)
    {
        if (klineHistory_it->first <= startDateTime)
        {
            break;
        }
        result.push_back(*(klineHistory_it->second));
    }

    return result;
}

QString History::errorString()
{
    const QString tmp = _errorString;
    _errorString.clear();

    return tmp;
}

bool History::isError() const
{
    return !_errorString.isEmpty();
}

void History::loadFromDB()
{
    Q_ASSERT(_db.isOpen());

    loadKLineTables();

    if (_klineTables.empty())
    {
        return;
    }

    QHash<QString, quint64> tableList;
    for (const auto& [_, money]: _klineTables)
    {
            tableList.emplace(getKLineTableName(money.stockExchangeID.name,
                                            money.klineID.symbol, KLineTypeToString(money.klineID.type)), static_cast<quint64>(money.klineID.type));
    }

    QString queryText;
    bool first = true;

    for (auto tableList_it = tableList.begin(); tableList_it != tableList.end(); ++tableList_it)
    {     
        if (!first)
        {
            queryText += " UNION ";
        }
        first = false;

        queryText +=
            QString("(SELECT MONEY_ID, OPEN_DATE_TIME, OPEN, HIGH, LOW, CLOSE, VOLUME, CLOSE_DATE_TIME, QUOTE_ASSET_VOLUME "
                    "FROM %1.`%2` "
                    "WHERE CLOSE_DATE_TIME > '%3'"
                    "ORDER BY CLOSE_DATE_TIME DESC)")
                .arg(_db.databaseName())
                .arg(tableList_it.key())
                .arg(QDateTime::currentDateTime().addMSecs(-(tableList_it.value() * (COUNT_SAVE_KLINES + 5))).toString(DATETIME_FORMAT));
    }

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        _errorString = executeDBErrorString(_db, query);
        _db.rollback();

        return;
    }

    while(query.next())
    {
        const auto id = query.value("MONEY_ID").toULongLong();
        Q_ASSERT(id != 0);

        const auto& moneyInfo = getMoneyInfo(id);

        StockExchangeKLine tmp;
        tmp.stockExcahnge = moneyInfo.stockExchangeID;
        tmp.kline.id = moneyInfo.klineID;
        tmp.kline.openTime = query.value("OPEN_DATE_TIME").toDateTime();
        tmp.kline.closeTime = query.value("CLOSE_DATE_TIME").toDateTime();
        tmp.kline.open = query.value("OPEN").toDouble();
        tmp.kline.close = query.value("CLOSE").toDouble();
        tmp.kline.high = query.value("HIGH").toDouble();
        tmp.kline.low = query.value("LOW").toDouble();
        tmp.kline.volume = query.value("VOLUME").toDouble();
        tmp.kline.quoteAssetVolume = query.value("QUOTE_ASSET_VOLUME").toDouble();

        add(std::move(tmp));
    }

    _db.commit();
}

static History::MoneyInfo defultMoneyInfo;

const History::MoneyInfo& History::getMoneyInfo(quint64 id) const
{
    Q_ASSERT(id != 0);

    const auto klineTables_it = _klineTables.find(id);
    if (klineTables_it == _klineTables.end())
    {
        return defultMoneyInfo;
    }

    return klineTables_it->second;
}

void History::addMoneyInfo(const QString& stockExchange, const QString& symbol, const QString& type, quint64 id)
{
    Q_ASSERT(!stockExchange.isEmpty());
    Q_ASSERT(!symbol.isEmpty());
    Q_ASSERT(!type.isEmpty());
    Q_ASSERT(id != 0);

    StockExchangeID stockExchangeID;
    stockExchangeID.name = stockExchange;

    KLineID klineID;
    klineID.symbol = symbol;
    klineID.type = stringToKLineType(type);

    MoneyInfo tmp;
    tmp.klineID = std::move(klineID);
    tmp.stockExchangeID = std::move(stockExchangeID);

    _klineTables.insert({id, tmp});
}

void History::loadKLineTables()
{
    const auto queryText =
        QString("SELECT ID, STOCK_EXCHANGE, MONEY, KLINE_INTERVAL "
                "FROM %1.`KLINES`")
            .arg(_db.databaseName());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        _errorString = executeDBErrorString(_db, query);

        _db.rollback();

        return;
    }

    _klineTables.clear();
    while(query.next())
    {
        addMoneyInfo(query.value("STOCK_EXCHANGE").toString(), query.value("MONEY").toString(),
                   query.value("KLINE_INTERVAL").toString(), query.value("ID").toULongLong());
    }

    _db.commit();
}
