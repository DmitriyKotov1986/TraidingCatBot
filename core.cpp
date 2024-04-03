//QT
#include <QtNetwork/QNetworkProxy>

//My
#include "core.h"
#include "appserver.h"

using namespace TradingCat;
using namespace Common;

bool needExit = false;

Core::Core(QObject *parent)
    : QObject{parent}
    , _cnf(Config::config())
    , _log(TDBLoger::DBLoger(_cnf->db_ConnectionInfo(), "LOG", true))
{
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(_log);
}

Core::~Core()
{
}

void Core::start()
{
    Q_CHECK_PTR(_log);

    _log->start();
    if (_log->isError())
    {
        const auto msg = QString ("Error start loger: %1").arg(_log->errorString());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(msg);

        exit(Common::EXIT_CODE::START_LOGGER_ERR);
    }
    _log->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Start successful");

    //DB saver
    _DBSaver.DBSaver = new TradingCat::DBSaver(_cnf->db_ConnectionInfo());

    _DBSaver.thread = new QThread();
    _DBSaver.DBSaver->moveToThread(_DBSaver.thread);

    QObject::connect(_DBSaver.thread, SIGNAL(started()),
                     _DBSaver.DBSaver, SLOT(start()), Qt::DirectConnection);

    QObject::connect(this, SIGNAL(stopAll()),
                     _DBSaver.DBSaver, SLOT(stop()), Qt::QueuedConnection);
    QObject::connect(_DBSaver.DBSaver, SIGNAL(finished()),
                     _DBSaver.thread, SLOT(quit()), Qt::DirectConnection);

    QObject::connect(_DBSaver.DBSaver, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                     _log, SLOT(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)), Qt::QueuedConnection);

    _DBSaver.thread->start();

    //AppServer
    _appServer.appServer = new AppServer(_cnf->server_config(), _cnf->db_ConnectionInfo());
    _appServer.thread = new QThread();
    _appServer.appServer->moveToThread(_appServer.thread);

    QObject::connect(_appServer.thread, SIGNAL(started()),
                     _appServer.appServer, SLOT(start()), Qt::DirectConnection);
    QObject::connect(_appServer.appServer, SIGNAL(started()),
                     SLOT(loadStockExchenge()), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(stopAll()),
                     _appServer.appServer, SLOT(stop()), Qt::QueuedConnection);
    QObject::connect(_appServer.appServer, SIGNAL(finished()),
                     _appServer.thread, SLOT(quit()), Qt::DirectConnection);

    QObject::connect(_appServer.appServer, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                     _log, SLOT(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)), Qt::QueuedConnection);

    _appServer.thread->start();

    //exitTimer
    _exitTimer = new QTimer();

    QObject::connect(_exitTimer, SIGNAL(timeout()), SLOT(stop()));

    _exitTimer->start(1000);
}

void Core::stop()
{
    if (!needExit)
    {
        return;
    }

    emit stopAll();

    if (_appServer.thread != nullptr)
    {
        _appServer.thread->quit();
        _appServer.thread->wait();

        delete _appServer.appServer;
        delete _appServer.thread;
    }

    if (_DBSaver.thread != nullptr)
    {
        _DBSaver.thread->quit();
        _DBSaver.thread->wait();

        delete _DBSaver.DBSaver;
        delete _DBSaver.thread;
    }

    _mexcStockExchange.thread->quit();
    _mexcStockExchange.thread->wait();

    delete _mexcStockExchange.mexcStockExchange;
    delete _mexcStockExchange.thread;

    _kucoinStockExchange.thread->quit();
    _kucoinStockExchange.thread->wait();

    delete _kucoinStockExchange.kucoinStockExchange;
    delete _kucoinStockExchange.thread;

    _gateStockExchange.thread->quit();
    _gateStockExchange.thread->wait();

    delete _gateStockExchange.gateStockExchange;
    delete _gateStockExchange.thread;

    _log->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Stop successful");

    TDBLoger::deleteDBLoger();

    emit finished();
}

