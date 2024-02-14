#include <QCoreApplication>
#include <QTimer>
#include <QObject>
#include <QCommandLineParser>

//My
#include "core.h"
#include "config.h"

using namespace TraidingCatBot;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("TraidingCatBot");
    QCoreApplication::setOrganizationName("Cat softwre development");
    QCoreApplication::setApplicationVersion(QString("Version:0.1 Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    //Создаем парсер параметров командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Traiding Cat Bot");
    parser.addHelpOption();
    parser.addVersionOption();

    //добавляем опцию Config
    QCommandLineOption config(QStringList() << "Config", "Config file name", "ConfigFileNameValue",
                              QString("%1/%2.ini").arg(QCoreApplication::applicationDirPath()).arg(QCoreApplication::applicationName()));
    parser.addOption(config);

    //Парсим опции командной строки
    parser.process(a);

    //читаем конфигурацию из файла
    QString configFileName = parser.value(config);

    //Читаем конфигурацию
    auto cnf = Config::config(configFileName);
    if (cnf->isError()) {
        QString msg = "Error load configuration: " + cnf->errorString();
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);

        return -1; // -1
    }

    //настраиваем таймер
    QTimer startTimer;
    startTimer.setInterval(0);       //таймер сработает так быстро как только это возможно
    startTimer.setSingleShot(true);  //таймер сработает 1 раз

    //создаем основной класс программы
    TraidingCatBot::Core core(nullptr);

    //При запуске выполняем слот Start
    QObject::connect(&startTimer, SIGNAL(timeout()), &core, SLOT(start()));
    //Для завершения работы необходимоподать сигнал Finished
    QObject::connect(&core, SIGNAL(finished()), &a, SLOT(quit()));

    //запускаем таймер
    startTimer.start();
    //запускаем цикл обработчика событий
    auto res =  a.exec();

    Config::deleteConfig();

    return res;
}
