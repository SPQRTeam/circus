#include "CircusApplication.h"

#include <QSurfaceFormat>

#include "curl/curl.h"

void installsigwatch(UnixSignalWatcher* sigwatch) {
    sigwatch->watchForSignal(SIGABRT);
    sigwatch->watchForSignal(SIGFPE);
    sigwatch->watchForSignal(SIGILL);
    sigwatch->watchForSignal(SIGINT);
    sigwatch->watchForSignal(SIGSEGV);
    sigwatch->watchForSignal(SIGTERM);
    sigwatch->watchForSignal(SIGQUIT);
    sigwatch->watchForSignal(SIGHUP);
    sigwatch->watchForSignal(SIGSYS);
}

namespace spqr {
CircusApplication::CircusApplication(int& argc, char** argv) : QApplication(argc, argv) {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapInterval(1);
    format.setVersion(2, 0);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    installsigwatch(&sigwatch);
    QObject::connect(&sigwatch, SIGNAL(unixSignal(int)), this, SLOT(quit()));
}

CircusApplication::~CircusApplication() {
    curl_global_cleanup();
}
}  // namespace spqr
