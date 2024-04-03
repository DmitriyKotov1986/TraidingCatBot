#ifndef HISTORY_H
#define HISTORY_H

//STL
#include <map>
#include <unordered_map>
#include <memory>

//Qt
#include <QList>
#include <QSqlDatabase>
#include <QMutex>

#include "Common/common.h"
#include "types.h"

namespace TradingCat
{

class History
{
public:
    using KLineHistory = QList<KLine>;

    struct MoneyInfo
    {
        StockExchangeID stockExchangeID;
        KLineID klineID;
    };

public:
    History() = delete;
    History(const  History&) = delete;
    History& operator=(const  History&) = delete;
    History(History&&) = delete;
    History& operator=(History&&) = delete;

    History(const Common::DBConnectionInfo& dbConnectionInfo);
    ~History();

    void add(const TradingCat::StockExchangeKLine& kline);
    KLineHistory getHistory(const StockExchangeID& stockExchangeId, const KLineID& klineID, const QDateTime& startDateTime);

    QString errorString();
    bool isError() const;

private:
    using KLineHistoryPtr = std::map<QDateTime, std::unique_ptr<KLine>>;
    using KLineHistories = std::unordered_map<KLineID, KLineHistoryPtr>;
    using StockExchaneHistory = std::unordered_map<StockExchangeID, KLineHistories>;

    using StockExchangeMap = std::unordered_map<quint64, MoneyInfo>;

private:
    void loadFromDB();
    void add(TradingCat::StockExchangeKLine&& kline);

    void loadKLineTables();
    void addMoneyInfo(const QString& stockExhcange, const QString& symbol, const QString& type, quint64 id);
    const History::MoneyInfo& getMoneyInfo(quint64 id) const;

private:
    const Common::DBConnectionInfo _dbConnectionInfo;
    QSqlDatabase _db;

    QString _errorString;

    QMutex _mutex;

    StockExchaneHistory _history;
    StockExchangeMap _klineTables;

};

} //namespace TraidingCatBot

#endif // HISTORY_H