void Core::loadStockExchenge()
{
    Q_CHECK_PTR(_log);

    Common::HTTPSSLQuery::ProxyList proxyList;
    for (const auto& proxyInfo: _cnf->proxy_info_list())
    {
        QNetworkProxy tmp;
        tmp.setHostName(proxyInfo.proxy_host);
        tmp.setPort(proxyInfo.proxy_port);
        tmp.setUser(proxyInfo.proxy_user);
        tmp.setPassword(proxyInfo.proxy_password);
        tmp.setType(QNetworkProxy::HttpProxy);

        proxyList.emplaceBack(std::move(tmp));
    }

    if (_cnf->stockExcange_MEXC_enable())
    {
        _mexcStockExchange.mexcStockExchange = new MexcStockExchange(_cnf->db_ConnectionInfo(), proxyList);
        _mexcStockExchange.thread = new QThread();
        _mexcStockExchange.mexcStockExchange->moveToThread(_mexcStockExchange.thread);

        QObject::connect(_mexcStockExchange.thread, SIGNAL(started()),
                         _mexcStockExchange.mexcStockExchange, SLOT(start()), Qt::DirectConnection);

        QObject::connect(this, SIGNAL(stopAll()),
                         _mexcStockExchange.mexcStockExchange, SLOT(stop()), Qt::QueuedConnection);
        QObject::connect(_mexcStockExchange.mexcStockExchange, SIGNAL(finished()),
                         _mexcStockExchange.thread, SLOT(quit()), Qt::DirectConnection);

        QObject::connect(_mexcStockExchange.mexcStockExchange, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                         _log, SLOT(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)), Qt::QueuedConnection);

        QObject::connect(_mexcStockExchange.mexcStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _DBSaver.DBSaver, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        QObject::connect(_mexcStockExchange.mexcStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _appServer.appServer, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        _mexcStockExchange.thread->start();

        _log->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Add stock exchange: MEXC"));
    }

    if (_cnf->stockExcange_KUCOIN_enable())
    {
        _kucoinStockExchange.kucoinStockExchange = new KucoinStockExchange(_cnf->db_ConnectionInfo(), proxyList);
        _kucoinStockExchange.thread = new QThread();
        _kucoinStockExchange.kucoinStockExchange->moveToThread(_kucoinStockExchange.thread);

        QObject::connect(_kucoinStockExchange.thread, SIGNAL(started()),
                         _kucoinStockExchange.kucoinStockExchange, SLOT(start()), Qt::DirectConnection);

        QObject::connect(this, SIGNAL(stopAll()),
                         _kucoinStockExchange.kucoinStockExchange, SLOT(stop()), Qt::QueuedConnection);
        QObject::connect(_kucoinStockExchange.kucoinStockExchange, SIGNAL(finished()),
                         _kucoinStockExchange.thread, SLOT(quit()), Qt::DirectConnection);

        QObject::connect(_kucoinStockExchange.kucoinStockExchange, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                         _log, SLOT(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)), Qt::QueuedConnection);

        QObject::connect(_kucoinStockExchange.kucoinStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _DBSaver.DBSaver, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        QObject::connect(_kucoinStockExchange.kucoinStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _appServer.appServer, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        _kucoinStockExchange.thread->start();

        _log->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Add stock exchange: KUCOIN"));
    }

    if (_cnf->stockExcange_GATE_enable())
    {
        _gateStockExchange.gateStockExchange = new GateStockExchange(_cnf->db_ConnectionInfo(), proxyList);
        _gateStockExchange.thread = new QThread();
        _gateStockExchange.gateStockExchange->moveToThread(_gateStockExchange.thread);

        QObject::connect(_gateStockExchange.thread, SIGNAL(started()),
                         _gateStockExchange.gateStockExchange, SLOT(start()), Qt::DirectConnection);

        QObject::connect(this, SIGNAL(stopAll()),
                         _gateStockExchange.gateStockExchange, SLOT(stop()), Qt::QueuedConnection);
        QObject::connect(_gateStockExchange.gateStockExchange, SIGNAL(finished()),
                         _gateStockExchange.thread, SLOT(quit()), Qt::DirectConnection);

        QObject::connect(_gateStockExchange.gateStockExchange, SIGNAL(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)),
                         _log, SLOT(sendLogMsg(Common::TDBLoger::MSG_CODE, const QString&)), Qt::QueuedConnection);

        QObject::connect(_gateStockExchange.gateStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _DBSaver.DBSaver, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        QObject::connect(_gateStockExchange.gateStockExchange, SIGNAL(getKLines(const TradingCat::StockExchangeKLinesList&)),
                         _appServer.appServer, SLOT(getKLines(const TradingCat::StockExchangeKLinesList&)), Qt::QueuedConnection);

        _gateStockExchange.thread->start();

        _log->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Add stock exchange: GATE"));
    }
}




