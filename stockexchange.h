
#ifndef STOCKEXCHANGE_H
#define STOCKEXCHANGE_H

#include <QObject>

//My
#include "types.h"


namespace TraidingCatBot
{

class StockExchange
    : public QObject
{
    Q_OBJECT

public:
    StockExchange() = delete;

    explicit StockExchange(const StockExchangeData& data, QObject *parent = nullptr);
    virtual ~StockExchange() = default;

    const StockExchangeData& getData() const;

public slots:
    void start();
    void stop();

private slots:




protected:
    virtual void startStockExchange() = 0;
    virtual void stopStockExchange() = 0;

private:

};

} //namespace TraidingCatBot

#endif // STOCKEXCHANGE_H
