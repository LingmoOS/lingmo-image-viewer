// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "widgets.h"

#include "ui/color_provider.h"
#include "ui/icon.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/theme.h"

#include "base/i18n.h"

#include <algorithm>
#include <cmath>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace {

constexpr double kButtonHeight = 32;
constexpr double kButtonPadding = 13;
constexpr double kGap = 7;

} // namespace

ColorRecipe themeText(double alpha)
{
    return ColorRecipe([alpha](const ColorProvider&) { return Theme::current().text.withAlpha(alpha); });
}

ColorRecipe themeAccent(double alpha)
{
    return ColorRecipe([alpha](const ColorProvider&) { return Theme::current().highlight.withAlpha(alpha); });
}

void paintBar(Painter& painter, const RectF& rect, BarEdge edge, bool accentPlate)
{
    painter.fillRect(rect, accentPlate ? color(ColorId::AlternateBackground) : color(ColorId::SecondBackground));
    if (edge == BarEdge::None)
        return;
    const double hairline = 1.0 / painter.scale();
    const double y = edge == BarEdge::Bottom ? rect.bottom() - hairline : rect.y;
    painter.fillRect({rect.x, y, rect.width, hairline}, color(ColorId::HeaderSeparator));
}

void Separator::paint(Painter& painter)
{
    const double width = 1.0 / painter.scale();
    const double x = std::floor(geometry().width / 2);
    const double inset = std::floor((geometry().height - 18) / 2);
    painter.fillRect({x, inset, width, geometry().height - inset * 2}, color(ColorId::Separator));
}

ToolButton::ToolButton(std::string text)
    : m_text(std::move(text))
{
    m_height = kButtonHeight;
}

ToolButton::~ToolButton() = default;

void ToolButton::setText(std::string text)
{
    m_text = std::move(text);
    if (m_label) {
        m_label->setText(m_text);
    } else if (!m_text.empty()) {
        m_label = addChild<Label>(m_text);
        m_label->setVerticallyCentered(true);
    }
    if (m_icon)
        requestLayout();
    else
        requestLayout();
    requestPaint();
}

void ToolButton::setGlyph(std::string name, double iconSize)
{
    m_iconSize = iconSize;
    if (!m_icon) {
        m_icon = addChild<Icon>(std::move(name), iconSize);
    } else {
        m_icon->setIconName(std::move(name));
        m_icon->setSize(iconSize);
    }
    requestLayout();
    requestPaint();
}

void ToolButton::setPrimary(bool primary)
{
    if (primary == m_primary)
        return;
    m_primary = primary;
    updateColors();
    requestPaint();
}

void ToolButton::setChecked(bool checked)
{
    if (checked == m_checked)
        return;
    m_checked = checked;
    updateColors();
    requestPaint();
}

SizeF ToolButton::sizeHint() const
{
    const SizeF label = m_label ? m_label->sizeHint() : SizeF{};
    double width = label.width;
    if (m_icon) {
        width += m_iconSize;
        if (!m_text.empty())
            width += kGap;
    }
    width += kButtonPadding * 2;
    return {std::ceil(width), kButtonHeight};
}

void ToolButton::layout()
{
    const SizeF label = m_label ? m_label->sizeHint() : SizeF{};
    double content = label.width;
    if (m_icon) {
        content += m_iconSize;
        if (!m_text.empty())
            content += kGap;
    }
    double x = std::floor((geometry().width - content) / 2);
    if (m_icon) {
        m_icon->setGeometry({x, std::floor((geometry().height - m_iconSize) / 2), m_iconSize, m_iconSize});
        x += m_iconSize + (m_text.empty() ? 0 : kGap);
    }
    if (m_label) {
        m_label->setGeometry({x, std::floor((geometry().height - label.height) / 2), label.width, label.height});
        m_label->setTextAlignment(Label::TextAlignment::Center);
    }
    updateColors();
}

