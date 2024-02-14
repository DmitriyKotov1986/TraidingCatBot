#include "stockexchange.h"

using namespace TraidingCatBot;

StockExchange::StockExchange(const StockExchangeID& id, QObject *parent)
    : QObject{parent}
    , _id(id)
{

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

/*QStringList TraidingCatBot::defaultSymbol(const QByteArray& data)
{
    QStringList result;

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error:" << error.errorString();

        return result;
    }

    const auto errCode = doc["code"].toInt();
    if (errCode != 0)
    {
        qDebug() << "Error Mexc::defaultSymbol. Code:" << errCode << "Msg:" << doc["msg"].toString();

        return result;
    }

    const auto symbolsList = doc["data"].toArray();
    for (const auto& symbol: symbolsList)
    {
        const auto& tmp = symbol.toString();
        if (!tmp.isEmpty())
        {
            result.push_back(tmp);
        }
    }

    return result;
}

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

*/

/*for (const auto& mexc_ptr: _money.values())
{
    delete mexc_ptr;
}

for (auto& money: _money)
{
    delete money;
}



void Core::sendRequest()
{
    HTTPSSLQuery::Headers headers;
    headers.insert(QByteArray{"X-MEXC-APIKEY"}, QByteArray{""});
    headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});

    _currentRequestID = HTTPSSLQuery::getId();
    emit sendHTTP(DEFAULT_SYMBOL_URL, HTTPSSLQuery::RequestType::GET, "", headers, _currentRequestID);
}

void Core::makeMoney(const QStringList& symbolsList)
{

    if (symbolsList.isEmpty())
    {
        emit finished();
    }

    for (const auto& symbol: symbolsList)
    {
        Mexc* mexc = new Mexc(symbol);

        QObject::connect(mexc, SIGNAL(detectKLine(const QString&, const TraidingCatBot::Mexc::KLine&)),
                         SLOT(detectKLine(const QString&, const TraidingCatBot::Mexc::KLine&)));

        _money.insert(symbol, mexc);

        QTimer::singleShot(_rg->bounded(1, 60000 * 5), [mexc](){ mexc->start(); });

        //      qDebug() << "Add: " <<  symbol;
    }
    qDebug() << "Total add:" << symbolsList.size();
}

void Core::detectKLine(const QString &symbol, const Mexc::KLine &kline)
{
    qDebug() << QTime::currentTime().toString("HH:mm:ss")
             << "(!)Detect:" << symbol
             << "delta:" << Mexc::deltaKLine(kline)
             << "Volume:" << Mexc::volumeKLine(kline);
}
*/
