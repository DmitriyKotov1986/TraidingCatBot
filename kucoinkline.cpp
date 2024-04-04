//QT
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QRandomGenerator64>

//My
#include "kucoinkline.h"

using namespace TradingCat;
using namespace Common;

static const quint64 TOO_MANY_REQUEST_CODE = 429;
static const quint64 ACCEESS_DENIED_CODE = 403;

KucoinKLine::KucoinKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, Common::HTTPSSLQuery *httpSSLQuery, QObject *parent /*= nullptr */)
    : _id(id)
    , _httpSSLQuery(httpSSLQuery)
    , _lastKLineCloseTime(lastKLineCloseTime)
{
    Q_CHECK_PTR(_httpSSLQuery);

    qRegisterMetaType<TradingCat::KLinesList>("KLinesList");

    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});
    _headers.insert(QByteArray{"User-Agent"}, QCoreApplication::applicationName().toUtf8());
}

KucoinKLine::~KucoinKLine()
{
    delete _timer;
}

void KucoinKLine::start()
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

    auto interval = static_cast<quint64>(_id.type);

    _timer->start(interval);

    run();
}

void KucoinKLine::stop()
{
    if (_timer != nullptr)
    {
        _timer->stop();
    }
}

KLinesList KucoinKLine::parseKLines(const QByteArray &data)
{
    KLinesList result;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        emit errorOccurred(_id, QString("KUCOIN: JSON parse error: %1")
                                    .arg(error.errorString()));

        return result;
    }

    const auto errCode = doc["code"].toString();
    if (errCode != "200000")
    {
        if (errCode == "400100")
        {
            emit delisting(_id);

            return result;
        }
        else
        {
            emit errorOccurred(_id, QString("KUCOIN: Stock excenge server error. Code: %1. Message: %2")
                                                            .arg(errCode)
                                                            .arg(doc["msg"].toString()));
            return result;
        }
    }

    for (const auto& kline: doc["data"].toArray())
    {
        const auto data = kline.toArray();

        const auto openDateTime = QDateTime::fromSecsSinceEpoch(data[0].toString().toLongLong());
        const auto closeDateTime = openDateTime.addMSecs(static_cast<quint64>(_id.type));

        if (closeDateTime > _lastKLineCloseTime)
        {
            KLine tmp;
            tmp.openTime = QDateTime::fromSecsSinceEpoch(data[0].toString().toLongLong());
            tmp.open = data[1].toString().toDouble();
            tmp.close = data[2].toString().toDouble();
            tmp.high = data[3].toString().toDouble();
            tmp.low = data[4].toString().toDouble();
            tmp.volume = data[5].toString().toDouble();
            tmp.quoteAssetVolume = data[6].toString().toDouble();
            tmp.closeTime = tmp.openTime.addMSecs(static_cast<quint64>(_id.type));

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

QString KucoinKLine::KLineTypeToString(KLineType type)
{
    switch (type)
    {
    case KLineType::MIN1: return "1min";
    case KLineType::MIN5: return "5min";
    case KLineType::MIN15: return "15min";
    case KLineType::MIN30: return "30min";
    case KLineType::MIN60: return "1hour";
    case KLineType::HOUR4: return "4hour";
    case KLineType::HOUR8: return "8hour";
    case KLineType::DAY1: return "1day";
    case KLineType::WEEK1: return "1week";
    default:
        Q_ASSERT(false);
    }

    return "UNKNOW";
}

QUrl KucoinKLine::getUrl(const QDateTime& start, const QDateTime& end)
{
    Q_ASSERT(start.msecsTo(end) < 1500 * static_cast<qint64>(_id.type));

   // if (_id.type == KLineType::MIN1 ) qDebug() << _id.symbol << start << end;

    return QUrl{QString("https://api.kucoin.com/api/v1/market/candles?type=%1&symbol=%2&startAt=%3&endAt=%4")
                    .arg(KucoinKLine::KLineTypeToString(_id.type))
                    .arg(_id.symbol)
                    .arg(start.toSecsSinceEpoch())
                    .arg(end.toSecsSinceEpoch())};
}

void KucoinKLine::getAnswerHTTP(const QByteArray &answer, quint64 id)
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

void KucoinKLine::errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString &msg, quint64 id)
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

void KucoinKLine::run()
{
    if (_currentRequestID != 0) //предыдущая передача еще не заончилась, поэтому - пропускаем такт
    {
        return;
    }

    auto start = _lastKLineCloseTime.addMSecs(-2 * static_cast<quint64>(_id.type));
    auto end = QDateTime::currentDateTime();

    if (start.msecsTo(end) > 1500 * static_cast<qint64>(_id.type))
    {
        start = end.addMSecs(-static_cast<quint64>(_id.type) * 1450);
    }

    _currentRequestID = _httpSSLQuery->send(getUrl(start, end), HTTPSSLQuery::RequestType::GET, "", _headers);
}


