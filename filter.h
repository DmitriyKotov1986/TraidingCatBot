#ifndef FILTER_H
#define FILTER_H

#include <QString>

#include "types.h"

namespace TradingCat
{

class Filter
{

public:
    Filter();
    Filter(const Filter& other);
    Filter& operator=(const Filter& other);
    Filter(Filter&& other);
    Filter& operator=(Filter &&other);

    explicit Filter(const QJsonArray& JSONFilter);

    ~Filter();

    void setFilter(const QJsonArray& JSONFilter);
    QJsonArray getFilter() const;

    bool filter(const StockExchangeID& stockExchangeID, const KLineID& klineID, double delta, double volume);

    QString errorString();
    bool isError() const;

private:
    void defaultFilter();
    void clear();
    void addFilter(const StockExchangeID& stockExchangeID, const KLineID& klineID, double delta, double volume);

private:
    struct FilterData
    {
        StockExchangeID stockExchangeID;
        KLineID klineID;
        double delta = 0;
        double volume = 0;
    };

    using FilterDataList = QList<FilterData*>;

private:
    FilterDataList _filterData;

    QString _errorString;
};

} //namespace TraidingCatBot

#endif // FILTER_H
