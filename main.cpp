#include <QCoreApplication>
#include <QTimer>
#include <QObject>

//My
#include "core.h"

using namespace TraidingCatBot;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("TraidingCatBot");
    QCoreApplication::setOrganizationName("Cat softwre development");
    QCoreApplication::setApplicationVersion(QString("Version:0.1 Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

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

    return res;
}
