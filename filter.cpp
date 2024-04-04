#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>

#include "filter.h"

using namespace TradingCat;

Filter::Filter()
{
    defaultFilter();
}

Filter::Filter(const Filter &other)
{
    setFilter(other.getFilter());
}

Filter &Filter::operator=(const Filter &other)
{
    setFilter(other.getFilter());

    return *this;
}

Filter::Filter(Filter &&other)
{
    _filterData = std::move(other._filterData);
}

Filter &Filter::operator=(Filter &&other)
{
    _filterData = std::move(other._filterData);

    return *this;
}

Filter::Filter(const QJsonArray &JSONFilter)
{
    setFilter(JSONFilter);
}

Filter::~Filter()
{
    clear();
}

void Filter::setFilter(const QJsonArray &JSONFilter)
{
    clear();

    for (int i = 0; i < JSONFilter.count(); ++i)
    {
        const auto kline = JSONFilter.at(i).toObject();

        auto filterData = new FilterData();
        filterData->stockExchangeID.name = kline["StockExchange"].toString();
        filterData->klineID.symbol = kline["Money"].toString();
        filterData->klineID.type = stringToKLineType(kline["Interval"].toString());
        filterData->delta = kline["Delta"].toDouble();
        filterData->volume = kline["Volume"].toDouble();

        _filterData.push_back(filterData);
    }
}

QJsonArray Filter::getFilter() const
{
    QJsonArray json;
    for (auto filterData: _filterData)
    {
        QJsonObject kline;

        kline.insert("StockExchange", filterData->stockExchangeID.name);
        kline.insert("Money", filterData->klineID.symbol);
        kline.insert("Interval", KLineTypeToString(filterData->klineID.type));
        kline.insert("Delta", filterData->delta);
        kline.insert("Volume", filterData->volume);

        json.push_back(kline);
    }

    return json;
}

bool Filter::filter(const StockExchangeID &stockExchangeID, const KLineID &klineID, double delta, double volume)
{
    for (const auto& filterData: _filterData)
    {
//        qDebug() << filterData->stockExchangeID.name << stockExchangeID.name <<
//                    filterData->klineID.symbol << klineID.symbol <<
//                    KLineTypeToString(filterData->klineID.type) << KLineTypeToString(klineID.type) <<
//                    filterData->delta <<  delta <<
//                    filterData->volume << volume;
        if (filterData->klineID.symbol == "ALL")
        {
            if (filterData->stockExchangeID == stockExchangeID &&
                filterData->klineID.type == klineID.type &&
                filterData->delta <= delta &&
                filterData->volume <= volume)
            {
                return true;
            }
        }
        else
        {
            if (filterData->stockExchangeID == stockExchangeID &&
                filterData->klineID == klineID &&
                filterData->delta <= delta &&
                filterData->volume <= volume)
            {
                return true;
            }
        }
    }

    return false;
}

void Filter::defaultFilter()
{
    {
        KLineID klineID;
        klineID.symbol = "ALL";
        klineID.type = KLineType::MIN1;

        StockExchangeID stockExchangeID;
        stockExchangeID.name = "MEXC";

        addFilter(stockExchangeID, klineID, 5.0, 1000.0);
    }
    {
        KLineID klineID;
        klineID.symbol = "ALL";
        klineID.type = KLineType::MIN1;

        StockExchangeID stockExchangeID;
        stockExchangeID.name = "KUCOIN";

        addFilter(stockExchangeID, klineID, 2.0, 800.0);
    }

    {
        KLineID klineID;
        klineID.symbol = "ALL";
        klineID.type = KLineType::MIN1;

        StockExchangeID stockExchangeID;
        stockExchangeID.name = "GATE";

        addFilter(stockExchangeID, klineID, 8.0, 2000.0);
    }

    {
        KLineID klineID;
        klineID.symbol = "ALL";
        klineID.type = KLineType::MIN1;

        StockExchangeID stockExchangeID;
        stockExchangeID.name = "BYBIT";

        addFilter(stockExchangeID, klineID, 5.0, 1000.0);
    }
}

void Filter::clear()
{
    for (auto& filterData: _filterData)
    {
        delete filterData;
    }
    _filterData.clear();
}

void Filter::addFilter(const StockExchangeID &stockExchangeID, const KLineID &klineID, double delta, double volume)
{
    auto filterData = new FilterData;
    filterData->stockExchangeID = stockExchangeID;
    filterData->klineID = klineID;
    filterData->delta = delta;
    filterData->volume = volume;

    _filterData.push_back(filterData);
}

QString Filter::errorString()
{
    const QString tmp = _errorString;
    _errorString.clear();

    return tmp;
}

bool Filter::isError() const
{
    return !_errorString.isEmpty();
}
