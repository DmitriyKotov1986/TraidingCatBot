#ifndef STOCKEXCHANGE_H
#define STOCKEXCHANGE_H

#include <QObject>

//My
#include "types.h"
#include "money.h"

namespace TraidingCatBot
{

class StockExchange
    : public QObject
{
    Q_OBJECT

public:
    StockExchange() = delete;

    explicit StockExchange(const StockExchangeID& id, QObject *parent = nullptr);
    virtual ~StockExchange() = default;

public slots:
    void start();
    void stop();

signals:
    void errorOccurred(const StockExchangeID& id, const QString& msg);
    void finished();
    void getKLine(const TraidingCatBot::KLine& KLine);

protected:
    virtual void startStockExchange() = 0;
    virtual void stopStockExchange() = 0;

private:
    quint64 _currentRequestID = 0;

    const StockExchangeID _id;
};



/*inline static const QUrl DEFAULT_SYMBOL_URL{"https://api.mexc.com/api/v3/defaultSymbols"};
QStringList defaultSymbol(const QByteArray& data);

inline static const QUrl PING_URL{"https://api.mexc.com/api/v3/ping"};

inline static const QUrl SERVER_TIME_URL{"https://api.mexc.com/api/v3/time"};
QDateTime serverTime(const QByteArray& data);


*/
} //namespace TraidingCatBot

#endif // STOCKEXCHANGE_H
