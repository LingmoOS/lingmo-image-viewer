// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/color_mixer.h"
#include "ui/file_drop_area.h"
#include "ui/geometry.h"
#include "ui/view.h"

#include <functional>
#include <string>
#include <vector>

namespace Lingmo::ui {
class BusyIndicator;
class Icon;
class ImageView;
class Label;
class Painter;
} // namespace Lingmo::ui

class ToolButton;

// The message in the middle of an empty picture area: a mark, a heading, a
// line of what can be done, and the one button that does it.
class EmptyState : public Lingmo::ui::View
{
public:
    EmptyState();
    ~EmptyState() override;

    void setTexts(std::string title, std::string detail, std::string button);

    std::function<void()> onButtonClicked;

    Lingmo::ui::SizeF sizeHint() const override { return {0, 260}; }

protected:
    void layout() override;

private:
    Lingmo::ui::Icon* m_icon;
    Lingmo::ui::Label* m_title;
    Lingmo::ui::Label* m_detail;
    ToolButton* m_button;
};

// "Loading picture": a turning mark and the words, centred.
class LoadingView : public Lingmo::ui::View
{
public:
    LoadingView();
    ~LoadingView() override;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    Lingmo::ui::BusyIndicator* m_indicator;
    Lingmo::ui::Label* m_label;
};

// The area the picture lives in: takes dropped files, draws the plate and the
// picture's shadow, and shows the empty, loading and error messages. The
// window decides how large the picture is drawn; the area only places it.
class ImageSurface : public Lingmo::ui::FileDropArea
{
public:
    enum class State {
        Empty,
        Loading,
        Ready,
        Error,
    };

    ImageSurface();
    ~ImageSurface() override;

    State state() const { return m_state; }
    void setState(State state);
    void setErrorText(const std::string& title, const std::string& detail);

    // The logical size to draw the picture at; empty hides it.
    void setImageDisplaySize(Lingmo::ui::SizeF size);
    Lingmo::ui::SizeF imageDisplaySize() const { return m_displaySize; }
    // Where the picture lies in this area, once laid out.
    Lingmo::ui::RectF imageRect() const { return m_imageRect; }
    // The room a picture fitted to the window gets, the margins taken off.
    Lingmo::ui::SizeF contentSize() const { return m_contentSize; }
    Lingmo::ui::ImageView* imageView() const { return m_image; }

    // A drop of `paths`, in the folder's order of opening.
    std::function<void(const std::vector<std::string>&)> onFilesDropped;
    // A press on the empty state's button.
    std::function<void()> onOpenRequested;
    // A double click on the picture: the window switches Fit and 1:1.
    std::function<void()> onToggleFit;
    // A wheel notch: +1 zooms in, -1 out.
    std::function<void(int)> onZoomStep;
    // The room the picture has changed, after a layout.
    std::function<void()> onContentChanged;

    bool acceptsPointer() const override { return true; }

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;
    bool scroll(const Lingmo::ui::ScrollEvent& event) override;
    void pointerPress(const Lingmo::ui::PointerEvent& event) override;

private:
    Lingmo::ui::ImageView* m_image;
    EmptyState* m_empty;
    LoadingView* m_loading;
    Lingmo::ui::Label* m_dropHint;
    State m_state = State::Empty;
    Lingmo::ui::SizeF m_displaySize;
    Lingmo::ui::RectF m_imageRect;
    Lingmo::ui::SizeF m_contentSize;
    Lingmo::ui::SizeF m_lastContentSize;
};
