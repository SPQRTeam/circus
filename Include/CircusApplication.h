#pragma once
#include <QApplication>
#include "curl/curl.h"

namespace spqr {

class CircusApplication : public QApplication {
   public:
    CircusApplication(int& argc, char** argv);
    ~CircusApplication();
};

}  // namespace spqr
