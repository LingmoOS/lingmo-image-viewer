// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewerwindow.h"

#include "base/event_loop.h"
#include "ui/application.h"
#include "ui/theme.h"
#include "ui/window.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <poll.h>
#include <string>

using namespace Lingmo;
using namespace Lingmo::ui;

namespace {

void spin(int milliseconds)
{
    base::EventLoop &loop = Application::instance()->loop();
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() < end) {
        pollfd descriptor{loop.fd(), POLLIN, 0};
        ::poll(&descriptor, 1, 10);
        loop.dispatch();
    }
}

}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s OUTDIR [IMAGE]\n", argv[0]);
        return 1;
    }
    const std::string directory = argv[1];
    const std::string image = argc > 2 ? argv[2] : IMAGE_VIEWER_SOURCE_DIR "/tests/data/lingmo-sample.png";
    std::filesystem::create_directories(directory);

    Application app(argc, argv, Application::Platform::Headless);
    app.setApplicationId("lingmo-image-viewer");

    for (const bool dark : {false, true}) {
        Theme::setCurrent(dark ? Theme::dark() : Theme::light());
        ViewerWindow window;
        window.window().setScale(1.5);
        window.open(image);
        window.window().show();
        window.waitForImage(3000);
        // ImageView decodes on its own thread and needs frames to show the
        // result; render a few frames before taking the picture.
        for (int i = 0; i < 20; ++i) {
            window.window().grab();
            spin(30);
        }
        window.window().grab().savePng(directory + "/image-viewer-" + (dark ? "dark" : "light") + ".png");
    }
    return 0;
}
