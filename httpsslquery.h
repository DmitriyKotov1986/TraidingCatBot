#ifndef HTTPSSLQUERY_H
#define HTTPSSLQUERY_H

//QT
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QString>
#include <QTimer>
#include <QHash>
#include <QUrl>
#include <QByteArray>

namespace TraidingCatBot
{

class HTTPSSLQuery final
    : public QObject
{
    Q_OBJECT

public:
    using Headers = QHash<QByteArray, QByteArray>;

    enum class RequestType: quint8
    {
        POST,
        GET
    };

public:
    static HTTPSSLQuery* create(QObject* parent = nullptr);
    static void deleteHTTPSSLQuery();
    static quint64 getId();

public:
    void setUserPassword(const QString& user, const QString& password);

private:
    explicit HTTPSSLQuery(QObject* parent = nullptr);
    ~HTTPSSLQuery();

public slots:
    void send(const QUrl& url, RequestType type, const QByteArray& data, const Headers& headers, quint64 id); //запускает отправку запроса

signals:
    void getAnswer(const QByteArray& answer, quint64 id);
    void errorOccurred(QNetworkReply::NetworkError code, const QString& msg, quint64 id);

private slots:
    void replyFinished(QNetworkReply* resp); //конец приема ответа
    void watchDocTimeout();
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    void makeManager();

private:
    QNetworkAccessManager* _manager = nullptr; //менеджер обработки соединений

    struct AnswerData
    {
        QTimer* timer = nullptr;
        quint64 id = 0;
    };

    QHash<QNetworkReply*, AnswerData> _answerData;

    QString _user;
    QString _password;

};

}

Q_DECLARE_METATYPE(TraidingCatBot::HTTPSSLQuery::Headers)
Q_DECLARE_METATYPE(TraidingCatBot::HTTPSSLQuery::RequestType)


#endif // HTTPSSLQUERY_H
