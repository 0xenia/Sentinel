//
// Created by Xenia on 1.09.2024.
//

#include "Logger.h"

Logger::Logger() = default;

Logger::~Logger() {
    logs.clear();
}

std::string Logger::currentDateTime(const std::string &message) {
    tm newtime{};
    const std::time_t now = std::time(nullptr);
    localtime_s(&newtime, &now);

    std::ostringstream timestamp;
    timestamp << "[" << std::put_time(&newtime, "%Y-%m-%d %H:%M:%S") << "] ";
    return timestamp.str() + message;
}


void Logger::log(const std::string &message) {
    std::string timeStampedMessage = currentDateTime(message);
    logs.emplace_back(timeStampedMessage);
}





