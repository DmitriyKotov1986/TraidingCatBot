//QT
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QRandomGenerator64>

//My
#include "gatekline.h"

using namespace TradingCat;
using namespace Common;

static const quint64 TOO_MANY_REQUEST_CODE = 429;
static const quint64 ACCEESS_DENIED_CODE = 403;

GateKLine::GateKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, Common::HTTPSSLQuery *httpSSLQuery, QObject *parent /*= nullptr */)
    : _id(id)
    , _httpSSLQuery(httpSSLQuery)
    , _lastKLineCloseTime(lastKLineCloseTime)
{
    Q_CHECK_PTR(_httpSSLQuery);

    qRegisterMetaType<TradingCat::KLinesList>("KLinesList");

    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});
    _headers.insert(QByteArray{"User-Agent"}, QCoreApplication::applicationName().toUtf8());
}

GateKLine::~GateKLine()
{
    delete _timer;
}

void GateKLine::start()
{
    if (_id.type == KLineType::UNKNOW)
    {
        return;
    }

    QObject::connect(_httpSSLQuery, SIGNAL(getAnswer(const QByteArray&, quint64)),
                     SLOT(getAnswerHTTP(const QByteArray&, quint64)));
    QObject::connect(_httpSSLQuery, SIGNAL(errorOccurred(QNetworkReply::NetworkError, quint64, const QString&, quint64)),
                     SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, quint64, const QString&, quint64)));

    _timer = new QTimer();

    QObject::connect(_timer, SIGNAL(timeout()), SLOT(run()));

    const auto interval = static_cast<quint64>(_id.type);

    _timer->start(interval);

    run();
}

void GateKLine::stop()
{
    if (_timer != nullptr)
    {
        _timer->stop();
    }
}

KLinesList GateKLine::parseKLines(const QByteArray &data)
{
    KLinesList result;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        emit errorOccurred(_id, QString("GATE: JSON parse error: %1")
                                    .arg(error.errorString()));

        return result;
    }

    const auto errCode = doc["label"].toString();
    if (!errCode.isEmpty())
    {
        if (errCode == "INVALID_CURRENCY")
        {
            emit delisting(_id);

            return result;
        }
        else
        {
            emit errorOccurred(_id, QString("GATE: Stock excenge server error. Code: %1. Message: %2")
                                        .arg(errCode)
                                        .arg(doc["message"].toString()));
            return result;
        }
    }

    for (const auto& kline: doc.array())
    {
        const auto data = kline.toArray();

        const auto openDateTime = QDateTime::fromSecsSinceEpoch(data[0].toString().toLongLong());
        const auto closeDateTime = openDateTime.addMSecs(static_cast<quint64>(_id.type));

        if (closeDateTime > _lastKLineCloseTime)
        {
            KLine tmp;
            tmp.openTime = openDateTime;
            tmp.quoteAssetVolume = data[1].toString().toDouble();
            tmp.close = data[2].toString().toDouble();
            tmp.high = data[3].toString().toDouble();
            tmp.low = data[4].toString().toDouble();
            tmp.open = data[5].toString().toDouble();
            tmp.volume = data[6].toString().toDouble();
            tmp.closeTime = closeDateTime;

            tmp.id = _id;

            result.emplaceBack(std::move(tmp));
        }
    }

    if (result.size() < 2)
    {
        result.clear();

        return result;
    }

    std::sort(result.begin(), result.end(),
              [](const auto& kline1, const auto& kline2)
              {
                  return kline1.closeTime < kline2.closeTime;
              });

    result.erase(std::prev(result.end()));

    _lastKLineCloseTime = std::max(_lastKLineCloseTime, std::prev(result.end())->closeTime);

    return result;
}

QString GateKLine::KLineTypeToString(KLineType type)
{
    switch (type)
    {
    case KLineType::MIN1: return "1m";
    case KLineType::MIN5: return "5m";
    case KLineType::MIN15: return "15m";
    case KLineType::MIN30: return "30m";
    case KLineType::MIN60: return "1h";
    case KLineType::HOUR4: return "4h";
    case KLineType::HOUR8: return "8h";
    case KLineType::DAY1: return "1d";
    case KLineType::WEEK1: return "1w";
    default:
        Q_ASSERT(false);
    }

    return "UNKNOW";
}

QUrl GateKLine::getUrl(quint32 count) const
{
    Q_ASSERT(count > 0 && count <= 1000);

    return QUrl{QString("https://api.gateio.ws/api/v4/spot/candlesticks?currency_pair=%1&interval=%2&limit=%3")
                    .arg(_id.symbol)
                    .arg(GateKLine::KLineTypeToString(_id.type))
                    .arg(count)};
}

void GateKLine::getAnswerHTTP(const QByteArray &answer, quint64 id)
{
    Q_CHECK_PTR(_timer);

    if (id != _currentRequestID)
    {
        return;
    }

    _currentRequestID = 0;

    auto klines = parseKLines(answer);
    if (klines.isEmpty())
    {
        return;
    }

    emit getKLines(klines);

    if (_HTTPRequestAfterError > 0)
    {
        --_HTTPRequestAfterError;
        if (_HTTPRequestAfterError == 0)
        {
            _HTTPRequestAfterError = -1;
            const auto interval = static_cast<quint64>(_id.type);
            _timer->setInterval(interval);
        }
    }
}

void GateKLine::errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString &msg, quint64 id)
{
    Q_CHECK_PTR(_timer);

    if (id != _currentRequestID)
    {
        return;
    }

    emit errorOccurred(_id, msg);

    if ((_id.type == KLineType::MIN1) || (_id.type == KLineType::MIN5))
    {
        if ((serverCode == TOO_MANY_REQUEST_CODE) ||  (serverCode == ACCEESS_DENIED_CODE))
        {
            const auto interval = static_cast<quint64>(_id.type);
            _timer->setInterval(_timer->interval() + interval);
            _HTTPRequestAfterError = QRandomGenerator64::global()->bounded(30, 300);
        }
    }

    _currentRequestID = 0;
}

void GateKLine::run()
{
    if (_currentRequestID != 0) //предыдущая передача еще не заончилась, поэтому - пропускаем такт
    {
        return;
    }

    quint32 count = (_lastKLineCloseTime.msecsTo(QDateTime::currentDateTime()) / static_cast<quint64>(_id.type)) + 2;
    if (count > 1000)
    {
        count = 1000;
    }

    _currentRequestID = _httpSSLQuery->send(getUrl(count), HTTPSSLQuery::RequestType::GET, "", _headers);
}


