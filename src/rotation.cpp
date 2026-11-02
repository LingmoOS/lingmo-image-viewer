// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rotation.h"

#include <cstddef>
#include <cstdint>

namespace imageviewer {

Pixels rotatedPixels(const uint32_t* words, int width, int height, std::size_t stride, int quarterTurns)
{
    Pixels pixels;
    if (!words || width <= 0 || height <= 0)
        return pixels;

    const int turns = ((quarterTurns % 4) + 4) % 4;
    const std::size_t count = std::size_t(width) * std::size_t(height);
    const auto source = [&](int x, int y) {
        const uint8_t* row = reinterpret_cast<const uint8_t*>(words) + std::size_t(y) * stride;
        return reinterpret_cast<const uint32_t*>(row)[x];
    };

    pixels.width = turns % 2 == 0 ? width : height;
    pixels.height = turns % 2 == 0 ? height : width;
    pixels.words.resize(count);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int destinationX = x;
            int destinationY = y;
            switch (turns) {
            case 1:
                // Clockwise: the top left goes to the top right.
                destinationX = height - 1 - y;
                destinationY = x;
                break;
            case 2:
                destinationX = width - 1 - x;
                destinationY = height - 1 - y;
                break;
            case 3:
                destinationX = y;
                destinationY = width - 1 - x;
                break;
            default:
                break;
            }
            pixels.words[std::size_t(destinationY) * std::size_t(pixels.width) + std::size_t(destinationX)] =
                source(x, y);
        }
    }
    return pixels;
}

} // namespace imageviewer
