#ifndef DBSAVER_H
#define DBSAVER_H

//STL
#include <queue>

//QT
#include <QObject>
#include <QSqlDatabase>
#include <QQueue>

//My
#include "Common/common.h"
#include "Common/tdbloger.h"
#include "types.h"

namespace TradingCat
{

class DBSaver final
    : public QObject
{
    Q_OBJECT

public:
    DBSaver() = delete;

    explicit DBSaver(const Common::DBConnectionInfo& dbConnectionInfo, QObject *parent = nullptr);
    ~DBSaver();

public slots:
    void start();
    void stop();

    void getKLines(const TradingCat::StockExchangeKLinesList& klines);

signals:
    void finished();
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg);

private:
    void loadKLineTables();
    quint64 checkKLineTable(const StockExchangeID& stockExchangeID, const KLineID klineID);
    void addMoneyID(const QString& stockExhcange, const QString& symbol, const QString& type, quint64 id);
    quint64 getMoneyID(const StockExchangeID& stockExchangeID, const KLineID klineID);
    void saveToDB();

private:
    using MoneyMap = QHash<KLineID, quint64>;
    using StockExchangeMap = QHash<StockExchangeID, MoneyMap>;

private:
    const Common::DBConnectionInfo _dbConnectionInfo;
    StockExchangeMap _klineTables;
    QSet<QString> _tablesList;
    QSqlDatabase _db;

    std::queue<QString> _insertQuery;
    std::queue<QString> _updateQuery;

};

} //namespace TraidingCatBot

#endif // DBSAVER_H
