#ifndef BINANCEKLINE_H
#define BINANCEKLINE_H

//Qt
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QVector>

//My
#include "types.h"
#include "Common/httpsslquery.h"

namespace TradingCat
{

class BinanceKLine final
    : public QObject
{
    Q_OBJECT

public:
    BinanceKLine() = delete;
    BinanceKLine(const BinanceKLine&) = delete;
    BinanceKLine& operator=(const BinanceKLine&) = delete;
    BinanceKLine(BinanceKLine&&) = delete;
    BinanceKLine& operator=(BinanceKLine&&) = delete;

    explicit BinanceKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, Common::HTTPSSLQuery *httpSSLQuery, QObject *parent = nullptr);
    ~BinanceKLine();

    void start();
    void stop();

signals:
    //public
    void getKLines(const TradingCat::KLinesList& klines); //испускается когда получена новая свечка
    void errorOccurred(const TradingCat::KLineID& id, const QString& msg);

    void delisting(const TradingCat::KLineID& id);

private slots:
    void getAnswerHTTP(const QByteArray& answer, quint64 id);
    void errorOccurredHTTP(QNetworkReply::NetworkError code, quint64 serverCode, const QString& msg, quint64 id);

    void run();

private:
    KLinesList parseKLines(const QByteArray& data);
    QUrl getUrl(quint32 count) const;
    static QString KLineTypeToString(KLineType type);

private:
    const KLineID _id;
    QUrl _url;

    Common::HTTPSSLQuery *_httpSSLQuery = nullptr;

    QDateTime _lastKLineCloseTime = QDateTime::currentDateTime();

    quint64 _currentRequestID = 0;

    QTimer *_timer = nullptr;

    qint64 _HTTPRequestAfterError = -1;

private:
    Common::HTTPSSLQuery::Headers _headers;

};

} // TradingCat

#endif // BINANCEKLINE_H
