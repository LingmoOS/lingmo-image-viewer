// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewerwindow.h"

#include "base/i18n.h"
#include "ui/application.h"

#include <string>

int main(int argc, char **argv)
{
    Lingmo::ui::Application app(argc, argv);
    app.setApplicationId("lingmo-image-viewer");
    Lingmo::i18n::setDomain("lingmo-image-viewer");

    ViewerWindow window;
    bool opened = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "-h" || argument == "--help") {
            std::printf("Usage: lingmo-image-viewer [IMAGE...]\n"
                        "Lingmo Image Viewer\n");
            return 0;
        }
        if (argument == "-v" || argument == "--version") {
            std::printf("lingmo-image-viewer %s\n", IMAGE_VIEWER_VERSION);
            return 0;
        }
        if (!argument.starts_with("-")) {
            opened = window.open(argument) || opened;
        }
    }
    window.window().show();
    return app.run();
}