void ToolButton::updateColors()
{
    const bool enabled = isEnabled();
    std::optional<ColorRecipe> tint;
    if (!enabled)
        tint = ColorRecipe(color(ColorId::DisabledText));
    else if (m_primary)
        tint = ColorRecipe(color(ColorId::HighlightedText));
    else if (m_checked)
        tint = themeAccent();
    else
        tint = themeText();
    if (m_icon) {
        m_icon->setColor(tint);
        m_icon->setOpacity(enabled ? 1 : 0.75);
    }
    if (m_label) {
        m_label->setColor(tint);
        m_label->setFadesWhenDisabled(false);
    }
}

void ToolButton::paint(Painter& painter)
{
    const bool enabled = isEnabled();
    Color fill{0, 0, 0, 0};
    if (enabled) {
        if (m_primary) {
            fill = color(ColorId::Highlight);
            if (m_pressed)
                fill = fill.darker(0.92);
            else if (m_hovered)
                fill = fill.lighter(1.08);
        } else if (m_pressed) {
            fill = color(ColorId::ButtonBackgroundPressed);
        } else if (m_hovered) {
            fill = color(ColorId::ButtonBackgroundHovered);
        } else if (m_checked) {
            fill = color(ColorId::Highlight).withAlpha(0.16);
        }
    }
    const double radius = m_height / 2;
    if (fill.a > 0) {
        painter.drawRoundedRect(localRect(), {radius, 0, Corners::Circular}, fill);
        // A filled, primary plate takes a hairline of its own so a light
        // accent does not melt into the bar.
        if (m_primary)
            painter.drawRoundedRect(localRect(), {radius, 0, Corners::Circular}, Color{0, 0, 0, 0}, 1 / painter.scale(),
                                    Color(255, 255, 255, 30));
    }
    if (hasVisibleFocus()) {
        const RectF ring = localRect().adjusted(-1, -1, 1, 1);
        painter.drawRoundedRect(ring, {radius + 1, 0, Corners::Circular}, Color{0, 0, 0, 0}, 2 / painter.scale(),
                                color(ColorId::ButtonFocusRing));
    }
}

void ToolButton::enabledChanged()
{
    updateColors();
    requestPaint();
}

bool ToolButton::keyPress(const KeyEvent& event)
{
    if (event.modifiers != NoModifier)
        return false;
    if (event.key == key::Space || event.key == key::Return || event.key == key::KeypadEnter) {
        if (onClicked)
            onClicked();
        return true;
    }
    return false;
}

void ToolButton::pointerEnter()
{
    m_hovered = true;
    requestPaint();
}

void ToolButton::pointerLeave()
{
    m_hovered = false;
    requestPaint();
}

void ToolButton::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_pressed = true;
    requestPaint();
}

void ToolButton::pointerRelease(const PointerEvent& event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    requestPaint();
    if (localRect().contains(event.position) && isEnabled() && onClicked)
        onClicked();
}

HeaderContent::HeaderContent()
    : m_icon(addChild<Icon>("image", 16))
    , m_title(addChild<Label>(tr("Image Viewer")))
    , m_file(addChild<Label>())
{
    m_icon->setColor(themeAccent(0.9));
    m_title->setWeight(600);
    m_file->setColor(themeText(0.55));
    m_file->setElide(Label::Elide::Middle);
    m_file->setVerticallyCentered(true);
    m_file->setVisible(false);
}

HeaderContent::~HeaderContent() = default;

void HeaderContent::setFileName(const std::string& name)
{
    if (name == m_fileName)
        return;
    m_fileName = name;
    m_file->setText(name);
    m_file->setVisible(!name.empty());
    requestLayout();
}

void HeaderContent::layout()
{
    const double height = geometry().height;
    const double iconSize = 16;
    m_icon->setGeometry({14, std::floor((height - iconSize) / 2), iconSize, iconSize});

    const SizeF title = m_title->sizeHint();
    const double titleX = 14 + iconSize + 9;
    m_title->setGeometry({titleX, std::floor((height - title.height) / 2), title.width, title.height});

    if (!m_fileName.empty()) {
        const double right = geometry().width - 12;
        const double maximum = std::max(0.0, std::min(360.0, right - (titleX + title.width + 24)));
        const double wanted = m_file->sizeHint().width;
        const double width = std::min(wanted, maximum);
        m_file->setGeometry({right - width, std::floor((height - title.height) / 2), width, title.height});
    }
}

