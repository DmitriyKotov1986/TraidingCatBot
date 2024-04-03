//STL
#include <stdio.h>    //printf(3)
#include <stdlib.h>   //exit(3)
#include <unistd.h>   //fork(3), chdir(3), sysconf(3)
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)

//QT
#include <QCoreApplication>
#include <QTimer>
#include <QObject>
#include <QCommandLineParser>
#include <QtSql>

//My
#include "core.h"
#include "config.h"
#include "Common/common.h"
#include "log.h"
#include "deamon.h"

using namespace TradingCat;
using namespace Common;

extern bool needExit;

// This function will be called when the daemon receive a SIGHUP signal.
void reload()
{
    LOG_INFO("Get SIGHUP signal");
    needExit = true;
}

int main(int argc, char *argv[])
{
    bool demonMode = false;
    if (argc > 1)
    {
        if (std::string(argv[1]) == "--daemon")
        {
            // The Daemon class is a singleton to avoid be instantiate more than once
            Daemon& daemon = Daemon::instance();

            // Set the reload function to be called in case of receiving a SIGHUP signal
            daemon.setReloadFunction(reload);

            LOG_INFO("TradingCat start on demon mode");

            demonMode = true;
        }
    }

    QCoreApplication a(argc, argv);
    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("TradingCat");
    QCoreApplication::setOrganizationName("Cat software development");
    QCoreApplication::setApplicationVersion(QString("Version:0.1 Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    QDir::setCurrent(QCoreApplication::applicationDirPath());

    //читаем конфигурацию из файла
    QString configFileName = QString("%1/%2.ini").arg(QCoreApplication::applicationDirPath()).arg(QCoreApplication::applicationName());

    //Читаем конфигурацию
    auto cnf = Config::config(configFileName);
    if (cnf->isError()) {
        QString msg = "Error load configuration: " + cnf->errorString();
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(msg);

        return Common::EXIT_CODE::LOAD_CONFIG_ERR; // -1
    }

    //настраиваем таймер
    QTimer startTimer;
    startTimer.setInterval(0);       //таймер сработает так быстро как только это возможно
    startTimer.setSingleShot(true);  //таймер сработает 1 раз

    //создаем основной класс программы
    TradingCat::Core core(nullptr);

    //При запуске выполняем слот Start
    QObject::connect(&startTimer, SIGNAL(timeout()), &core, SLOT(start()));
    //Для завершения работы необходимоподать сигнал Finished
    QObject::connect(&core, SIGNAL(finished()), &a, SLOT(quit()));

    //запускаем таймер
    startTimer.start();
    //запускаем цикл обработчика событий
    auto res =  a.exec();

    Config::deleteConfig();

    if (demonMode)
    {
        LOG_INFO("TradingCat finished");
    }

    return res;
}
