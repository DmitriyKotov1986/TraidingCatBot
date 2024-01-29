//QT
#include <QCoreApplication>
#include <QAuthenticator>
#include <QMutex>
#include <QMutexLocker>

#include "httpsslquery.h"

using namespace TraidingCatBot;

//static
static HTTPSSLQuery *HTTPSSLQuery_ptr = nullptr;

HTTPSSLQuery* HTTPSSLQuery::create(QObject* parent)
{
    if (HTTPSSLQuery_ptr == nullptr)
    {
        HTTPSSLQuery_ptr = new HTTPSSLQuery(parent);
    }

    return HTTPSSLQuery_ptr;
}

void HTTPSSLQuery::deleteHTTPSSLQuery()
{
    Q_CHECK_PTR(HTTPSSLQuery_ptr);

    delete HTTPSSLQuery_ptr;

    HTTPSSLQuery_ptr = nullptr;
}

quint64 HTTPSSLQuery::getId()
{
    static quint64 id = 0;

    static QMutex idMutex;

    QMutexLocker locker(&idMutex);
    ++id;

    return id;
}

void HTTPSSLQuery::setUserPassword(const QString &user, const QString &password)
{
    _user = user;
    _password = password;
}

//class
HTTPSSLQuery::HTTPSSLQuery(QObject* parent)
    : QObject{parent}
    , _manager()
{
    qRegisterMetaType<TraidingCatBot::HTTPSSLQuery::Headers>("Headers");
    qRegisterMetaType<TraidingCatBot::HTTPSSLQuery::RequestType>("RequestType");
    qRegisterMetaType<QNetworkReply::NetworkError>("NetworkError");
}

HTTPSSLQuery::~HTTPSSLQuery()
{
    if (_manager != nullptr)
    {
        watchDocTimeout(); // закрываем все соединения

        _manager->deleteLater();
    }
}

void HTTPSSLQuery::send(const QUrl& url, RequestType type, const QByteArray& data, const Headers& headers, quint64 id)
{
    makeManager();
    Q_CHECK_PTR(_manager);

    //создаем и отправляем запрос
    QNetworkRequest request(url);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    for (auto header_it = headers.begin(); header_it != headers.end(); ++header_it)
    {
        request.setRawHeader(header_it.key(), header_it.value());
    }
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));
    request.setTransferTimeout(30000);

    QNetworkReply* resp = nullptr;
    switch (type)
    {
    case RequestType::POST:
    {
        resp = _manager->post(request, data);

        break;
    }
    case RequestType::GET:
    {
        resp = _manager->get(request);

        break;
    }
    default:
        Q_ASSERT(false);
    }


    QTimer* watchDocTimer = new QTimer();
    watchDocTimer->setSingleShot(true);
    QObject::connect(watchDocTimer, SIGNAL(timeout()), SLOT(watchDocTimeout()));

    auto timer = new QTimer();
    timer->start(60000);
    _answerData.insert(resp, {timer, id});

    //qDebug() << "REQUEST: " << url << " data:" << data;
}

void HTTPSSLQuery::replyFinished(QNetworkReply *resp)
{
    Q_ASSERT(resp);
    Q_ASSERT(_answerData.contains(resp));

    auto answerData_it = _answerData.find(resp);

    if (resp->error() == QNetworkReply::NoError)
    {
        QByteArray answer = resp->readAll();

        //qDebug() << "ANSWER" << answer;

        emit getAnswer(answer, answerData_it->id);
    }
    else
    {
        const auto msg = QString("HTTP request fail. Code: %1 Msg: %2")
                             .arg(QString::number(resp->error()))
                             .arg(resp->errorString());

        emit errorOccurred(resp->error(), msg, answerData_it->id);
    }

    auto timer = answerData_it->timer;
    delete timer;

    _answerData.erase(answerData_it);

    resp->deleteLater();
}

void HTTPSSLQuery::watchDocTimeout()
{
    for (auto const resp: _answerData.keys())
    {
        resp->abort();
    }
}

void HTTPSSLQuery::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);
    Q_CHECK_PTR(reply);

    authenticator->setUser(_user);
    authenticator->setPassword(_password);
    authenticator->setOption("realm", authenticator->realm());
}

void HTTPSSLQuery::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_CHECK_PTR(reply);

    reply->ignoreSslErrors();

    QString errorsString;
    for (const auto& error: errors)
    {
        if (errorsString.isEmpty())
        {
            errorsString += "; ";
        }

        errorsString += error.errorString();
    }

    qDebug() << "SSL Error: " << errors.first().errorString();
}

void HTTPSSLQuery::makeManager()
{
    if (_manager == nullptr)
    {
        _manager = new QNetworkAccessManager(this);
        _manager->setTransferTimeout(30000);
        QObject::connect(_manager, SIGNAL(finished(QNetworkReply *)),
                         SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными

        QObject::connect(_manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                         SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
        QObject::connect(_manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                         SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));

    }
}



