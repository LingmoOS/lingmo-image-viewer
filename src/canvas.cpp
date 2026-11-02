// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canvas.h"
#include "imagelist.h"
#include "widgets.h"

#include "ui/color_provider.h"
#include "ui/icon.h"
#include "ui/image_view.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/progress_bar.h"
#include "ui/theme.h"

#include "base/i18n.h"

#include <algorithm>
#include <cmath>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace {

constexpr double kPlateMargin = 26;
constexpr double kImageRadius = 4;

// The quiet plate around the picture: a little deeper than the window in
// both modes, so the picture's own colours are what the eye sees.
Color backdropColor()
{
    const Theme& theme = Theme::current();
    return theme.darkMode ? Color::rgb(0x141416) : Color::rgb(0xE7E9EE);
}

} // namespace

EmptyState::EmptyState()
    : m_icon(addChild<Icon>("image", 64))
    , m_title(addChild<Label>())
    , m_detail(addChild<Label>())
    , m_button(addChild<ToolButton>())
{
    m_icon->setColor(themeText(0.22));
    m_title->setFontSize(19);
    m_title->setWeight(600);
    m_title->setTextAlignment(Label::TextAlignment::Center);
    m_detail->setColor(themeText(0.55));
    m_detail->setTextAlignment(Label::TextAlignment::Center);
    m_detail->setWrap(true);
    m_detail->setMaximumLineCount(3);
    m_button->setPrimary(true);
    m_button->onClicked = [this] {
        if (onButtonClicked)
            onButtonClicked();
    };
}

EmptyState::~EmptyState() = default;

void EmptyState::setTexts(std::string title, std::string detail, std::string button)
{
    m_title->setText(std::move(title));
    m_detail->setText(std::move(detail));
    m_button->setText(std::move(button));
    requestLayout();
}

void EmptyState::layout()
{
    const double width = geometry().width;
    const double height = geometry().height;
    const double iconSize = 64;
    const double detailWidth = std::max(180.0, std::min(440.0, width - 80));
    const double titleHeight = m_title->sizeHint().height;
    const double detailHeight = m_detail->heightForWidth(detailWidth);
    const double buttonHeight = m_button->sizeHint().height;
    const double gapIcon = 18;
    const double gapTitle = 8;
    const double gapButton = 24;
    const double content = iconSize + gapIcon + titleHeight + gapTitle + detailHeight + gapButton + buttonHeight;
    double y = std::floor((height - content) / 2);
    y = std::max(y, 24.0);

    m_icon->setGeometry({std::round((width - iconSize) / 2), y, iconSize, iconSize});
    y += iconSize + gapIcon;
    const SizeF title = m_title->sizeHint();
    m_title->setGeometry({std::round((width - title.width) / 2), y, title.width, titleHeight});
    y += titleHeight + gapTitle;
    m_detail->setGeometry({std::round((width - detailWidth) / 2), y, detailWidth, detailHeight});
    y += detailHeight + gapButton;
    const SizeF button = m_button->sizeHint();
    m_button->setGeometry({std::round((width - button.width) / 2), y, button.width, buttonHeight});
}

LoadingView::LoadingView()
    : m_indicator(addChild<BusyIndicator>())
    , m_label(addChild<Label>(tr("Loading image…")))
{
    m_label->setColor(themeText(0.6));
}

LoadingView::~LoadingView() = default;

void LoadingView::layout()
{
    const double width = geometry().width;
    const double height = geometry().height;
    const SizeF label = m_label->sizeHint();
    const SizeF indicator = m_indicator->sizeHint();
    const double gap = 10;
    const double total = indicator.width + gap + label.width;
    const double x = std::round((width - total) / 2);
    m_indicator->setGeometry({x, std::round((height - indicator.height) / 2), indicator.width, indicator.height});
    m_label->setGeometry({x + indicator.width + gap, std::round((height - label.height) / 2), label.width,
                          label.height});
}

void LoadingView::paint(Painter& painter)
{
    // A plate of its own, so the turning mark stays readable over a picture
    // that is still there.
    const double padding = 14;
    const SizeF label = m_label->sizeHint();
    const SizeF indicator = m_indicator->sizeHint();
    const double width = indicator.width + 10 + label.width + padding * 2;
    const double height = std::max(indicator.height, label.height) + padding;
    const RectF plate{std::round((geometry().width - width) / 2), std::round((geometry().height - height) / 2), width,
                      height};
    painter.drawShadow(plate, height / 2, 10, Color(0, 0, 0, 90), {0, 3});
    painter.fillRoundedRect(plate, height / 2, color(ColorId::SecondBackground));
}

