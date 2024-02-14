#ifndef TCONFIG_H
#define TCONFIG_H

//QT
#include <QString>

namespace TraidingCatBot
{

class Config final
{
public:
    static Config* config(const QString& configFileName = "");
    static void deleteConfig();

private:
    explicit Config(const QString& configFileName);

public:
    bool save();

    //[STOCKEXCHANGES]
    bool stockExcange_MEXC_enable() const;

    QString errorString();
    bool isError() const;

private:
    const QString _configFileName;

    QString _errorString;

    //[STOCKEXCHANGES]
    bool _stockExcange_MEXC_enable = true;

};

} //namespace TraidingCatBot

#endif // TCONFIG_H
