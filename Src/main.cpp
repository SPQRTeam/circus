
#include <QSurfaceFormat>

#include "AppWindow.h"
#include "CircusApplication.h"
#include "Constants.h"

int main(int argc, char** argv) {
    spqr::CircusApplication app(argc, argv);
    app.setApplicationName(spqr::appName);
    Q_INIT_RESOURCE(icon_logo);
    QIcon icon(":/spqr_logo.png");

    app.setWindowIcon(icon);
    spqr::AppWindow window(argc, argv);
    window.show();
    window.setWindowIcon(icon);

    return app.exec();
}
