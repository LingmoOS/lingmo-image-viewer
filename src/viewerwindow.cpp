// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewerwindow.h"
#include "canvas.h"
#include "glyphs.h"
#include "rotation.h"
#include "widgets.h"

#include "ui/application.h"
#include "ui/bitmap.h"
#include "ui/file_chooser.h"
#include "ui/image_view.h"
#include "ui/keys.h"
#include "ui/theme.h"
#include "ui/toast.h"
#include "ui/window_header.h"

#include "base/event_loop.h"
#include "base/i18n.h"
#include "base/paths.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <poll.h>
#include <system_error>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace {

// The keys xkb gives for the zoom keys; '+' and '=' are the same physical
// key on most layouts, and the keypad has its own.
constexpr uint32_t kPlus = 0x2b;
constexpr uint32_t kMinus = 0x2d;
constexpr uint32_t kEqual = 0x3d;
constexpr uint32_t kKeypadAdd = 0xffab;
constexpr uint32_t kKeypadSubtract = 0xffad;

constexpr double kZoomStep = 1.25;
constexpr double kMinimumZoom = 0.05;
constexpr double kMaximumZoom = 16.0;

std::vector<std::string> imagePatterns()
{
    return {"*.png", "*.jpg", "*.jpeg", "*.webp", "*.tif", "*.tiff", "*.heif",
            "*.heic", "*.avif", "*.jxl", "*.bmp", "*.ico", "*.svg", "*.gif"};
}

} // namespace

ViewerWindow::ViewerWindow()
    : m_alive(std::make_shared<char>())
{
    glyphs::registerGlyphs();
    m_window.setTitle(tr("Image Viewer"));
    m_window.resize({1000, 720});
    m_window.setMinimumSize({560, 420});
    m_window.onCloseRequested = [this] { close(); };
    buildInterface();
    wireShortcuts();
    m_slideshowTimer.setInterval(std::chrono::milliseconds(3000));
    m_slideshowTimer.onTimeout = [this] {
        if (m_slideshow && !m_closed && m_list.size() > 1)
            next();
    };
    m_toast = std::make_unique<Toast>(&m_window);
    applyChrome();
    m_surface->setState(ImageSurface::State::Empty);
}

ViewerWindow::~ViewerWindow()
{
    m_alive.reset();
}

void ViewerWindow::buildInterface()
{
    auto root = std::make_unique<Column>();
    WindowHeader* header = root->addChild<WindowHeader>(40);
    header->setName("windowHeader");
    header->setLayoutHints({.fillWidth = true});
    m_headerContent = header->setContent<HeaderContent>();

    m_toolbar = root->addChild<ToolBar>();
    m_toolbar->setName("toolBar");
    m_toolbar->setLayoutHints({.fillWidth = true});

    m_surface = root->addChild<ImageSurface>();
    m_surface->setName("imageSurface");
    m_surface->setLayoutHints({.fillWidth = true, .fillHeight = true, .stretch = 1});

    m_status = root->addChild<StatusBar>();
    m_status->setName("statusBar");
    m_status->setLayoutHints({.fillWidth = true});

    m_window.setContentView(std::move(root));

    m_toolbar->onOpen = [this] { openChooser(); };
    m_toolbar->onPrevious = [this] { previous(); };
    m_toolbar->onNext = [this] { next(); };
    m_toolbar->onFit = [this] { setFit(); };
    m_toolbar->onActualSize = [this] { actualSize(); };
    m_toolbar->onZoomIn = [this] { zoomIn(); };
    m_toolbar->onZoomOut = [this] { zoomOut(); };
    m_toolbar->onRotateLeft = [this] { rotateLeft(); };
    m_toolbar->onRotateRight = [this] { rotateRight(); };
    m_toolbar->onSlideshow = [this] { toggleSlideshow(); };
    m_toolbar->onFullScreen = [this] { toggleFullScreen(); };

    ImageView* image = m_surface->imageView();
    image->setName("imageView");
    image->setFillMode(ImageView::FillMode::PreserveAspectFit);
    image->onStatusChanged = [this](ImageView::Status status) {
        switch (status) {
        case ImageView::Status::Loading:
            onImageLoading();
            break;
        case ImageView::Status::Ready:
            onImageReady();
            break;
        case ImageView::Status::Error:
            onImageFailed();
            break;
        case ImageView::Status::Null:
            break;
        }
    };

    m_surface->onOpenRequested = [this] { openChooser(); };
    m_surface->onToggleFit = [this] {
        if (m_mode == Mode::Fit)
            actualSize();
        else
            setFit();
    };
    m_surface->onZoomStep = [this](int direction) {
        if (direction > 0)
            zoomIn();
        else
            zoomOut();
    };
    m_surface->onFilesDropped = [this](const std::vector<std::string>& paths) { openPaths(paths); };
    m_surface->onContentChanged = [this] { scheduleFitUpdate(); };
}

