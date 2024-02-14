#ifndef CORE_H
#define CORE_H

//QT
#include <QObject>
#include <QHash>
#include <QThread>

//My
#include "config.h"
#include "stockexchange.h"

namespace TraidingCatBot
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

signals:
    void stopAll();    //остановка

private:
    void loadStockExchenge();

private:
    struct StockExchangeInfo
    {
        StockExchange *stockEnchange = nullptr;
        QThread* thread = nullptr;
    };

private:
    Config *_cnf = nullptr;                             //Конфигурация

    QThread *_HTTPSSLQueryThread = nullptr;            //поток обработки HTTP запросов

    QHash<QString, StockExchangeInfo> _stockExchanges; //загруженные биржи

}; //class Core

} //namespace TraidingCatBot

#endif
