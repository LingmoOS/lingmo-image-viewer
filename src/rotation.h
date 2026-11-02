// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace imageviewer {

// Pixels turned a quarter turn at a time.
struct Pixels
{
    int width = 0;
    int height = 0;
    // 0xAARRGGBB, rows one after another.
    std::vector<uint32_t> words;
};

// The 0xAARRGGBB words at `words`, `height` rows `stride` bytes apart, turned
// `quarterTurns` quarter turns clockwise (negative turns anticlockwise). The
// words are moved as they are, so premultiplied pixels stay premultiplied.
Pixels rotatedPixels(const uint32_t* words, int width, int height, std::size_t stride, int quarterTurns);

} // namespace imageviewer
