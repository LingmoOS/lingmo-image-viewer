// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "glyphs.h"

#include "ui/icon.h"

namespace glyphs {

namespace {

// The toolkit draws a glyph's alpha in the tint asked for, so these are
// 16-point strokes on a 16 by 16 grid with currentColor, as its own are.
constexpr const char* kZoomIn =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="none">)"
    R"(<g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><circle cx="7" cy="7" r="4.4"/>)"
    R"(<path d="M10.4 10.4l3.1 3.1M4.9 7h4.2M7 4.9v4.2"/></g></svg>)";

constexpr const char* kZoomOut =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="none">)"
    R"(<g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"><circle cx="7" cy="7" r="4.4"/>)"
    R"(<path d="M10.4 10.4l3.1 3.1M4.9 7h4.2"/></g></svg>)";

constexpr const char* kFit =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="none">)"
    R"(<g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round">)"
    R"(<path d="M5.6 2.6H2.6v3M10.4 2.6h3v3M13.4 10.4v3h-3M5.6 13.4h-3v-3"/>)"
    R"(<path d="M2.6 2.6l3.3 3.3M13.4 2.6L10.1 5.9M13.4 13.4l-3.3-3.3M2.6 13.4l3.3-3.3"/></g></svg>)";

constexpr const char* kRotateLeft =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="none">)"
    R"(<g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round">)"
    R"(<path d="M2.8 8a5.2 5.2 0 1 1 1.6 3.75"/><path d="M2.8 3.4V8h4.4"/></g></svg>)";

constexpr const char* kRotateRight =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="none">)"
    R"(<g stroke="currentColor" stroke-linecap="round" stroke-linejoin="round">)"
    R"(<path d="M13.2 8a5.2 5.2 0 1 0-1.6 3.75"/><path d="M13.2 3.4V8H8.8"/></g></svg>)";

} // namespace

void registerGlyphs()
{
    using Lingmo::ui::Icon;
    Icon::registerGlyph("viewer-zoom-in", kZoomIn);
    Icon::registerGlyph("viewer-zoom-out", kZoomOut);
    Icon::registerGlyph("viewer-fit", kFit);
    Icon::registerGlyph("viewer-rotate-left", kRotateLeft);
    Icon::registerGlyph("viewer-rotate-right", kRotateRight);
}

} // namespace glyphs
