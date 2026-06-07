#pragma once
#include <array>
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "../viewModels/SearchViewModel.h"

class MaterialColorScheme;
class IFontRepository;
class SearchBottomSheetView;

#define SEARCH_QUERY_MAX_LENGTH     20

/// @brief On-screen keyboard grid used by the search bottom sheet.
///        A single focusable view with an internal cursor; key caps are
///        child labels and the cursor highlight is a solid obj block.
class SearchKeyboardView : public ViewContainer
{
    SHARED_ONLY(SearchKeyboardView)

public:
    static constexpr int kCharRows = 4;
    static constexpr int kCharCols = 10;
    static constexpr int kControlRow = kCharRows;
    static constexpr int kControlCols = 3;

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override;

    /// @brief Moves the internal key cursor (called by the sheet on dpad input).
    void MoveCursor(FocusMoveDirection direction);

private:
    SearchBottomSheetView* _sheet;
    const MaterialColorScheme* _materialColorScheme;

    std::array<SharedPtr<Label2DView>, kCharRows * kCharCols> _keyLabels;
    std::array<SharedPtr<Label2DView>, kControlCols> _controlLabels;

    int _cursorRow = 1; // start on 'A'
    int _cursorCol = 0;
    bool _penDown = false;
    int _penRow = -1;
    int _penCol = -1;
    u32 _cursorBlockVramOffset = 0;
    bool _cursorBlockLoaded = false;

    SearchKeyboardView(SearchBottomSheetView* sheet,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void ActivateKey(int row, int col);
    bool HitTest(const Point& point, int& row, int& col) const;
    void GetKeyRect(int row, int col, int& x, int& y, int& width) const;
};

/// @brief Bottom sheet with an on-screen keyboard for searching the current folder.
class SearchBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(SearchBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_keyboard);
    }

    // called by the keyboard view
    void AppendChar(char c);
    void Backspace();
    void Commit();

protected:
    void Close() override;

private:
    SearchViewModel* _viewModel;
    const MaterialColorScheme* _materialColorScheme;

    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _queryLabel;
    SharedPtr<SearchKeyboardView> _keyboard;

    char _query[SEARCH_QUERY_MAX_LENGTH + 1] = {0};
    u32 _queryLength = 0;

    SearchBottomSheetView(SearchViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void UpdateQueryLabel();
};
