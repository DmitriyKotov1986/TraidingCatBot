//QT

//My
#include "Common/httpsslquery.h"
#include "core.h"
#include "types.h"
#include "mexcstockexchange.h"

using namespace TraidingCatBot;
using namespace Common;

Core::Core(QObject *parent)
    : QObject{parent}
{
}

Core::~Core()
{
    for (const auto& stockExchangeInfo: _stockExchanges)
    {
        stockExchangeInfo.thread->quit();
        stockExchangeInfo.thread->wait();

        delete stockExchangeInfo.stockEnchange;
        delete stockExchangeInfo.thread;
    }

    if (_HTTPSSLQueryThread != nullptr)
    {
        _HTTPSSLQueryThread->quit();
        _HTTPSSLQueryThread->wait();

        delete _HTTPSSLQueryThread;
    }   
}

void Core::start()
{
    //создаем поток обработки HTTP Запросов
    auto HTTPSSLQuery = HTTPSSLQuery::create();

    _HTTPSSLQueryThread = new QThread();
    HTTPSSLQuery->moveToThread(_HTTPSSLQueryThread);

    _HTTPSSLQueryThread->start();

    loadStockExchenge();
}

void Core::loadStockExchenge()
{
    if (_cnf->stockExcange_MEXC_enable())
    {
        StockExchangeID id;
        id.name = "MEXC";

        StockExchangeInfo stockExchangeInfo;
        stockExchangeInfo.stockEnchange = new MexcStockExchange(id);
        stockExchangeInfo.thread = new QThread();
        stockExchangeInfo.stockEnchange->moveToThread(stockExchangeInfo.thread);

        QObject::connect(stockExchangeInfo.thread, SIGNAL(started()),
                         stockExchangeInfo.stockEnchange, SLOT(start()), Qt::DirectConnection);

        QObject::connect(this, SIGNAL(stopAll()),
                         stockExchangeInfo.stockEnchange, SLOT(stop()), Qt::QueuedConnection);
        QObject::connect(stockExchangeInfo.stockEnchange, SIGNAL(finished()),
                         stockExchangeInfo.thread, SLOT(quint()), Qt::DirectConnection);

        _stockExchanges.insert(id.name, stockExchangeInfo);
    }
}




