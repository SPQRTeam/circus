#pragma once
#include <QApplication>

#include "sigwatch.h"
namespace spqr {

class CircusApplication : public QApplication {
   public:
    CircusApplication(int& argc, char** argv);
    ~CircusApplication();

   private:
    UnixSignalWatcher sigwatch;
};

}  // namespace spqr
