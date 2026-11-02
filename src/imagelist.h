// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// The viewer's filesystem and formatting work, without a window or the
// toolkit, so the tests can use it without a display.
namespace imageviewer {

// The extensions the viewer opens; the toolkit's ImageView decodes them.
bool isImageFile(const std::string& path);

// The order the files of a folder are shown in: case-insensitive, with runs
// of digits compared as numbers, so "img2.png" comes before "img10.png".
bool naturalNameLess(const std::string& a, const std::string& b);

// The last part of the path, and all but it; "." before a name is kept.
std::string fileName(const std::string& path);
std::string directoryOf(const std::string& path);
// The absolute path, cleaned, without requiring the file to exist.
std::string absolutePath(const std::string& path);

// A file:// URL as a local path, or the text itself when it is not one.
// Percent escapes are decoded.
std::string pathFromUri(const std::string& uri);
bool isImageUri(const std::string& uri);

// "812 B", "42.3 KB", "1.4 MB": one decimal below ten of a unit.
std::string formatFileSize(uint64_t bytes);
// "1024 × 640", or a dash when the size is not known.
std::string formatDimensions(int width, int height);
// The zoom as a percentage: "92%", or "7.5%" below ten.
std::string formatZoom(double scale);
// "2 / 6", or a dash when there is no list.
std::string formatIndex(int index, std::size_t count);

// The pictures of one folder, and which of them is the current one. Moving
// wraps round the ends, so the arrows never stop in the middle of a folder.
class ImageList
{
public:
    // The folders' pictures, sorted; `current` picks the place to start at,
    // else the first picture.
    void setDirectory(const std::string& directory, const std::string& current = {});

    const std::string& directory() const { return m_directory; }
    const std::vector<std::string>& files() const { return m_files; }
    bool empty() const { return m_files.empty(); }
    std::size_t size() const { return m_files.size(); }
    // Where the current picture is, or -1 when none is.
    int index() const { return m_index; }
    std::string currentFile() const;
    bool contains(const std::string& file) const;
    // Picks a picture already in the folder; false when it is not one.
    bool select(const std::string& file);
    // The picture `offset` places away, wrapping round; empty when none.
    std::string neighbour(int offset) const;
    // Moves to the neighbour; false when the folder has nowhere else to go.
    bool navigate(int offset);

    // Every picture file directly inside the directory, sorted.
    static std::vector<std::string> imagesIn(const std::string& directory);

private:
    std::string m_directory;
    std::vector<std::string> m_files;
    int m_index = -1;
};

} // namespace imageviewer