void ViewerWindow::wireShortcuts()
{
    m_window.addShortcut(NoModifier, key::Escape, [this] { close(); });
    m_window.addShortcut(ControlModifier, key::letter('w'), [this] { close(); });
    m_window.addShortcut(ControlModifier, key::letter('o'), [this] { openChooser(); });

    m_window.addShortcut(NoModifier, key::Left, [this] { previous(); });
    m_window.addShortcut(NoModifier, key::Right, [this] { next(); });
    m_window.addShortcut(NoModifier, key::PageUp, [this] { previous(); });
    m_window.addShortcut(NoModifier, key::PageDown, [this] { next(); });

    m_window.addShortcut(NoModifier, key::letter('0'), [this] { setFit(); });
    m_window.addShortcut(NoModifier, key::letter('1'), [this] { actualSize(); });

    m_window.addShortcut(NoModifier, kPlus, [this] { zoomIn(); });
    m_window.addShortcut(ShiftModifier, kPlus, [this] { zoomIn(); });
    m_window.addShortcut(NoModifier, kEqual, [this] { zoomIn(); });
    m_window.addShortcut(NoModifier, kKeypadAdd, [this] { zoomIn(); });
    m_window.addShortcut(NoModifier, kMinus, [this] { zoomOut(); });
    m_window.addShortcut(NoModifier, kKeypadSubtract, [this] { zoomOut(); });

    m_window.addShortcut(NoModifier, key::letter('r'), [this] { rotateRight(); });
    m_window.addShortcut(ShiftModifier, key::letter('r'), [this] { rotateLeft(); });
    m_window.addShortcut(NoModifier, key::Space, [this] { toggleSlideshow(); });
    m_window.addShortcut(NoModifier, 0xffc8, [this] { toggleFullScreen(); });
}

void ViewerWindow::openChooser()
{
    m_chooser = std::make_unique<FileChooser>(&m_window);
    m_chooser->setMode(FileChooser::Mode::OpenFile);
    m_chooser->setTitle(tr("Open Image"));
    m_chooser->setFilters({{tr("Image Files"), imagePatterns(), {}}, {tr("All Files"), {"*"}, {}}});
    std::string folder;
    if (!m_currentPath.empty())
        folder = imageviewer::directoryOf(m_currentPath);
    if (folder.empty()) {
        std::error_code error;
        folder = Lingmo::base::paths::userDirectory(Lingmo::base::paths::UserDirectory::Pictures);
        if (!std::filesystem::is_directory(folder, error))
            folder = Lingmo::base::paths::home();
    }
    if (!folder.empty())
        m_chooser->setCurrentFolder(folder);
    m_chooser->onAccepted = [this](const std::vector<std::string>& paths, const std::string&) {
        if (!paths.empty())
            open(paths.front());
    };
    m_chooser->open();
}

bool ViewerWindow::open(const std::string& path)
{
    if (path.empty())
        return false;

    std::error_code error;
    std::string absolute = imageviewer::absolutePath(path);
    if (std::filesystem::is_directory(absolute, error)) {
        const std::vector<std::string> images = ImageList::imagesIn(absolute);
        if (images.empty()) {
            m_currentPath.clear();
            m_list = ImageList();
            m_haveSize = false;
            m_pendingProbe = false;
            m_scale = 1;
            m_surface->setImageDisplaySize({});
            displayError(tr("No pictures in this folder"), tr("Choose another folder or open a picture."));
            applyChrome();
            return false;
        }
        absolute = images.front();
    }

    const bool hadImage = m_haveSize && m_surface->imageDisplaySize().width > 0;
    m_currentPath = absolute;
    m_fileSize = 0;
    if (std::filesystem::is_regular_file(absolute, error)) {
        m_fileSize = uint64_t(std::filesystem::file_size(absolute, error));
        if (error)
            m_fileSize = 0;
    }
    m_list.setDirectory(imageviewer::directoryOf(absolute), absolute);
    if (m_slideshow && m_list.size() < 2) {
        m_slideshow = false;
        m_slideshowTimer.stop();
    }
    m_rotation = 0;
    m_mode = Mode::Fit;
    m_scale = 1;
    m_haveSize = false;
    m_pendingProbe = true;
    m_imageWidth = 0;
    m_imageHeight = 0;

    // A size to decode at before the picture's own is known: the room it
    // has, or a plain box while the window is still being laid out.
    if (!hadImage) {
        SizeF provisional = m_surface->contentSize();
        if (provisional.width < 64 || provisional.height < 64)
            provisional = {640, 480};
        m_surface->setImageDisplaySize(provisional);
    }
    m_surface->setState(ImageSurface::State::Loading);
    m_surface->imageView()->setSource(absolute);
    applyChrome();
    return true;
}

