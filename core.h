#ifndef CORE_H
#define CORE_H

//QT
#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QUrl>
#include <QTimer>
#include <QRandomGenerator>

//My
#include "httpsslquery.h"
#include "mexc.h"

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
    void start();

private slots:
    void getAnswerHTTP(const QByteArray& answer, quint64 id);
    void errorOccurredHTTP(QNetworkReply::NetworkError code, const QString& msg, quint64 id);

    void detectKLine(const QString& symbol, const TraidingCatBot::Mexc::KLine& kline);

signals:
    void finished();
    void sendHTTP(const QUrl& url, TraidingCatBot::HTTPSSLQuery::RequestType type,
                  const QByteArray& data, const TraidingCatBot::HTTPSSLQuery::Headers& headers, quint64 id);

private:
    void sendRequest();
    void makeMoney(const QStringList& symbolsList);

private:
    QThread *_HTTPSSLQueryThread = nullptr;
    quint64 _currentRequestID = 0;
    QRandomGenerator *_rg = nullptr;

    QHash<QString, Mexc*> _money;

}; //class Core

} //namespace TraidingCatBot

#endif
