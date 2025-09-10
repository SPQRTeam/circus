#include <nanobind/nanobind.h>

namespace nb = nanobind;

#include <QSurfaceFormat>
#include <string>
#include <vector>

#include "AppWindow.h"
#include "CircusApplication.h"
#include "Constants.h"

namespace spqr {

// TODO: find better way to start main app?
int main(int argc, char** argv) {
    spqr::CircusApplication app(argc, argv);
    app.setApplicationName(spqr::appName);

    spqr::AppWindow window(argc, argv);
    window.show();

    return app.exec();
}

int run_main() {
    // Ensure there is at least a program name
    std::vector<std::string> args{};
    if (args.empty()) {
        args.push_back(spqr::appName);
    }

    // Build mutable, null-terminated argv storage
    std::vector<std::vector<char>> argbuf;
    argbuf.reserve(args.size());
    std::vector<char*> argv;
    argv.reserve(args.size());

    for (auto& s : args) {
        argbuf.emplace_back(s.begin(), s.end());
        argbuf.back().push_back('\0');
        argv.push_back(argbuf.back().data());
    }

    int argc = static_cast<int>(argv.size());
    // Call the application's main defined above.
    // Note: this will block the calling thread until the Qt event loop exits.
    return spqr::main(argc, argv.data());
}

namespace python {

NB_MODULE(circuspy, m) {
    m.def("main", &spqr::run_main,
          "Run the application's main(argc, argv). Blocks until Qt exits. "
          "Must be called from the process main thread and only once (QApplication is single-instance).");
}

}  // namespace python
}  // namespace spqr
