//QT
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QRandomGenerator64>

//My
#include "mexckline.h"

using namespace TradingCat;
using namespace Common;

static const quint64 TOO_MANY_REQUEST_CODE = 429;
static const quint64 ACCEESS_DENIED_CODE = 403;

MexcKLine::MexcKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, Common::HTTPSSLQuery *httpSSLQuery, QObject *parent /*= nullptr */)
    : _id(id)
    , _httpSSLQuery(httpSSLQuery)
    , _lastKLineCloseTime(lastKLineCloseTime)
{
    Q_CHECK_PTR(_httpSSLQuery);

    qRegisterMetaType<TradingCat::KLinesList>("KLinesList");

    _headers.insert(QByteArray{"X-MEXC-APIKEY"}, QByteArray{""});
    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});
    _headers.insert(QByteArray{"User-Agent"}, QCoreApplication::applicationName().toUtf8());
}

MexcKLine::~MexcKLine()
{
    delete _timer;
}

void MexcKLine::start()
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

void MexcKLine::stop()
{
    if (_timer != nullptr)
    {
        _timer->stop();
    }
}

KLinesList MexcKLine::parseKLines(const QByteArray &data)
{
    KLinesList result;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        emit errorOccurred(_id, QString("MEXC: JSON parse error: %1")
                                    .arg(error.errorString()));

        return result;
    }

    const auto object = doc.array();

    for (const auto& kline: object)
    {
        const auto data = kline.toArray();

        const auto openDateTime = QDateTime::fromMSecsSinceEpoch(data[0].toInteger());
        const auto closeDateTime = QDateTime::fromMSecsSinceEpoch(data[6].toInteger());

        if (closeDateTime > _lastKLineCloseTime)
        {
            KLine tmp;
            tmp.openTime = QDateTime::fromMSecsSinceEpoch(data[0].toInteger());
            tmp.open = data[1].toString().toDouble();
            tmp.high = data[2].toString().toDouble();
            tmp.low = data[3].toString().toDouble();
            tmp.close = data[4].toString().toDouble();
            tmp.volume = data[5].toString().toDouble();
            tmp.closeTime = QDateTime::fromMSecsSinceEpoch(data[6].toInteger());
            tmp.quoteAssetVolume = data[7].toString().toDouble();
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

QUrl MexcKLine::getUrl(quint16 count)
{
    Q_ASSERT(count > 0 && count <= 2000);

    return QUrl{QString("https://api.mexc.com/api/v3/klines?symbol=%1&interval=%2&limit=%3")
                    .arg(_id.symbol)
                    .arg(KLineTypeToString(_id.type))
                    .arg(count)};
}

void MexcKLine::getAnswerHTTP(const QByteArray &answer, quint64 id)
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

void MexcKLine::errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString &msg, quint64 id)
{
    Q_CHECK_PTR(_timer);

    if (id != _currentRequestID)
    {
        return;
    }

    if (serverCode == 400)
    {
        emit delisting(_id);

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

void MexcKLine::run()
{
    if (_currentRequestID != 0) //предыдущая передача еще не заончилась, поэтому - пропускаем такт
    {
        return;
    }

    quint32 count = (_lastKLineCloseTime.msecsTo(QDateTime::currentDateTime()) / static_cast<quint64>(_id.type)) + 2;
    if (count > 2000)
    {
        count = 2000;
    }

    _currentRequestID = _httpSSLQuery->send(getUrl(count), HTTPSSLQuery::RequestType::GET, "", _headers);
}


