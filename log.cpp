#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "log.h"

std::string getLogTime(std::chrono::time_point<std::chrono::system_clock> time)
{
    auto epoch_seconds = std::chrono::system_clock::to_time_t(time);
    std::stringstream stream;
    stream << std::put_time(gmtime(&epoch_seconds), "[%F_%T");
    auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);
    auto delta_us = std::chrono::duration_cast<std::chrono::milliseconds>(time - truncated).count();
    stream << "." << std::fixed << std::setw(3) << std::setfill('0') << delta_us << "]";
    return stream.str();
}

Log::Log(std::ostream &out, const char *type, const char *functionName, int line)
    : out(out)
{
    out << getLogTime(std::chrono::system_clock::now()) << " " << type << " " << line << ":" << functionName << "()-> ";
}

Log::Log(std::ostream &out, const char *type)
    : out(out)
{
    out << getLogTime(std::chrono::system_clock::now()) << " " << type << "-> ";
}

Log::~Log()
{
    out << std::endl;
}
