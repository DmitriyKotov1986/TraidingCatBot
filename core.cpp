//QT

//My
#include "httpsslquery.h"

#include "core.h"

using namespace TraidingCatBot;

Core::Core(QObject *parent)
    : QObject{parent}
    , _rg(QRandomGenerator::global())
{

}

Core::~Core()
{
    if (_HTTPSSLQueryThread  != nullptr)
    {
        _HTTPSSLQueryThread ->quit();
        _HTTPSSLQueryThread ->wait();

        delete _HTTPSSLQueryThread;
    }

    for (const auto& mexc_ptr: _money.values())
    {
        delete mexc_ptr;
    }

    for (auto& money: _money)
    {
        delete money;
    }
}

void Core::start()
{
    //создаем поток обработки HTTP Запросов
    auto HTTPSSLQuery = HTTPSSLQuery::create();

    _HTTPSSLQueryThread = new QThread();
    HTTPSSLQuery->moveToThread(_HTTPSSLQueryThread);

    QObject::connect(this, SIGNAL(sendHTTP(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)), HTTPSSLQuery,
                           SLOT(send(const QUrl&, TraidingCatBot::HTTPSSLQuery::RequestType, const QByteArray&, const TraidingCatBot::HTTPSSLQuery::Headers&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(getAnswer(const QByteArray&, quint64)), SLOT(getAnswerHTTP(const QByteArray&, quint64)), Qt::QueuedConnection);
    QObject::connect(HTTPSSLQuery, SIGNAL(errorOccurred(QNetworkReply::NetworkError, const QString&, quint64)),
                           SLOT(errorOccurredHTTP(QNetworkReply::NetworkError, const QString&, quint64)), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(finished()), _HTTPSSLQueryThread, SLOT(quit()), Qt::QueuedConnection); //сигнал на завершение
    QObject::connect(_HTTPSSLQueryThread, SIGNAL(finished()), HTTPSSLQuery, SLOT(deleteLater()), Qt::DirectConnection); //уничтожиться после остановки

    _HTTPSSLQueryThread->start();

    sendRequest();
}


void Core::getAnswerHTTP(const QByteArray &answer, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

   makeMoney(defaultSymbol(answer));
   // makeMoney(QStringList() << "BTCUSDT");
}

void Core::errorOccurredHTTP(QNetworkReply::NetworkError code, const QString &msg, quint64 id)
{
    if (id != _currentRequestID)
    {
        return;
    }

    qDebug() << msg;

    emit finished();
}

void Core::detectKLine(const QString &symbol, const Mexc::KLine &kline)
{
    qDebug() << QTime::currentTime().toString("HH:mm:ss")
             << "(!)Detect:" << symbol
             << "delta:" << Mexc::deltaKLine(kline)
             << "Volume:" << Mexc::volumeKLine(kline);
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
