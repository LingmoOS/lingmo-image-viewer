// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewerwindow.h"

#include "ui/application.h"

#include <gtest/gtest.h>

#ifndef IMAGE_VIEWER_SOURCE_DIR
#define IMAGE_VIEWER_SOURCE_DIR "."
#endif

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    Lingmo::ui::Application app(argc, argv, Lingmo::ui::Application::Platform::Headless);
    return RUN_ALL_TESTS();
}

TEST(ViewerWindow, OpensTheSampleAndTogglesExtras)
{
    ViewerWindow window;
    ASSERT_TRUE(window.open(std::string(IMAGE_VIEWER_SOURCE_DIR) + "/tests/data/lingmo-sample.png"));
    window.window().show();
    for (int i = 0; i < 10 && !window.hasImage(); ++i) {
        window.waitForImage(300);
        window.window().grab();
    }
    ASSERT_TRUE(window.hasImage());
    EXPECT_EQ(window.list().size(), 3u);

    EXPECT_FALSE(window.slideshowActive());
    window.toggleSlideshow();
    EXPECT_TRUE(window.slideshowActive());
    window.toggleSlideshow();
    EXPECT_FALSE(window.slideshowActive());

    EXPECT_FALSE(window.isFullScreen());
    window.toggleFullScreen();
    EXPECT_TRUE(window.isFullScreen());
    window.toggleFullScreen();
    EXPECT_FALSE(window.isFullScreen());
}
