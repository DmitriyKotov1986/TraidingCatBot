#ifndef MONEY_H
#define MONEY_H

#include <QObject>

//My
#include "types.h"
#include "Common/httpsslquery.h"

namespace TraidingCatBot
{

class MoneyKLine
    : public QObject
{
    Q_OBJECT

public:
    MoneyKLine() = delete;

    explicit MoneyKLine(const StockExchangeInfo& stockExchangeInfo, const KLineID& id, QObject *parent = nullptr); //гарантируется что symbol не пустая строка
    virtual ~MoneyKLine();

public:
    void start();
    void stop();

    const KLineID& getID() const;

signals:
    //public
    void getKLine(const TraidingCatBot::KLine& KLine); //испускается когда получена новая свечка
    void errorOccurred(const KLineID& id, const QString& msg);

    //private
    void sendHTTP(const QUrl& url, Common::HTTPSSLQuery::RequestType type,
                  const QByteArray& data, const Common::HTTPSSLQuery::Headers& headers, quint64 id);

protected:
    virtual void startKLine() = 0;
    virtual void stopKLine() = 0;
    virtual KLines parseKLines(const QByteArray& data) = 0;
    virtual QUrl getUrl(quint16 count) = 0;
    virtual Common::HTTPSSLQuery::Headers getHeaders() = 0;

private slots:
    void getAnswerHTTP(const QByteArray& answer, quint64 id);
    void errorOccurredHTTP(QNetworkReply::NetworkError code, const QString& msg, quint64 id);

    void run();

private:
    QDateTime stockExchangeTime() const;

private:
    const KLineID _id;
    const StockExchangeInfo _stockExchangeInfo;

    quint64 _currentRequestID = 0;

    QTimer *_timer = nullptr;
};

} //namespace TraidingCatBot

Q_DECLARE_METATYPE(TraidingCatBot::KLine)

#endif // MONEY_H
