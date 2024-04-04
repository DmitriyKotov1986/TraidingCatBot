#ifndef CORE_H
#define CORE_H

//QT
#include <QObject>
#include <QHash>
#include <QThread>
#include <QTimer>

//My
#include "config.h"
#include "Common/tdbloger.h"
#include "dbsaver.h"
#include "appserver.h"
#include "mexcstockexchange.h"
#include "kucoinstockexchange.h"
#include "gatestockexchange.h"
#include "bybitstockexchange.h"

namespace TradingCat
{

class Core final
    : public QObject
{
    Q_OBJECT

public:
    explicit Core(QObject *parent = nullptr);
    ~Core();

public slots:
    void start();      //запуск работы
    void stop();

signals:
    void stopAll();    //остановка
    void finished();

private slots:
    void loadStockExchenge();

private:
    struct MexcStockExchangeInfo
    {
        MexcStockExchange *mexcStockExchange = nullptr;
        QThread* thread = nullptr;
    };

    struct KucoinStockExchangeInfo
    {
        KucoinStockExchange *kucoinStockExchange = nullptr;
        QThread* thread = nullptr;
    };

    struct GateStockExchangeInfo
    {
        GateStockExchange *gateStockExchange = nullptr;
        QThread* thread = nullptr;
    };

    struct BybitStockExchangeInfo
    {
        BybitStockExchange *bybitStockExchange = nullptr;
        QThread* thread = nullptr;
    };

    struct DBSaverInfo
    {
        TradingCat::DBSaver *DBSaver = nullptr;
        QThread *thread = nullptr;
    };

    struct AppServerInfo
    {
         AppServer *appServer = nullptr;
         QThread *thread = nullptr;
    };

private:
    Config *_cnf = nullptr;                            //Конфигурация
    Common::TDBLoger *_log = nullptr;

    DBSaverInfo _DBSaver;                              //поток сохранения данных в БД

    AppServerInfo _appServer;

    MexcStockExchangeInfo _mexcStockExchange;
    KucoinStockExchangeInfo _kucoinStockExchange;
    GateStockExchangeInfo _gateStockExchange;
    BybitStockExchangeInfo _bybitStockExchange;

    QTimer *_exitTimer = nullptr;

}; //class Core

} //namespace TraidingCatBot

#endif
