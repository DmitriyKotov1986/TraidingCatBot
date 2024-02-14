#include "money.h"

using namespace TraidingCatBot;
using namespace Common;

MoneyKLine::MoneyKLine(const StockExchangeInfo& stockExchangeInfo, const KLineID& id, QObject *parent)
    : QObject{parent}
    , _id(id)
    , _stockExchangeInfo(stockExchangeInfo)
{
    Q_ASSERT(!id.stockExcahnge.isEmpty());
    Q_ASSERT(!id.symbol.isEmpty());
    Q_ASSERT(id.type != KLineType::UNKNOW);

    qRegisterMetaType<TraidingCatBot::KLine>("KLine");
}

MoneyKLine::~MoneyKLine()
{
    delete _timer;
}

void MoneyKLine::start()
{
    auto HTTPSSLQuery = HTTPSSLQuery::create();

    QObject::connect(this, SIGNAL(sendHTTP(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)),
                     HTTPSSLQuery, SLOT(send(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(getAnswer(const QByteArray&, quint64)),
                     SLOT(getAnswerHTTP(const QByteArray&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(errorOccurred(QNetworkReply::NetworkError, const QString&, quint64)),
                     SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, const QString&, quint64)), Qt::QueuedConnection);

    _timer = new QTimer();
    _timer->setSingleShot(true);

    QObject::connect(_timer, SIGNAL(timeout()), SLOT(run()));

    run();

    startKLine();
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

    const auto klines = parseKLines(answer);
    if (klines.isEmpty())
    {
        _timer->start(static_cast<quint64>(_id.type) / 2);

        return;
    }

    QDateTime lastKLine;
    for (const auto& kline: klines)
    {
        lastKLine = std::max(lastKLine, kline.closeTime);

        emit getKLine(kline);
    }

    _timer->start(stockExchangeTime().secsTo(lastKLine.addSecs(static_cast<quint64>(_id.type))) + 1);
}

void MoneyKLine::errorOccurredHTTP(QNetworkReply::NetworkError code, const QString &msg, quint64 id)
{
    if (id == _currentRequestID)
    {
        emit errorOccurred(_id, msg);
    }

    _timer->start(static_cast<quint64>(_id.type));
}

void MoneyKLine::run()
{
    _currentRequestID = HTTPSSLQuery::getId();

    emit sendHTTP(getUrl(1), HTTPSSLQuery::RequestType::GET, "", getHeaders(), _currentRequestID);
}

QDateTime MoneyKLine::stockExchangeTime() const
{
    return QDateTime::currentDateTime().addSecs(_stockExchangeInfo.timeShift);
}


