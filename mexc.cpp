//QT
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

#include "mexc.h"

using namespace TraidingCatBot;

QStringList TraidingCatBot::defaultSymbol(const QByteArray& data)
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


double Mexc::deltaKLine(const KLine &kline)
{
    return ((kline.high - kline.low) / kline.low) * 100.0;
}

double Mexc::volumeKLine(const KLine &kline)
{
    return ((kline.open + kline.close) / 2) * kline.volume;
}

Mexc::KLines Mexc::parseKLines(const QByteArray &data)
{
    KLines result;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error:" << error.errorString();

        return result;
    }

    if (doc.isObject())
    {
        qDebug() << "Error Mexc::parseKLines. Code:" << doc["code"].toInt() << "Msg:" << doc["msg"].toString();

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

        result.push_back(tmp);
    }

    return result;
}

Mexc::Mexc(const QString &symbol, QObject *parent)
    : QObject{parent}
    , _symbol(symbol)
{
    qRegisterMetaType<TraidingCatBot::Mexc::KLine>("KLine");
    qRegisterMetaType<TraidingCatBot::Mexc::KLines>("KLines");

    _headers.insert(QByteArray{"X-MEXC-APIKEY"}, QByteArray{""});
    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});

    _klineUrl = QUrl{QString("https://api.mexc.com/api/v3/klines?symbol=%1&interval=1m&limit=5").arg(_symbol)};
}

Mexc::~Mexc()
{
    delete _timer;
}

void Mexc::start()
{
    auto HTTPSSLQuery = HTTPSSLQuery::create();

    QObject::connect(this, SIGNAL(sendHTTP(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)),
                     HTTPSSLQuery, SLOT(send(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(getAnswer(const QByteArray&, quint64)),
                     SLOT(getAnswerHTTP(const QByteArray&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(errorOccurred(QNetworkReply::NetworkError, const QString&, quint64)),
                     SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, const QString&, quint64)), Qt::QueuedConnection);

    _timer = new QTimer();

    QObject::connect(_timer, SIGNAL(timeout()), SLOT(run()));

    _timer->start(60000 * 5);

    run();
}

void Mexc::getAnswerHTTP(const QByteArray &answer, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    const auto klines = parseKLines(answer);
    for (const auto& kline: klines)
    {
        if (deltaKLine(kline) >= 2.0 && volumeKLine(kline) > 50.0)
        {
            emit detectKLine(_symbol, kline);
        }
    }
}

void Mexc::errorOccurredHTTP(QNetworkReply::NetworkError code, const QString &msg, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    qDebug() << msg;
}

void Mexc::run()
{
    _currentRequestID = HTTPSSLQuery::getId();

    emit sendHTTP(_klineUrl, HTTPSSLQuery::RequestType::GET, "", _headers, _currentRequestID);
}

