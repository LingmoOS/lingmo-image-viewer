// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagelist.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <system_error>

namespace imageviewer {

namespace {

constexpr std::array<const char*, 14> kImageExtensions{
    ".png", ".jpg", ".jpeg", ".webp", ".tif", ".tiff", ".heif",
    ".heic", ".avif", ".jxl", ".bmp", ".ico", ".svg", ".gif",
};

std::string lowered(std::string_view text)
{
    std::string lower(text);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return lower;
}

std::string_view extension(std::string_view path)
{
    const std::size_t slash = path.find_last_of('/');
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash))
        return {};
    return path.substr(dot);
}

bool isDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

// A run of digits without its leading zeros, as the integer it names would
// be compared: fewer digits is smaller, else the digits themselves compare.
std::string_view number(std::string_view run)
{
    std::size_t begin = 0;
    while (begin + 1 < run.size() && run[begin] == '0')
        ++begin;
    return run.substr(begin);
}

// A number with one decimal below ten, and none above.
std::string formatUnit(double value, const char* unit)
{
    char text[32];
    if (value < 9.95)
        std::snprintf(text, sizeof(text), "%.1f %s", value, unit);
    else
        std::snprintf(text, sizeof(text), "%.0f %s", value, unit);
    return text;
}

} // namespace

bool isImageFile(const std::string& path)
{
    const std::string lower = lowered(extension(path));
    return std::find(kImageExtensions.begin(), kImageExtensions.end(), lower) != kImageExtensions.end();
}

bool naturalNameLess(const std::string& a, const std::string& b)
{
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (isDigit(a[i]) && isDigit(b[j])) {
            const std::size_t startA = i;
            const std::size_t startB = j;
            while (i < a.size() && isDigit(a[i]))
                ++i;
            while (j < b.size() && isDigit(b[j]))
                ++j;
            const std::string_view numberA = number(std::string_view(a).substr(startA, i - startA));
            const std::string_view numberB = number(std::string_view(b).substr(startB, j - startB));
            if (numberA.size() != numberB.size())
                return numberA.size() < numberB.size();
            const int compared = numberA.compare(numberB);
            if (compared != 0)
                return compared < 0;
            continue;
        }
        const unsigned char left = static_cast<unsigned char>(a[i]);
        const unsigned char right = static_cast<unsigned char>(b[j]);
        const int compared = std::tolower(left) - std::tolower(right);
        if (compared != 0)
            return compared < 0;
        ++i;
        ++j;
    }
    if (i == a.size() && j == b.size())
        return a < b;
    return i == a.size();
}

std::string fileName(const std::string& path)
{
    if (path.empty())
        return {};
    std::size_t end = path.size();
    while (end > 1 && path[end - 1] == '/')
        --end;
    const std::size_t slash = path.find_last_of('/', end - 1);
    return path.substr(slash == std::string::npos ? 0 : slash + 1, end - (slash == std::string::npos ? 0 : slash + 1));
}

std::string directoryOf(const std::string& path)
{
    if (path.empty())
        return {};
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return ".";
    if (slash == 0)
        return "/";
    return path.substr(0, slash);
}

std::string absolutePath(const std::string& path)
{
    std::error_code error;
    std::filesystem::path absolute = std::filesystem::absolute(path, error);
    if (error)
        absolute = std::filesystem::path(path);
    return absolute.lexically_normal().string();
}

std::string pathFromUri(const std::string& uri)
{
    std::string path = uri;
    if (path.starts_with("file://"))
        path.erase(0, 7);
    else if (path.starts_with("file:"))
        path.erase(0, 5);
    // An empty host leaves the path as it is.
    std::string decoded;
    decoded.reserve(path.size());
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '%' && i + 2 < path.size()
            && std::isxdigit(static_cast<unsigned char>(path[i + 1]))
            && std::isxdigit(static_cast<unsigned char>(path[i + 2]))) {
            const auto digit = [](char c) {
                return std::isdigit(static_cast<unsigned char>(c))
                    ? c - '0'
                    : std::tolower(static_cast<unsigned char>(c)) - 'a' + 10;
            };
            decoded.push_back(char(digit(path[i + 1]) * 16 + digit(path[i + 2])));
            i += 2;
            continue;
        }
        decoded.push_back(path[i]);
    }
    return decoded;
}

