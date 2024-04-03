//STL
#include <algorithm>

//My
#include "Common/httpsslquery.h"

#include "money.h"

using namespace TraidingCatBot;
using namespace Common;

MoneyKLine::MoneyKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, QObject *parent)
    : QObject{parent}
    , _id(id)
    , _lastKLineCloseTime(lastKLineCloseTime)
{
    Q_ASSERT(!_id.symbol.isEmpty());
    Q_ASSERT(_id.type != KLineType::UNKNOW);

    qRegisterMetaType<TraidingCatBot::KLine>("KLine");
}

MoneyKLine::~MoneyKLine()
{
    delete _timer;
}

void MoneyKLine::start()
{
    if (_id.type == KLineType::UNKNOW)
    {
        return;
    }

    auto HTTPSSLQueryPool = HTTPSSLQueryPool::create();

    QObject::connect(this, SIGNAL(sendHTTP(const QUrl&, Common::HTTPSSLQuery::RequestType, const QByteArray&, const Common::HTTPSSLQuery::Headers&, quint64)),
                     HTTPSSLQueryPool, SLOT(send(const QUrl&, Common::HTTPSSLQuery::RequestType, const QByteArray&, const Common::HTTPSSLQuery::Headers&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQueryPool, SIGNAL(getAnswer(const QByteArray&, quint64)),
                     SLOT(getAnswerHTTP(const QByteArray&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQueryPool, SIGNAL(errorOccurred(QNetworkReply::NetworkError, const QString&, quint64)),
                     SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, const QString&, quint64)), Qt::QueuedConnection);

    startKLine();

    _timer = new QTimer();

    QObject::connect(_timer, SIGNAL(timeout()), SLOT(run()));

    auto interval = static_cast<quint64>(_id.type);
    _timer->start(_id.type == KLineType::MIN1 ? (interval * 5) : interval);

    run();
}

void MoneyKLine::stop()
{
    stopKLine();
}

const KLineID& MoneyKLine::getID() const
{
    return _id;
}

void MoneyKLine::getAnswerHTTP(const QByteArray &answer, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    auto klines = parseKLines(answer);
    if (klines.isEmpty())
    {
        return;
    }

    std::sort(klines.begin(), klines.end(),
        [](const auto& kline1, const auto& kline2)
        {
            return kline1.closeTime < kline1.closeTime;
        });

    for (const auto& kline: klines)
    {
        if (kline.closeTime > _lastKLineCloseTime)
        {
            emit getKLine(kline);

            _lastKLineCloseTime = kline.closeTime;
        }
    }
}

void MoneyKLine::errorOccurredHTTP(QNetworkReply::NetworkError code, const QString &msg, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    emit errorOccurred(_id, msg);
}

void MoneyKLine::run()
{
    _currentRequestID = HTTPSSLQueryPool::getId();

    quint32 count = (_lastKLineCloseTime.msecsTo(QDateTime::currentDateTime()) / static_cast<quint64>(_id.type)) + 1;
    if (count > 2000)
    {
        count = 2000;
    }
    if (count == 0)
    {
        return;
    }

    emit sendHTTP(getUrl(count), HTTPSSLQuery::RequestType::GET, "", getHeaders(), _currentRequestID);
}


