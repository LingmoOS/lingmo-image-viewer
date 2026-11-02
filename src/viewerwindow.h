// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "imagelist.h"

#include "ui/geometry.h"
#include "ui/window.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using imageviewer::ImageList;

namespace Lingmo::ui {
class FileChooser;
class Toast;
}

class HeaderContent;
class ImageSurface;
class StatusBar;
class ToolBar;

// The picture window: a title bar with the picture's name, the view controls,
// the picture itself and a line of details under it. It keeps the folder's
// pictures, so the arrows step through them without going back to a dialog.
class ViewerWindow
{
public:
    // How the picture is sized: fitted to the window, at its own pixels, or
    // at a zoom the user stepped to.
    enum class Mode {
        Fit,
        Actual,
        Manual,
    };

    ViewerWindow();
    ~ViewerWindow();

    ViewerWindow(const ViewerWindow&) = delete;
    ViewerWindow& operator=(const ViewerWindow&) = delete;

    Lingmo::ui::Window& window() { return m_window; }
    ImageSurface* surface() const { return m_surface; }

    // Opens the picture, or the first picture of the folder it names, and
    // takes the folder's pictures as the ones to step through.
    bool open(const std::string& path);
    void openPaths(const std::vector<std::string>& paths);

    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void actualSize();
    void rotateLeft();
    void rotateRight();
    void previous();
    void next();
    // Escape and the window's close button: leaves the application.
    void close();

    // What the window shows, for the status line and the tests.
    const std::string& currentPath() const { return m_currentPath; }
    Mode mode() const { return m_mode; }
    double scale() const { return m_scale; }
    int zoomPercent() const;
    // 0, 90, 180 or 270, clockwise.
    int rotation() const { return m_rotation; }
    bool hasImage() const { return m_haveSize; }
    bool isClosed() const { return m_closed; }
    const ImageList& list() const { return m_list; }

    // Runs the event loop until the picture finished loading or failed.
    void waitForImage(int timeoutMs);

private:
    void buildInterface();
    void wireShortcuts();
    void openChooser();
    void onImageLoading();
    void onImageReady();
    void onImageFailed();
    void applyChrome();
    void applyGeometry();
    void scheduleFitUpdate();
    void setFit();
    void setManualScale(double scale);
    void rotateBy(int quarters);
    void displayError(const std::string& title, const std::string& detail);
    Lingmo::ui::SizeF effectiveSize() const;
    double fitScale() const;
    void showMessage(const std::string& message);

    std::shared_ptr<char> m_alive;
    Lingmo::ui::Window m_window;
    HeaderContent* m_headerContent = nullptr;
    ToolBar* m_toolbar = nullptr;
    ImageSurface* m_surface = nullptr;
    StatusBar* m_status = nullptr;

    ImageList m_list;
    std::string m_currentPath;
    std::uint64_t m_fileSize = 0;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    int m_rotation = 0;
    Mode m_mode = Mode::Fit;
    double m_scale = 1;
    bool m_haveSize = false;
    bool m_pendingProbe = false;
    bool m_closed = false;
    bool m_fitUpdateScheduled = false;

    std::unique_ptr<Lingmo::ui::FileChooser> m_chooser;
    std::unique_ptr<Lingmo::ui::Toast> m_toast;
};
