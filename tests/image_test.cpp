// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagelist.h"
#include "rotation.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#ifndef IMAGE_VIEWER_SOURCE_DIR
#define IMAGE_VIEWER_SOURCE_DIR "."
#endif

using imageviewer::ImageList;
using imageviewer::formatFileSize;
using imageviewer::formatZoom;
using imageviewer::naturalNameLess;
using imageviewer::rotatedPixels;

TEST(ImageList, SortsNaturallyAndWraps) {
    EXPECT_TRUE(naturalNameLess("img2.png", "img10.png"));
    EXPECT_FALSE(naturalNameLess("img10.png", "img2.png"));
    EXPECT_TRUE(naturalNameLess("A.png", "b.png"));

    const std::filesystem::path data = std::filesystem::path(IMAGE_VIEWER_SOURCE_DIR) / "tests/data";
    ImageList list;
    list.setDirectory(data.string(), (data / "lingmo-sample.png").string());
    ASSERT_FALSE(list.empty());
    EXPECT_TRUE(list.contains((data / "lingmo-sample.png").string()));
    EXPECT_EQ(imageviewer::fileName(list.currentFile()), "lingmo-sample.png");
    EXPECT_FALSE(list.neighbour(1).empty());
    EXPECT_FALSE(list.neighbour(-1).empty());
}

TEST(ImageList, FormatsSizesAndZooms) {
    EXPECT_EQ(formatFileSize(0), "0 B");
    EXPECT_EQ(formatZoom(1.0), "100%");
    EXPECT_EQ(formatZoom(0.075), "7.5%");
}

TEST(Rotation, TurnsASmallImageClockwise) {
    const uint32_t pixels[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF};
    const imageviewer::Pixels turned = rotatedPixels(pixels, 2, 2, 2 * sizeof(uint32_t), 1);
    ASSERT_EQ(turned.width, 2);
    ASSERT_EQ(turned.height, 2);
    EXPECT_EQ(turned.words[0], 0xFF0000FF);
    EXPECT_EQ(turned.words[1], 0xFFFF0000);
}