ImageSurface::ImageSurface()
    : m_image(addChild<ImageView>())
    , m_empty(addChild<EmptyState>())
    , m_loading(addChild<LoadingView>())
    , m_dropHint(addChild<Label>(tr("Drop to open")))
{
    setClipsChildren(true);
    m_image->setFillMode(ImageView::FillMode::PreserveAspectFit);
    m_image->setSmooth(true);
    m_image->setVisible(false);
    m_empty->setVisible(false);
    m_loading->setVisible(false);
    m_dropHint->setColor(themeText(1.0));
    m_dropHint->setWeight(600);
    m_dropHint->setTextAlignment(Label::TextAlignment::Center);
    m_dropHint->setVerticallyCentered(true);
    m_dropHint->setVisible(false);
    m_empty->setTexts(tr("No image open"), tr("Drag and drop pictures here, or press Ctrl+O"),
                      tr("Open Image"));

    filter = [](const std::string& url) { return imageviewer::isImageUri(url); };
    onDropped = [this](const std::vector<std::string>& urls) {
        std::vector<std::string> paths;
        paths.reserve(urls.size());
        for (const std::string& url : urls)
            paths.push_back(imageviewer::pathFromUri(url));
        if (!paths.empty() && onFilesDropped)
            onFilesDropped(paths);
    };
    onContainsDragChanged = [this] {
        m_dropHint->setVisible(containsDrag());
        requestLayout();
    };

    m_empty->onButtonClicked = [this] {
        if (onOpenRequested)
            onOpenRequested();
    };
}

ImageSurface::~ImageSurface() = default;

void ImageSurface::setState(State state)
{
    m_state = state;
    const bool hasPicture = m_displaySize.width > 0 && m_displaySize.height > 0;
    m_image->setVisible((state == State::Ready || state == State::Loading) && hasPicture);
    m_empty->setVisible(state == State::Empty || state == State::Error);
    m_loading->setVisible(state == State::Loading && !hasPicture);
    requestPaint();
}

void ImageSurface::setErrorText(const std::string& title, const std::string& detail)
{
    m_empty->setTexts(title, detail, tr("Open Another Image"));
}

void ImageSurface::setImageDisplaySize(SizeF size)
{
    if (size == m_displaySize)
        return;
    m_displaySize = size;
    requestLayout();
    requestPaint();
}

void ImageSurface::layout()
{
    const RectF rect = localRect();
    const SizeF content{std::max(0.0, rect.width - kPlateMargin * 2),
                        std::max(0.0, rect.height - kPlateMargin * 2)};

    // The picture, centred on whole pixels.
    if (m_displaySize.width > 0 && m_displaySize.height > 0) {
        m_imageRect = {std::round(rect.x + (rect.width - m_displaySize.width) / 2),
                       std::round(rect.y + (rect.height - m_displaySize.height) / 2), m_displaySize.width,
                       m_displaySize.height};
    } else {
        m_imageRect = {};
    }

    // The messages fill the area and centre themselves; the picture is
    // placed where the window asked for it.
    for (const auto& child : children()) {
        if (child.get() != m_image)
            child->setGeometry(rect);
    }
    m_image->setGeometry(m_imageRect);

    // A plate over the bottom of the picture, while a drop is over it.
    const SizeF hint = m_dropHint->sizeHint();
    const SizeF plate{hint.width + 32, hint.height + 20};
    m_dropHint->setGeometry({std::round((rect.width - hint.width) / 2), std::round(rect.height - plate.height - 20),
                             hint.width, hint.height});

    if (content != m_lastContentSize) {
        m_lastContentSize = content;
        m_contentSize = content;
        if (onContentChanged)
            onContentChanged();
    } else {
        m_contentSize = content;
    }
}

void ImageSurface::paint(Painter& painter)
{
    const RectF rect = localRect();
    painter.fillRect(rect, backdropColor());

    if ((m_state == State::Ready || m_state == State::Loading) && m_imageRect.width > 0
        && m_imageRect.height > 0) {
        const Theme& theme = Theme::current();
        const Color shade = theme.darkMode ? Color(0, 0, 0, 120) : Color(0, 0, 0, 42);
        painter.drawShadow(m_imageRect, kImageRadius, 16, shade, {0, 6});
        painter.strokeRoundedRect(m_imageRect, kImageRadius,
                                  theme.darkMode ? Color(255, 255, 255, 26) : Color(0, 0, 0, 20));
    }

    if (containsDrag()) {
        const Color accent = color(ColorId::Highlight);
        const RectF inner = rect.shrunk(Margins::all(3));
        painter.fillRoundedRect(inner, Theme::current().bigRadius, accent.withAlpha(0.07));
        painter.drawRoundedRect(inner, {Theme::current().bigRadius, 0, Corners::Circular}, Color{0, 0, 0, 0}, 2.0,
                                accent.withAlpha(0.7));
        if (m_dropHint->isVisible()) {
            const RectF plate{m_dropHint->geometry().x - 16, m_dropHint->geometry().y - 10,
                              m_dropHint->geometry().width + 32, m_dropHint->geometry().height + 20};
            painter.fillRoundedRect(plate, plate.height / 2, accent);
        }
    }
}

bool ImageSurface::scroll(const ScrollEvent& event)
{
    if (m_state != State::Ready || !onZoomStep || event.deltaY == 0)
        return false;
    onZoomStep(event.deltaY < 0 ? 1 : -1);
    return true;
}

void ImageSurface::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left || event.clickCount != 2 || m_state != State::Ready)
        return;
    if (onToggleFit)
        onToggleFit();
}