ToolBar::ToolBar()
    : m_open(addChild<ToolButton>(tr("Open")))
    , m_previous(addChild<ToolButton>())
    , m_next(addChild<ToolButton>())
    , m_fit(addChild<ToolButton>(tr("Fit")))
    , m_actual(addChild<ToolButton>(tr("1:1")))
    , m_zoomOut(addChild<ToolButton>())
    , m_zoomIn(addChild<ToolButton>())
    , m_rotateLeft(addChild<ToolButton>())
    , m_rotateRight(addChild<ToolButton>())
    , m_zoom(addChild<Label>("—"))
{
    setSpacing(6);
    setMargins({14, 9, 14, 9});

    m_open->setGlyph("folder", 16);
    m_open->setToolTip(tr("Open an image (Ctrl+O)"));
    m_open->onClicked = [this] {
        if (onOpen)
            onOpen();
    };
    m_previous->setGlyph("chevron-left", 16);
    m_previous->setToolTip(tr("Previous image (Left)"));
    m_previous->onClicked = [this] {
        if (onPrevious)
            onPrevious();
    };
    m_next->setGlyph("chevron-right", 16);
    m_next->setToolTip(tr("Next image (Right)"));
    m_next->onClicked = [this] {
        if (onNext)
            onNext();
    };

    addStretch(1);

    m_fit->setGlyph("viewer-fit", 16);
    m_fit->setToolTip(tr("Fit the picture to the window (0)"));
    m_fit->onClicked = [this] {
        if (onFit)
            onFit();
    };
    m_actual->setToolTip(tr("Show the picture at its own size (1)"));
    m_actual->onClicked = [this] {
        if (onActualSize)
            onActualSize();
    };
    addChild<Separator>();

    m_zoomOut->setGlyph("viewer-zoom-out", 16);
    m_zoomOut->setToolTip(tr("Zoom out (-)"));
    m_zoomOut->onClicked = [this] {
        if (onZoomOut)
            onZoomOut();
    };
    m_zoom->setLayoutHints({.alignment = Alignment::Center, .preferredWidth = 52});
    m_zoom->setTextAlignment(Label::TextAlignment::Center);
    m_zoom->setVerticallyCentered(true);
    m_zoom->setColor(themeText(0.8));
    m_zoom->setWeight(500);
    m_zoomIn->setGlyph("viewer-zoom-in", 16);
    m_zoomIn->setToolTip(tr("Zoom in (+)"));
    m_zoomIn->onClicked = [this] {
        if (onZoomIn)
            onZoomIn();
    };

    addChild<Separator>();
    m_rotateLeft->setGlyph("viewer-rotate-left", 16);
    m_rotateLeft->setToolTip(tr("Rotate anticlockwise (Shift+R)"));
    m_rotateLeft->onClicked = [this] {
        if (onRotateLeft)
            onRotateLeft();
    };
    m_rotateRight->setGlyph("viewer-rotate-right", 16);
    m_rotateRight->setToolTip(tr("Rotate clockwise (R)"));
    m_rotateRight->onClicked = [this] {
        if (onRotateRight)
            onRotateRight();
    };

    setHasImage(false);
    setHasNeighbours(false);
    setMode(true, false);
}

ToolBar::~ToolBar() = default;

void ToolBar::setHasImage(bool hasImage)
{
    m_hasImage = hasImage;
    for (ToolButton* button : {m_fit, m_actual, m_zoomOut, m_zoomIn, m_rotateLeft, m_rotateRight})
        button->setEnabled(hasImage);
    if (!hasImage)
        setMode(false, false);
}

void ToolBar::setHasNeighbours(bool hasNeighbours)
{
    m_previous->setEnabled(hasNeighbours);
    m_next->setEnabled(hasNeighbours);
}

void ToolBar::setMode(bool fit, bool actual)
{
    m_fit->setChecked(fit && m_hasImage);
    m_actual->setChecked(actual && m_hasImage);
}

