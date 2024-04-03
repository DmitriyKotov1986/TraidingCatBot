#ifndef KUCOINKLINE_H
#define KUCOINKLINE_H

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

class KucoinKLine final
    : public QObject
{
    Q_OBJECT

public:
    KucoinKLine() = delete;
    KucoinKLine(const KucoinKLine&) = delete;
    KucoinKLine& operator=(const KucoinKLine&) = delete;
    KucoinKLine(KucoinKLine&&) = delete;
    KucoinKLine& operator=(KucoinKLine&&) = delete;

    explicit KucoinKLine(const KLineID& id, const QDateTime& lastKLineCloseTime, Common::HTTPSSLQuery *httpSSLQuery, QObject *parent = nullptr);
    ~KucoinKLine();

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
    QUrl getUrl(const QDateTime& start, const QDateTime& end);
    QString KLineTypeToString(KLineType type);

private:
    const KLineID _id;
    QUrl _url;

    Common::HTTPSSLQuery *_httpSSLQuery = nullptr;

    QDateTime _lastKLineCloseTime = QDateTime::currentDateTime();

    quint64 _currentRequestID = 0;

    QTimer *_timer = nullptr;

    qint64 _HTTPRequestAAfterError = -1;

private:
    Common::HTTPSSLQuery::Headers _headers;

};

} //namespace TradingCat

#endif // KUCOINKLINE_H