bool isImageUri(const std::string& uri)
{
    if (uri.empty())
        return false;
    std::string path = pathFromUri(uri);
    const std::size_t query = path.find_first_of("?#");
    if (query != std::string::npos)
        path.erase(query);
    return isImageFile(path);
}

std::string formatFileSize(uint64_t bytes)
{
    constexpr double kKib = 1024.0;
    constexpr double kMib = kKib * 1024.0;
    constexpr double kGib = kMib * 1024.0;
    if (bytes < uint64_t(kKib))
        return std::to_string(bytes) + " B";
    if (bytes < uint64_t(kMib))
        return formatUnit(double(bytes) / kKib, "KB");
    if (bytes < uint64_t(kGib))
        return formatUnit(double(bytes) / kMib, "MB");
    return formatUnit(double(bytes) / kGib, "GB");
}

std::string formatDimensions(int width, int height)
{
    if (width <= 0 || height <= 0)
        return "—";
    return std::to_string(width) + " × " + std::to_string(height);
}

std::string formatZoom(double scale)
{
    if (!(scale > 0))
        return "—";
    const double percent = scale * 100;
    char text[32];
    if (percent < 9.95)
        std::snprintf(text, sizeof(text), "%.1f%%", percent);
    else
        std::snprintf(text, sizeof(text), "%.0f%%", percent);
    return text;
}

std::string formatIndex(int index, std::size_t count)
{
    if (index < 0 || count == 0)
        return "—";
    return std::to_string(index + 1) + " / " + std::to_string(count);
}

void ImageList::setDirectory(const std::string& directory, const std::string& current)
{
    m_directory = absolutePath(directory);
    m_files = imagesIn(m_directory);
    m_index = -1;
    if (m_files.empty())
        return;
    if (!current.empty()) {
        const std::string absolute = absolutePath(current);
        const auto found = std::find(m_files.begin(), m_files.end(), absolute);
        if (found != m_files.end()) {
            m_index = int(found - m_files.begin());
            return;
        }
    }
    m_index = 0;
}

std::string ImageList::currentFile() const
{
    if (m_index < 0 || std::size_t(m_index) >= m_files.size())
        return {};
    return m_files[std::size_t(m_index)];
}

bool ImageList::contains(const std::string& file) const
{
    const std::string absolute = absolutePath(file);
    return std::find(m_files.begin(), m_files.end(), absolute) != m_files.end();
}

bool ImageList::select(const std::string& file)
{
    const std::string absolute = absolutePath(file);
    const auto found = std::find(m_files.begin(), m_files.end(), absolute);
    if (found == m_files.end())
        return false;
    m_index = int(found - m_files.begin());
    return true;
}

std::string ImageList::neighbour(int offset) const
{
    if (m_files.empty() || m_index < 0)
        return {};
    const int count = int(m_files.size());
    const int index = ((m_index + offset) % count + count) % count;
    return m_files[std::size_t(index)];
}

bool ImageList::navigate(int offset)
{
    if (m_files.empty() || offset == 0)
        return false;
    const int count = int(m_files.size());
    if (count == 1)
        return false;
    m_index = ((m_index + offset) % count + count) % count;
    return true;
}

std::vector<std::string> ImageList::imagesIn(const std::string& directory)
{
    std::vector<std::string> files;
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error);
    if (error)
        return files;
    for (const std::filesystem::directory_entry& entry : iterator) {
        std::error_code typeError;
        if (!entry.is_regular_file(typeError) || typeError)
            continue;
        std::string path = entry.path().string();
        if (!isImageFile(path))
            continue;
        files.push_back(std::move(path));
    }
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        const std::string nameA = fileName(a);
        const std::string nameB = fileName(b);
        if (nameA != nameB && naturalNameLess(nameA, nameB))
            return true;
        if (nameA != nameB && naturalNameLess(nameB, nameA))
            return false;
        return a < b;
    });
    return files;
}

} // namespace imageviewer