void ToolBar::setZoomText(const std::string& text)
{
    m_zoom->setText(text);
}

void ToolBar::paint(Painter& painter)
{
    paintBar(painter, localRect(), BarEdge::Bottom);
}

StatusBar::StatusBar()
    : m_file(addChild<Label>())
    , m_imageInfo(addChild<Label>())
    , m_zoom(addChild<Label>("—"))
    , m_index(addChild<Label>())
{
    const double small = std::max(10.0, Theme::current().fontSize() - 1);
    for (Label* label : {m_file, m_imageInfo, m_zoom, m_index}) {
        label->setFontSize(small);
        label->setVerticallyCentered(true);
        label->setTextAlignment(Label::TextAlignment::Left);
    }
    m_file->setWeight(500);
    m_file->setColor(themeText(0.85));
    m_file->setElide(Label::Elide::Middle);
    m_imageInfo->setColor(themeText(0.5));
    m_zoom->setColor(themeText(0.8));
    m_zoom->setWeight(500);
    m_zoom->setTextAlignment(Label::TextAlignment::Right);
    m_index->setColor(themeText(0.5));
    m_index->setTextAlignment(Label::TextAlignment::Right);
    m_imageInfo->setVisible(false);
    m_file->setVisible(false);
    m_index->setVisible(false);
}

StatusBar::~StatusBar() = default;

void StatusBar::setFile(const std::string& name)
{
    m_file->setText(name);
    m_file->setVisible(!name.empty());
    requestLayout();
}

void StatusBar::setImageInfo(const std::string& dimensions, const std::string& size)
{
    const std::string text = dimensions.empty() ? size : (size.empty() ? dimensions : dimensions + "  ·  " + size);
    m_imageInfo->setText(text);
    m_imageInfo->setVisible(!text.empty());
    requestLayout();
}

void StatusBar::setZoom(const std::string& text)
{
    m_zoom->setText(text);
    requestLayout();
}

void StatusBar::setIndex(const std::string& text)
{
    m_index->setText(text);
    m_index->setVisible(!text.empty());
    requestLayout();
}

void StatusBar::layout()
{
    const double height = geometry().height;
    const double left = 16;
    const double right = geometry().width - 16;
    const double gap = 12;
    const double vertical = std::floor((height - m_zoom->sizeHint().height) / 2);

    // The zoom and the place in the folder keep to the right.
    const SizeF zoom = m_zoom->sizeHint();
    const SizeF index = m_index->isVisible() ? m_index->sizeHint() : SizeF{};
    double rightEdge = right;
    if (m_index->isVisible()) {
        m_index->setGeometry({rightEdge - index.width, vertical, index.width, index.height});
        rightEdge -= index.width + gap;
    }
    m_zoom->setGeometry({rightEdge - zoom.width, vertical, zoom.width, zoom.height});
    rightEdge -= zoom.width + gap;

    // The name gives way first when the window is narrow, then the details.
    const SizeF info = m_imageInfo->isVisible() ? m_imageInfo->sizeHint() : SizeF{};
    const SizeF name = m_file->isVisible() ? m_file->sizeHint() : SizeF{};
    const double available = std::max(0.0, rightEdge - left);
    double nameWidth = 0;
    double infoWidth = 0;
    if (m_imageInfo->isVisible() && info.width + gap <= available * 0.6) {
        infoWidth = info.width;
        nameWidth = std::min(name.width, available - infoWidth - gap);
    } else {
        nameWidth = std::min(name.width, available);
        infoWidth = 0;
    }
    if (m_file->isVisible()) {
        const SizeF label = m_file->sizeHint();
        m_file->setGeometry({left, vertical, nameWidth, label.height});
    }
    if (m_imageInfo->isVisible()) {
        const SizeF label = m_imageInfo->sizeHint();
        m_imageInfo->setGeometry({left + nameWidth + gap, vertical, infoWidth, label.height});
    }
}

void StatusBar::paint(Painter& painter)
{
    paintBar(painter, localRect(), BarEdge::Top);
}