void ViewerWindow::openPaths(const std::vector<std::string>& paths)
{
    for (const std::string& path : paths) {
        if (!path.empty()) {
            open(path);
            return;
        }
    }
}

void ViewerWindow::onImageLoading()
{
    m_surface->setState(ImageSurface::State::Loading);
    applyChrome();
}

void ViewerWindow::onImageReady()
{
    if (m_pendingProbe) {
        m_pendingProbe = false;
        const SizeF size = m_surface->imageView()->sizeHint();
        if (size.width >= 1 && size.height >= 1) {
            m_imageWidth = int(std::lround(size.width));
            m_imageHeight = int(std::lround(size.height));
            m_haveSize = true;
        }
    }
    if (!m_haveSize) {
        onImageFailed();
        return;
    }
    if (m_mode == Mode::Fit)
        m_scale = fitScale();
    m_surface->setState(ImageSurface::State::Ready);
    applyGeometry();
    applyChrome();
}

void ViewerWindow::onImageFailed()
{
    m_pendingProbe = false;
    m_haveSize = false;
    m_scale = 1;
    m_surface->setImageDisplaySize({});
    displayError(tr("Can't open this image"),
                 m_currentPath.empty()
                     ? tr("The file may be damaged or in an unsupported format.")
                     : imageviewer::fileName(m_currentPath) + " — "
                           + tr("The file may be damaged or in an unsupported format."));
    applyChrome();
}

void ViewerWindow::displayError(const std::string& title, const std::string& detail)
{
    m_surface->setErrorText(title, detail);
    m_surface->setState(ImageSurface::State::Error);
}

void ViewerWindow::applyChrome()
{
    const std::string name = imageviewer::fileName(m_currentPath);
    m_headerContent->setFileName(name);
    m_status->setFile(name);
    if (m_haveSize) {
        m_status->setImageInfo(imageviewer::formatDimensions(m_imageWidth, m_imageHeight),
                               imageviewer::formatFileSize(m_fileSize));
    } else {
        m_status->setImageInfo({}, {});
    }
    m_status->setZoom(m_haveSize ? imageviewer::formatZoom(m_scale) : "—");
    m_status->setIndex(imageviewer::formatIndex(m_list.index(), m_list.size()));

    m_toolbar->setHasImage(m_haveSize);
    m_toolbar->setHasNeighbours(m_list.size() > 1);
    m_toolbar->setMode(m_mode == Mode::Fit, m_mode == Mode::Actual);
    m_toolbar->setZoomText(m_haveSize ? imageviewer::formatZoom(m_scale) : "—");
    m_toolbar->setSlideshow(m_slideshow);
    m_toolbar->setFullScreen(m_fullScreen);

    m_window.setTitle(name.empty() ? std::string(tr("Image Viewer")) : name + " — " + tr("Image Viewer"));
}

void ViewerWindow::applyGeometry()
{
    if (!m_haveSize)
        return;
    const SizeF effective = effectiveSize();
    const bool fitted = m_mode == Mode::Fit;
    const auto snap = [fitted](double value) {
        return fitted ? std::floor(value) : std::round(value);
    };
    m_surface->setImageDisplaySize({snap(effective.width * m_scale), snap(effective.height * m_scale)});
}

void ViewerWindow::scheduleFitUpdate()
{
    if (m_fitUpdateScheduled)
        return;
    base::EventLoop* loop = base::EventLoop::current();
    if (!loop)
        return;
    m_fitUpdateScheduled = true;
    const std::weak_ptr<char> alive = m_alive;
    loop->post([this, alive] {
        if (!alive.lock())
            return;
        m_fitUpdateScheduled = false;
        if (!m_haveSize || m_mode != Mode::Fit)
            return;
        const double scale = fitScale();
        if (std::abs(scale - m_scale) < 0.0005)
            return;
        m_scale = scale;
        applyGeometry();
        applyChrome();
    });
}

void ViewerWindow::setFit()
{
    if (!m_haveSize)
        return;
    m_mode = Mode::Fit;
    m_scale = fitScale();
    applyGeometry();
    applyChrome();
}

void ViewerWindow::actualSize()
{
    if (!m_haveSize)
        return;
    m_mode = Mode::Actual;
    m_scale = 1;
    applyGeometry();
    applyChrome();
}

void ViewerWindow::setManualScale(double scale)
{
    if (!m_haveSize)
        return;
    m_mode = Mode::Manual;
    m_scale = std::clamp(scale, kMinimumZoom, kMaximumZoom);
    applyGeometry();
    applyChrome();
}

