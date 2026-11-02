// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/color_mixer.h"
#include "ui/layout.h"
#include "ui/view.h"

#include <functional>
#include <string>

namespace Lingmo::ui {
class Icon;
class Label;
class Painter;
} // namespace Lingmo::ui

// A theme colour as the current theme gives it, resolved when it is painted,
// so a view set with it follows the desktop from light to dark.
Lingmo::ui::ColorRecipe themeText(double alpha = 1.0);
Lingmo::ui::ColorRecipe themeAccent(double alpha = 1.0);

// The plate of a bar, and the hairline along the edge that meets the content.
enum class BarEdge {
    None,
    Top,
    Bottom,
};
void paintBar(Lingmo::ui::Painter& painter, const Lingmo::ui::RectF& rect, BarEdge edge, bool accentPlate = false);

// A vertical hairline between groups of controls.
class Separator : public Lingmo::ui::View
{
public:
    Separator() = default;

    Lingmo::ui::SizeF sizeHint() const override { return {13, 22}; }

protected:
    void paint(Lingmo::ui::Painter& painter) override;
};

// A tool bar button: a glyph, a word, or both, on a rounded plate that shows
// itself only when hovered, pressed or checked, so the bar stays quiet. A
// primary one is filled with the accent colour, for the one action an empty
// window offers.
class ToolButton : public Lingmo::ui::View
{
public:
    explicit ToolButton(std::string text = {});
    ~ToolButton() override;

    void setText(std::string text);
    void setGlyph(std::string name, double iconSize = 16);
    void setPrimary(bool primary);
    // Drawn on the accent's tint, and its glyph or text in the accent.
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }

    Lingmo::ui::SizeF sizeHint() const override;
    bool acceptsPointer() const override { return true; }
    bool acceptsFocus() const override { return true; }

    std::function<void()> onClicked;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;
    void enabledChanged() override;
    bool keyPress(const Lingmo::ui::KeyEvent& event) override;
    void pointerEnter() override;
    void pointerLeave() override;
    void pointerPress(const Lingmo::ui::PointerEvent& event) override;
    void pointerRelease(const Lingmo::ui::PointerEvent& event) override;

private:
    void updateColors();

    std::string m_text;
    Lingmo::ui::Label* m_label = nullptr;
    Lingmo::ui::Icon* m_icon = nullptr;
    double m_iconSize = 16;
    double m_height = 32;
    bool m_primary = false;
    bool m_checked = false;
    bool m_hovered = false;
    bool m_pressed = false;
};

// What fills the window's header before the window buttons: the application's
// mark, its name, and the name of the picture open.
class HeaderContent : public Lingmo::ui::View
{
public:
    HeaderContent();
    ~HeaderContent() override;

    void setFileName(const std::string& name);
    const std::string& fileName() const { return m_fileName; }

    Lingmo::ui::SizeF sizeHint() const override { return {0, 40}; }

protected:
    void layout() override;

private:
    Lingmo::ui::Icon* m_icon;
    Lingmo::ui::Label* m_title;
    Lingmo::ui::Label* m_file;
    std::string m_fileName;
};

// The bar above the picture: open, step through the folder, and the view
// controls. It owns its buttons and reports what they were pressed for.
class ToolBar : public Lingmo::ui::Row
{
public:
    ToolBar();
    ~ToolBar() override;

    void setHasImage(bool hasImage);
    // The folder's other pictures: arrows only act when there are any.
    void setHasNeighbours(bool hasNeighbours);
    // Which of the two size modes is the checked one; both are clear while
    // the zoom is a step of its own.
    void setMode(bool fit, bool actual);
    void setZoomText(const std::string& text);

    std::function<void()> onOpen;
    std::function<void()> onPrevious;
    std::function<void()> onNext;
    std::function<void()> onFit;
    std::function<void()> onActualSize;
    std::function<void()> onZoomIn;
    std::function<void()> onZoomOut;
    std::function<void()> onRotateLeft;
    std::function<void()> onRotateRight;

protected:
    void paint(Lingmo::ui::Painter& painter) override;

private:
    ToolButton* m_open;
    ToolButton* m_previous;
    ToolButton* m_next;
    ToolButton* m_fit;
    ToolButton* m_actual;
    ToolButton* m_zoomOut;
    ToolButton* m_zoomIn;
    ToolButton* m_rotateLeft;
    ToolButton* m_rotateRight;
    Lingmo::ui::Label* m_zoom;
    bool m_hasImage = false;
};

// The line under the picture: its name, its size on disk and in pixels, the
// zoom, and where it is in the folder.
class StatusBar : public Lingmo::ui::View
{
public:
    StatusBar();
    ~StatusBar() override;

    void setFile(const std::string& name);
    void setImageInfo(const std::string& dimensions, const std::string& size);
    void setZoom(const std::string& text);
    void setIndex(const std::string& text);

    Lingmo::ui::SizeF sizeHint() const override { return {0, 30}; }

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    Lingmo::ui::Label* m_file;
    Lingmo::ui::Label* m_imageInfo;
    Lingmo::ui::Label* m_zoom;
    Lingmo::ui::Label* m_index;
};
