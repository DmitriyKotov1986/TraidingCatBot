#include "dbsaver.h"

using namespace TradingCat;
using namespace Common;

DBSaver::DBSaver(const Common::DBConnectionInfo& dbConnectionInfo, QObject *parent)
    : QObject{parent}
    , _dbConnectionInfo(dbConnectionInfo)
{
}

DBSaver::~DBSaver()
{
    stop();
}

void DBSaver::start()
{
    if (!Common::connectToDB(_db, _dbConnectionInfo, "TraidongCatBotDB"))
    {
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, connectDBErrorString(_db));

        return;
    }

    loadKLineTables();
}

void DBSaver::stop()
{
    if (_db.isOpen())
    {
        saveToDB();

        _db.close();
    }

    emit finished();
}

void DBSaver::getKLines(const StockExchangeKLinesList &klines)
{
    for (const auto& kline: klines)
    {
        const auto tableName = getKLineTableName(kline.stockExcahnge.name, kline.kline.id.symbol, KLineTypeToString(kline.kline.id.type));

        auto moneyID = checkKLineTable(kline.stockExcahnge, kline.kline.id);
        Q_ASSERT(moneyID != 0);
        if (moneyID == 0)
        {
            return;
        }

        const QString closeDateTime = kline.kline.closeTime.toString(DATETIME_FORMAT);

        const auto insertText =
            QString("INSERT DELAYED INTO %1.`%2` (MONEY_ID, DATE_TIME, OPEN_DATE_TIME, OPEN, HIGH, LOW, CLOSE, VOLUME, CLOSE_DATE_TIME, QUOTE_ASSET_VOLUME) "
                    "VALUES (%3, '%4', '%5', %6, %7, %8, %9, %10, '%11', %12)")
                .arg(_db.databaseName())
                .arg(tableName)
                .arg(moneyID)
                .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
                .arg(kline.kline.openTime.toString(DATETIME_FORMAT))
                .arg(kline.kline.open)
                .arg(kline.kline.high)
                .arg(kline.kline.low)
                .arg(kline.kline.close)
                .arg(kline.kline.volume)
                .arg(closeDateTime)
                .arg(kline.kline.quoteAssetVolume);

        _insertQuery.emplace(std::move(insertText));

        const auto updateText =
            QString("UPDATE %1.`KLINES` "
                    "SET LAST_CLOSE_DATE_TIME = '%2' "
                    "WHERE ID = %3")
                .arg(_db.databaseName())
                .arg(closeDateTime )
                .arg(moneyID);

        _updateQuery.emplace(std::move(updateText));

    }
    if (_updateQuery.size() > 500)
    {
        saveToDB();
    }
}

void DBSaver::loadKLineTables()
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
        emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

        _db.rollback();

        return;
    }

    _klineTables.clear();
    _tablesList.clear();
    while(query.next())
    {
        const auto stockExchange = query.value("STOCK_EXCHANGE").toString();
        const auto money = query.value("MONEY").toString();
        const auto interval = query.value("KLINE_INTERVAL").toString();

        addMoneyID(stockExchange, money, interval, query.value("ID").toULongLong());

        _tablesList.insert(getKLineTableName(stockExchange, money, interval));
    }

    _db.commit();
}

quint64 DBSaver::checkKLineTable(const StockExchangeID &stockExcangeID, const KLineID klineID)
{
    const auto tableName = getKLineTableName(stockExcangeID.name, klineID.symbol, KLineTypeToString(klineID.type));

    if (!_tablesList.contains(tableName))
    {
        const auto queryText =
            QString("CREATE TABLE %1.`%2` ( "
                    "`ID` int NOT NULL AUTO_INCREMENT, "
                    "`MONEY_ID` int NOT NULL DEFAULT '0', "
                    "`DATE_TIME` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "`OPEN_DATE_TIME` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "`OPEN` double NOT NULL DEFAULT '0', "
                    "`HIGH` double NOT NULL DEFAULT '0', "
                    "`LOW` double NOT NULL DEFAULT '0', "
                    "`CLOSE` double NOT NULL DEFAULT '0', "
                    "`VOLUME` double NOT NULL DEFAULT '0', "
                    "`CLOSE_DATE_TIME` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "`QUOTE_ASSET_VOLUME` double NOT NULL DEFAULT '0', "
                    "PRIMARY KEY (`ID`), "
                    "UNIQUE KEY `idx_KLINE_TMP_MONEY_ID_OPEN_DATE_TIME` (`MONEY_ID`,`OPEN_DATE_TIME`), "
                    "UNIQUE KEY `idx_KLINE_TMP_MONEY_ID_CLOSE_DATE_TIME` (`MONEY_ID`,`CLOSE_DATE_TIME`) "
                    ") ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci ")
                .arg(_db.databaseName())
                .arg(tableName);

        writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

        _db.transaction();
        QSqlQuery query(_db);

        if (!query.exec(queryText))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

            _db.rollback();

            return 0;
        }

        _db.commit();
    }

    auto id = getMoneyID(stockExcangeID, klineID);
    if (id == 0)
    {
        const auto queryText =
            QString("INSERT INTO %1.KLINES (STOCK_EXCHANGE, MONEY, KLINE_INTERVAL, LAST_CLOSE_DATE_TIME) "
                    "VALUES('%2', '%3', '%4', '%5') ")
                .arg(_db.databaseName())
                .arg(stockExcangeID.name)
                .arg(klineID.symbol)
            .   arg(KLineTypeToString(klineID.type))
                .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT));

        writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

        _db.transaction();
        QSqlQuery query(_db);

        if (!query.exec(queryText))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

            _db.rollback();

            return 0;
        }

        _db.commit();

        loadKLineTables();

        id = getMoneyID(stockExcangeID, klineID);
    }

    return id;
}

quint64 DBSaver::getMoneyID(const StockExchangeID &stockExchangeID, const KLineID klineID)
{
    const auto klineTables_it = _klineTables.find(stockExchangeID);
    if (klineTables_it == _klineTables.end())
    {
        return 0;
    }

    const auto& money = klineTables_it.value();
    const auto money_it = money.find(klineID);
    if (money_it == money.end())
    {
        return 0;
    }

    return money_it.value();
}

void DBSaver::addMoneyID(const QString& stockExchange, const QString& symbol, const QString& type, quint64 id)
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


    auto klineTables_it = _klineTables.find(stockExchangeID);
    if (klineTables_it == _klineTables.end())
    {
        MoneyMap tmp;
        tmp.insert(klineID, id);

        _klineTables.insert(stockExchangeID, tmp);
    }
    else
    {
        klineTables_it.value().insert(klineID, id);
    }
}

void DBSaver::saveToDB()
{
    _db.transaction();
    QSqlQuery query(_db);

    while (!_insertQuery.empty())
    {
        const auto& queryText = _insertQuery.front();

        writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

        if (!query.exec(queryText))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

            _db.rollback();

            return;
        }

        _insertQuery.pop();
    }

    while (!_updateQuery.empty())
    {
        const auto& queryText = _updateQuery.front();

        writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

        if (!query.exec(queryText))
        {
            emit sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, executeDBErrorString(_db, query));

            _db.rollback();

            return;
        }

        _updateQuery.pop();
    }

    _db.commit();
}
