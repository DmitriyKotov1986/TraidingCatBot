#ifndef GATESTOCKECHAN_H
#define GATESTOCKECHAN_H

//QT
#include <QObject>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QSqlDatabase>

//My
#include "Common/httpsslquery.h"
#include "Common/tdbloger.h"
#include "types.h"
#include "gatekline.h"

namespace TradingCat
{

class GateStockExchange
    : public QObject
{
    Q_OBJECT

private:
    enum class RequestType
    {
        NONE,
        DEFAULT_SYMBOLS
    };

    using MoneySymbols = QSet<QString>;

public:
    GateStockExchange() = delete;

    explicit GateStockExchange(const Common::DBConnectionInfo& dbConnectionInfo, Common::HTTPSSLQuery::ProxyList proxyList, QObject *parent = nullptr);

public slots:
    void start();
    void stop();

signals:
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg);
    void finished();
    void getKLines(const TradingCat::StockExchangeKLinesList& kline);

private slots:
    void getAnswerHTTP(const QByteArray& answer, quint64 id);
    void errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString& msg, quint64 id);
    void errorOccurredMoneyKLine(const TradingCat::KLineID& id, const QString& msg);
    void getKLinesMoney(const TradingCat::KLinesList& klines);
    void sendUpdateMoney();
    void delisting(const TradingCat::KLineID& id);

private:
    MoneySymbols parseDefaultSymbol(const QByteArray& data);
    void makeMoney(const MoneySymbols& symbolsList);

    void loadLastStopKLine();

private:
    const Common::DBConnectionInfo _dbConnectionInfo;
    QSqlDatabase _db;
    Common::HTTPSSLQuery::ProxyList _proxyList;

    KLineTypes _klineTypes;
    StockExchangeID _id;

    Common::HTTPSSLQuery *_httpSSLQuery = nullptr;
    Common::HTTPSSLQuery::Headers _headers;
    quint64 _currentRequestID = 0;
    RequestType _currentRequestType = RequestType::NONE;

    QHash<KLineID, GateKLine*> _moneyKLine;
    QHash<KLineID, QDateTime> _moneyLastStopKLine;

    TradingCat::StockExchangeKLinesList _klines;

    QTimer *_updateMoneyTimer = nullptr;
};

} // //namespace TradingCat

#endif // GATESTOCKECHAN_H