void ViewerWindow::zoomIn()
{
    if (!m_haveSize)
        return;
    if (m_scale >= kMaximumZoom)
        return;
    setManualScale(m_scale * kZoomStep);
}

void ViewerWindow::zoomOut()
{
    if (!m_haveSize)
        return;
    if (m_scale <= kMinimumZoom)
        return;
    setManualScale(m_scale / kZoomStep);
}

void ViewerWindow::fitToWindow()
{
    setFit();
}

void ViewerWindow::rotateLeft()
{
    rotateBy(-1);
}

void ViewerWindow::rotateRight()
{
    rotateBy(1);
}

void ViewerWindow::rotateBy(int quarters)
{
    if (!m_haveSize || m_currentPath.empty())
        return;
    const int turn = ((m_rotation / 90 + quarters) % 4 + 4) % 4;
    const int rotation = turn * 90;
    if (rotation == m_rotation)
        return;
    ImageView* view = m_surface->imageView();
    if (rotation == 0) {
        // The file itself, again: the decode is on a thread of its own.
        m_rotation = 0;
        view->setSource(m_currentPath);
        applyGeometry();
        applyChrome();
        return;
    }

    Bitmap decoded = Bitmap::decodeFile(m_currentPath);
    if (decoded.isNull()) {
        showMessage(tr("Could not rotate this image"));
        return;
    }
    const imageviewer::Pixels turned = imageviewer::rotatedPixels(
        reinterpret_cast<const uint32_t*>(decoded.constBits()), decoded.width(), decoded.height(), decoded.stride(),
        turn);
    if (turned.width <= 0 || turned.height <= 0) {
        showMessage(tr("Could not rotate this image"));
        return;
    }
    m_rotation = rotation;
    view->setPixels(turned.width, turned.height, turned.words.data(), std::size_t(turned.width) * sizeof(uint32_t),
                    true, 1.0);
    if (m_mode == Mode::Fit)
        m_scale = fitScale();
    applyGeometry();
    m_surface->setState(ImageSurface::State::Ready);
    applyChrome();
}

void ViewerWindow::previous()
{
    if (m_list.size() < 2)
        return;
    if (m_list.navigate(-1))
        open(m_list.currentFile());
}

void ViewerWindow::next()
{
    if (m_list.size() < 2)
        return;
    if (m_list.navigate(1))
        open(m_list.currentFile());
}

void ViewerWindow::toggleSlideshow()
{
    if (m_slideshow) {
        m_slideshow = false;
        m_slideshowTimer.stop();
        m_toolbar->setSlideshow(false);
        showMessage(tr("Slideshow stopped"));
        return;
    }
    if (m_list.size() < 2) {
        showMessage(tr("Open a folder with more than one picture"));
        return;
    }
    m_slideshow = true;
    m_slideshowTimer.start();
    m_toolbar->setSlideshow(true);
    showMessage(tr("Slideshow started"));
}

void ViewerWindow::toggleFullScreen()
{
    m_fullScreen = !m_fullScreen;
    m_window.setFullScreen(m_fullScreen);
    m_toolbar->setFullScreen(m_fullScreen);
}

void ViewerWindow::close()
{
    if (m_closed)
        return;
    m_closed = true;
    m_slideshow = false;
    m_slideshowTimer.stop();
    if (Application* application = Application::instance())
        application->quit();
}

SizeF ViewerWindow::effectiveSize() const
{
    if (m_rotation == 90 || m_rotation == 270)
        return {double(m_imageHeight), double(m_imageWidth)};
    return {double(m_imageWidth), double(m_imageHeight)};
}

double ViewerWindow::fitScale() const
{
    if (!m_haveSize)
        return 1;
    const SizeF content = m_surface->contentSize();
    const SizeF size = effectiveSize();
    if (content.width < 1 || content.height < 1 || size.width < 1 || size.height < 1)
        return 1;
    const double fit = std::min(content.width / size.width, content.height / size.height);
    return std::clamp(fit, 0.02, 1.0);
}

int ViewerWindow::zoomPercent() const
{
    return int(std::lround(m_scale * 100));
}

void ViewerWindow::showMessage(const std::string& message)
{
    if (!m_toast)
        m_toast = std::make_unique<Toast>(&m_window);
    m_toast->show(message, Toast::Short);
}

void ViewerWindow::waitForImage(int timeoutMs)
{
    base::EventLoop* loop = base::EventLoop::current();
    if (!loop)
        return;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        const ImageSurface::State state = m_surface->state();
        if (state == ImageSurface::State::Ready || state == ImageSurface::State::Error)
            return;
        pollfd descriptor{loop->fd(), POLLIN, 0};
        ::poll(&descriptor, 1, 10);
        loop->dispatch();
    }
}
