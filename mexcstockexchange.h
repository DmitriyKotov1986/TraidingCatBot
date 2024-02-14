#ifndef MEXCSTOCKEXCHANGE_H
#define MEXCSTOCKEXCHANGE_H

#include <QObject>

#include "types.h"

#include "stockexchange.h"

namespace TraidingCatBot
{

class MexcStockExchange
    : public StockExchange
{
    Q_OBJECT
public:
    MexcStockExchange() = delete;

    explicit MexcStockExchange(const StockExchangeID& id, QObject *parent = nullptr);

protected:
    virtual void startStockExchange() override;
    virtual void stopStockExchange() override;

};

} // //namespace TraidingCatBot

#endif // MEXCSTOCKEXCHANGE_H
