//
// Created by Xenia on 1.09.2024.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>


class Logger {
public:
    Logger();
    ~Logger();
    void log(const std::string& message);
    const std::vector<std::string>& getLogs() { return logs; }
private:
    std::vector<std::string> logs;
    static std::string currentDateTime(const std::string& message);
};


#endif //LOGGER_H
