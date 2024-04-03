#include "stockexchange.h"

using namespace TraidingCatBot;

StockExchange::StockExchange(const StockExchangeData& data, QObject *parent)
    : QObject{parent}
    , _data(data)
{
    qRegisterMetaType<TraidingCatBot::StockExchangeKLine>("StockExchangeKLine");
}

const StockExchangeData& StockExchange::getData() const
{
    return _data;
}

void StockExchange::start()
{
    startStockExchange();
}

void TraidingCatBot::StockExchange::stop()
{
    stopStockExchange();

    emit finished();
}

void StockExchange::getKLine(const KLine &kline)
{
    StockExchangeKLine tmp;
    tmp.kline = kline;
    tmp.stockExcahnge = getData().id;

    emit getKLine(tmp);
}


/*
QDateTime serverTime(const QByteArray &data)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error:" << error.errorString();

        return {};
    }

    return QDateTime::fromMSecsSinceEpoch(doc["serverTime"].toInteger());
}

void Core::detectKLine(const QString &symbol, const Mexc::KLine &kline)
{
    qDebug() << QTime::currentTime().toString("HH:mm:ss")
             << "(!)Detect:" << symbol
             << "delta:" << Mexc::deltaKLine(kline)
             << "Volume:" << Mexc::volumeKLine(kline);
}
*/
