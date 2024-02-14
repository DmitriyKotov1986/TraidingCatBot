#ifndef MEXCKLINE_H
#define MEXCKLINE_H

//Qt
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QVector>

//My
#include "money.h"
#include "types.h"

namespace TraidingCatBot
{

class MexcKLine final
    : public MoneyKLine
{
public:
    MexcKLine() = delete;

    explicit MexcKLine(const StockExchangeInfo& stockExchangeInfo, const KLineID& id, QObject *parent = nullptr);
    ~MexcKLine();

protected:
    virtual void startKLine() override;
    virtual void stopKLine() override;
    virtual KLines parseKLines(const QByteArray& data) override;
    virtual QUrl getUrl(quint16 count) override;
    virtual Common::HTTPSSLQuery::Headers getHeaders() override;

private:
    Common::HTTPSSLQuery::Headers _headers;

};

} //namespace TraidingCatBot

#endif
