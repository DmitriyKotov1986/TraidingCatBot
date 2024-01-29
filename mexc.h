#ifndef MEXC_H
#define MEXC_H

//Qt
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QVector>

//My
#include "httpsslquery.h"

namespace TraidingCatBot
{

inline static const QUrl DEFAULT_SYMBOL_URL{"https://api.mexc.com/api/v3/defaultSymbols"};
QStringList defaultSymbol(const QByteArray& data);

inline static const QUrl PING_URL{"https://api.mexc.com/api/v3/ping"};

inline static const QUrl SERVER_TIME_URL{"https://api.mexc.com/api/v3/time"};
QDateTime serverTime(const QByteArray& data);

class Mexc final
    : public QObject
{
    Q_OBJECT

public:
    struct KLine
    {
        QDateTime openTime;
        double open = 0.0;
        double high = 0.0;
        double low = 0.0;
        double close = 0.0;
        double volume = 0.0;
        QDateTime closeTime;
        double quoteAssetVolume = 0.0;
    };

    using KLines = QVector<KLine>;

public:
    static double deltaKLine(const KLine& kline);
    static double volumeKLine(const KLine& kline);

private:
    static KLines parseKLines(const QByteArray& data);

public:
    Mexc() = delete;

    explicit Mexc(const QString& symbol, QObject *parent = nullptr);
    ~Mexc();

public:
    void start();

private slots:
    void getAnswerHTTP(const QByteArray& answer, quint64 id);
    void errorOccurredHTTP(QNetworkReply::NetworkError code, const QString& msg, quint64 id);

    void run();

signals:
    void sendHTTP(const QUrl& url, TraidingCatBot::HTTPSSLQuery::RequestType type,
                  const QByteArray& data, const TraidingCatBot::HTTPSSLQuery::Headers& headers, quint64 id);
    void detectKLine(const QString& symbol, const TraidingCatBot::Mexc::KLine& KLine);

private:
    const QString _symbol;
    quint64 _currentRequestID = 0;

    QUrl _klineUrl;
    HTTPSSLQuery::Headers _headers;

    QTimer *_timer = nullptr;
};

} //namespace TraidingCatBot

Q_DECLARE_METATYPE(TraidingCatBot::Mexc::KLine)
Q_DECLARE_METATYPE(TraidingCatBot::Mexc::KLines)

#endif
