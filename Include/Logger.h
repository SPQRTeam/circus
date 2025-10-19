#pragma once

#include <Eigen/Eigen>

#include <fstream>
#include <iostream>

namespace spqr {
    class Logger {
    public:
    std::ofstream logFile;

    Logger(const std::string& filename, std::string header) {
        logFile.open(filename);
        if (header.back() != '\n')
            header += '\n';
        logFile << header;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        bool first = true;
        ((oss << (first ? "" : ",") << args, first = false), ...);
        logFile << oss.str() << "\n";
    }

    ~Logger() {
        logFile.close();
    }
};
}