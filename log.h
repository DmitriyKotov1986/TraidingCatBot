#ifndef LOG_H
#define LOG_H

#include <iostream>

#define LOG_INFO(...) Log(std::clog, "INFO").write(__VA_ARGS__)
#define LOG_WARNING(...) Log(std::clog, "WARNING", __FUNCTION__, __LINE__).write(__VA_ARGS__)
#define LOG_ERROR(...) Log(std::clog, "ERROR", __FUNCTION__, __LINE__).write(__VA_ARGS__)
#define LOG_DEBUG(...) Log(std::clog, "DEBUG", __FUNCTION__, __LINE__).write(__VA_ARGS__)

template <typename TF>
void writeLog(std::ostream &out, TF const &f)
{
    out << f;
}

struct Log
{
    std::ostream &out;

    Log(std::ostream &out, const char *type, const char *functionName, int line);
    Log(std::ostream &out, const char *type);

    ~Log();

    template <typename TF, typename... TR>
    void write(TF const &f, TR const &... rest)
    {
        writeLog(out, f);
        write(rest...);
    }

    template <typename TF>
    void write(TF const &f)
    {
        writeLog(out, f);
    }

    void write() {};  // Handle the empty params case
};

#endif // LOG_H
