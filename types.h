#ifndef TYPES_H
#define TYPES_H

//QT
#include <QDateTime>
#include <QVector>
#include <QString>

namespace TraidingCatBot
{

enum class KLineType: qint64 //Тип свечи
{
    MIN1   = 1 * 60,     //1мин свеча
    MIN5   = 5 * 60,     //5мин свеча
    MIN15  = 15 * 60,    //15мин свеча
    MIN30  = 30 * 60,    //30мин свеча
    MIN60  = 60 * 60,    //60мин свеча
    HOUR4  = 240 * 60,
    HOUR8  = 480 * 60,
    DAY1   = 1440 * 60,
    WEEK1  = 10080 * 60,
    MONTH1 = 43200 * 60,
    UNKNOW = 0      //неизвестный тип свечи
};

QString KLineTypeToString(KLineType type);
KLineType stringToKLineType(const QString& type);

using KLineTypes = QVector<KLineType>;

struct KLineID
{
    QString symbol;        //название монеты
    QString stockExcahnge; //Название биржи
    KLineType type = KLineType::UNKNOW; //интервал свечи
};

struct KLine //данные свечи
{
    KLineID id;
    QDateTime openTime;  //время открытия
    double open = 0.0;   //цена в момент открытия
    double high = 0.0;   //наибольшая свеча
    double low = 0.0;    //наименьшая свеча
    double close = 0.0;  //цена в момент закрытия
    double volume = 0.0; //объем
    QDateTime closeTime; //время закрытия
    double quoteAssetVolume = 0.0;
};

double deltaKLine(const KLine &kline);
double volumeKLine(const KLine &kline);

using KLines = QVector<KLine>;

struct StockExchangeInfo //глобальна информация про биржу
{
    qint64 timeShift = 0; //смещение времени биржи относительно времени сервера в секундах
    qint64 ping = 0;      //пинг до сервера биржи
};

struct StockExchangeID
{
    QString name;
};

} //namespace TraidingCatBot

#endif // TYPES_H
