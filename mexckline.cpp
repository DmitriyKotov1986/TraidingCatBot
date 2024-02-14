//QT
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

//My
#include "mexckline.h"

using namespace TraidingCatBot;
using namespace Common;

MexcKLine::MexcKLine(const StockExchangeInfo& stockExchangeInfo, const KLineID& id, QObject *parent /*= nullptr */)
    : MoneyKLine(stockExchangeInfo, id, parent)
{
    _headers.insert(QByteArray{"X-MEXC-APIKEY"}, QByteArray{""});
    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});
}

MexcKLine::~MexcKLine()
{
}

KLines MexcKLine::parseKLines(const QByteArray &data)
{
    KLines result;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        emit errorOccurred(getID(), QString("MexcKLine::parseKLines: JSON parse error: %1")
                                    .arg(error.errorString()));

        return result;
    }

    const auto object = doc.array();

    for (const auto& kline: object)
    {
        const auto data = kline.toArray();
        KLine tmp;
        tmp.openTime = QDateTime::fromMSecsSinceEpoch(data[0].toInteger());
        tmp.open = data[1].toString().toDouble();
        tmp.high = data[2].toString().toDouble();
        tmp.low = data[3].toString().toDouble();
        tmp.close = data[4].toString().toDouble();
        tmp.volume = data[5].toString().toDouble();
        tmp.closeTime = QDateTime::fromMSecsSinceEpoch(data[6].toInteger());
        tmp.quoteAssetVolume = data[7].toString().toDouble();
        tmp.id = getID();

        result.push_back(tmp);
    }

    return result;
}

QUrl MexcKLine::getUrl(quint16 count)
{
    Q_ASSERT(count > 0 && count <= 2000);

    const auto& id = getID();

    return QUrl{QString("https://api.mexc.com/api/v3/klines?symbol=%1&interval=&2&limit=%3")
                    .arg(id.symbol)
                    .arg(KLineTypeToString(id.type))
                    .arg(count)};
}

HTTPSSLQuery::Headers MexcKLine::getHeaders()
{
    return _headers;
}


void MexcKLine::startKLine()
{
}

void MexcKLine::stopKLine()
{
}
