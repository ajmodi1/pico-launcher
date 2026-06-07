#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "SearchBottomSheetView.h"

// Keyboard geometry. Key cells are 16x16 on a 20px vertical pitch.
// Char grid columns span x = 48..208; the control row sits below it.
#define KEY_GRID_X          48
#define KEY_CELL_WIDTH      16
#define KEY_ROW_PITCH       20
#define CONTROL_DEL_X       48
#define CONTROL_SPACE_X     112
#define CONTROL_OK_X        176
#define CONTROL_END_X       208

#define TITLE_LABEL_X       20
#define QUERY_LABEL_X       84
#define HEADER_Y_OFFSET     14
#define KEYBOARD_Y_OFFSET   34

static const char16_t* sKeyRows[SearchKeyboardView::kCharRows] =
{
    u"1234567890",
    u"ABCDEFGHIJ",
    u"KLMNOPQRST",
    u"UVWXYZ-.()"
};

SearchKeyboardView::SearchKeyboardView(SearchBottomSheetView* sheet,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _sheet(sheet), _materialColorScheme(materialColorScheme)
{
    const auto font = fontRepository->GetFont(FontType::Regular10);
    for (int r = 0; r < kCharRows; r++)
    {
        for (int c = 0; c < kCharCols; c++)
        {
            auto& label = _keyLabels[r * kCharCols + c];
            label = Label2DView::CreateShared(32, 16, 1, font);
            char16_t text[2] = { sKeyRows[r][c], 0 };
            label->SetText(text);
            label->SetHorizontalAlignment(Alignment::Center);
            AddChildTail(label.GetPointer());
        }
    }
    static const char16_t* sControlTexts[kControlCols] = { u"DEL", u"SPACE", u"OK" };
    for (int i = 0; i < kControlCols; i++)
    {
        _controlLabels[i] = Label2DView::CreateShared(64, 16, 6, font);
        _controlLabels[i]->SetText(sControlTexts[i]);
        _controlLabels[i]->SetHorizontalAlignment(Alignment::Center);
        AddChildTail(_controlLabels[i].GetPointer());
    }
}

void SearchKeyboardView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        // 16x16 4bpp solid block (all pixels color index 15) for the key cursor
        _cursorBlockVramOffset = objVramManager->Alloc(128);
        vu32* dst = (vu32*)objVramManager->GetVramAddress(_cursorBlockVramOffset);
        for (int i = 0; i < 32; i++)
        {
            dst[i] = 0xFFFFFFFF;
        }
        _cursorBlockLoaded = true;
    }
}

Rectangle SearchKeyboardView::GetBounds() const
{
    return Rectangle(KEY_GRID_X - 8, _position.y, CONTROL_END_X - KEY_GRID_X + 16,
        (kCharRows + 1) * KEY_ROW_PITCH);
}

void SearchKeyboardView::GetKeyRect(int row, int col, int& x, int& y, int& width) const
{
    y = _position.y + row * KEY_ROW_PITCH;
    if (row < kCharRows)
    {
        x = KEY_GRID_X + col * KEY_CELL_WIDTH;
        width = KEY_CELL_WIDTH;
    }
    else if (col == 0)
    {
        x = CONTROL_DEL_X;
        width = CONTROL_SPACE_X - CONTROL_DEL_X;
    }
    else if (col == 1)
    {
        x = CONTROL_SPACE_X;
        width = CONTROL_OK_X - CONTROL_SPACE_X;
    }
    else
    {
        x = CONTROL_OK_X;
        width = CONTROL_END_X - CONTROL_OK_X;
    }
}

void SearchKeyboardView::Update()
{
    for (int r = 0; r < kCharRows; r++)
    {
        for (int c = 0; c < kCharCols; c++)
        {
            // 32px wide label centered on a 16px cell
            _keyLabels[r * kCharCols + c]->SetPosition(
                KEY_GRID_X + c * KEY_CELL_WIDTH - 8, _position.y + r * KEY_ROW_PITCH);
        }
    }
    for (int i = 0; i < kControlCols; i++)
    {
        int x, y, width;
        GetKeyRect(kControlRow, i, x, y, width);
        // 64px wide label centered on the control key zone
        _controlLabels[i]->SetPosition(x + width / 2 - 32, y);
    }
    ViewContainer::Update();
}

void SearchKeyboardView::Draw(GraphicsContext& graphicsContext)
{
    // cursor highlight block, drawn first so the key labels render on top of it
    // (later oam allocations get lower indices and thus higher display priority)
    if (_cursorBlockLoaded)
    {
        int x, y, width;
        GetKeyRect(_cursorRow, _cursorCol, x, y, width);
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(
                _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow),
                _materialColorScheme->GetColor(md::sys::color::primary)),
            y, y + 16);
        int blockCount = width / 16;
        gfx_oam_entry_t* oams = graphicsContext.GetOamManager().AllocOams(blockCount);
        for (int i = 0; i < blockCount; i++)
        {
            OamBuilder::OamWithSize<16, 16>(x + i * 16, y, _cursorBlockVramOffset >> 7)
                .WithPalette16(paletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[i]);
        }
    }

    // key cap colors; the focused key is drawn inverted on the highlight block
    for (int r = 0; r <= kControlRow; r++)
    {
        int cols = r == kControlRow ? kControlCols : kCharCols;
        for (int c = 0; c < cols; c++)
        {
            auto& label = r == kControlRow ? _controlLabels[c] : _keyLabels[r * kCharCols + c];
            bool isCursor = r == _cursorRow && c == _cursorCol;
            label->SetBackgroundColor(isCursor
                ? _materialColorScheme->GetColor(md::sys::color::primary)
                : _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
            label->SetForegroundColor(isCursor
                ? _materialColorScheme->GetColor(md::sys::color::onPrimary)
                : _materialColorScheme->onSurface);
        }
    }
    ViewContainer::Draw(graphicsContext);
}

void SearchKeyboardView::MoveCursor(FocusMoveDirection direction)
{
    int cols = _cursorRow == kControlRow ? kControlCols : kCharCols;
    switch (direction)
    {
        case FocusMoveDirection::Left:
        {
            _cursorCol = (_cursorCol + cols - 1) % cols;
            break;
        }
        case FocusMoveDirection::Right:
        {
            _cursorCol = (_cursorCol + 1) % cols;
            break;
        }
        case FocusMoveDirection::Up:
        {
            if (_cursorRow > 0)
            {
                if (_cursorRow == kControlRow)
                {
                    // control key -> nearest char column
                    _cursorCol = _cursorCol == 0 ? 1 : (_cursorCol == 1 ? 5 : 8);
                }
                _cursorRow--;
            }
            break;
        }
        case FocusMoveDirection::Down:
        {
            if (_cursorRow < kControlRow)
            {
                if (_cursorRow == kControlRow - 1)
                {
                    // char column -> control key it sits above
                    _cursorCol = _cursorCol < 4 ? 0 : (_cursorCol < 8 ? 1 : 2);
                }
                _cursorRow++;
            }
            break;
        }
    }
}

void SearchKeyboardView::ActivateKey(int row, int col)
{
    if (row < kCharRows)
    {
        _sheet->AppendChar((char)sKeyRows[row][col]);
    }
    else if (col == 0)
    {
        _sheet->Backspace();
    }
    else if (col == 1)
    {
        _sheet->AppendChar(' ');
    }
    else
    {
        _sheet->Commit();
    }
}

bool SearchKeyboardView::HitTest(const Point& point, int& row, int& col) const
{
    int yRel = point.y - _position.y;
    if (yRel < 0 || yRel >= (kCharRows + 1) * KEY_ROW_PITCH)
    {
        return false;
    }
    row = yRel / KEY_ROW_PITCH;
    if (row < kCharRows)
    {
        if (point.x < KEY_GRID_X || point.x >= KEY_GRID_X + kCharCols * KEY_CELL_WIDTH)
        {
            return false;
        }
        col = (point.x - KEY_GRID_X) / KEY_CELL_WIDTH;
        return true;
    }
    if (point.x >= CONTROL_DEL_X && point.x < CONTROL_SPACE_X)
    {
        col = 0;
        return true;
    }
    if (point.x >= CONTROL_SPACE_X && point.x < CONTROL_OK_X)
    {
        col = 1;
        return true;
    }
    if (point.x >= CONTROL_OK_X && point.x < CONTROL_END_X)
    {
        col = 2;
        return true;
    }
    return false;
}

bool SearchKeyboardView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        ActivateKey(_cursorRow, _cursorCol);
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void SearchKeyboardView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    int row, col;
    if (HitTest(touchPoint, row, col))
    {
        _penDown = true;
        _penRow = row;
        _penCol = col;
        // move the cursor for immediate visual feedback
        _cursorRow = row;
        _cursorCol = col;
    }
}

void SearchKeyboardView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    int row, col;
    if (_penDown && HitTest(lastTouchPoint, row, col) && row == _penRow && col == _penCol)
    {
        ActivateKey(row, col);
    }
    _penDown = false;
}

SearchBottomSheetView::SearchBottomSheetView(SearchViewModel* viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _materialColorScheme(materialColorScheme)
    , _titleLabel(Label2DView::CreateShared(64, 16, 12, fontRepository->GetFont(FontType::Medium11)))
    , _queryLabel(Label2DView::CreateShared(160, 16, SEARCH_QUERY_MAX_LENGTH + 2,
        fontRepository->GetFont(FontType::Regular10)))
    , _keyboard(SearchKeyboardView::CreateShared(this, materialColorScheme, fontRepository))
{
    _titleLabel->SetText(u"Search");
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_queryLabel.GetPointer());
    AddChildTail(_keyboard.GetPointer());

    // prefill with the active query so it can be edited or cleared
    const char* initialQuery = _viewModel->GetInitialQuery();
    for (u32 i = 0; i < SEARCH_QUERY_MAX_LENGTH && initialQuery[i]; i++)
    {
        _query[_queryLength++] = initialQuery[i];
    }
    UpdateQueryLabel();
}

void SearchBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + HEADER_Y_OFFSET);
    _queryLabel->SetPosition(QUERY_LABEL_X, _position.y + HEADER_Y_OFFSET);
    _keyboard->SetPosition(0, _position.y + KEYBOARD_Y_OFFSET);
    BottomSheetView::Update();
}

void SearchBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _queryLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _queryLabel->SetForegroundColor(_materialColorScheme->onSurface);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool SearchBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    if (inputProvider.Triggered(InputKey::Start))
    {
        Commit();
        return true;
    }
    if (inputProvider.Triggered(InputKey::X))
    {
        Backspace();
        return true;
    }
    return false;
}

SharedPtr<View> SearchBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _keyboard.GetPointer())
    {
        // the keyboard is a single focusable view with an internal cursor
        _keyboard->MoveCursor(direction);
    }
    return nullptr;
}

void SearchBottomSheetView::AppendChar(char c)
{
    if (_queryLength < SEARCH_QUERY_MAX_LENGTH)
    {
        _query[_queryLength++] = c;
        UpdateQueryLabel();
    }
}

void SearchBottomSheetView::Backspace()
{
    if (_queryLength > 0)
    {
        _queryLength--;
        UpdateQueryLabel();
    }
}

void SearchBottomSheetView::Commit()
{
    _query[_queryLength] = 0;
    _viewModel->Commit(_query);
}

void SearchBottomSheetView::Close()
{
    _viewModel->Close();
}

void SearchBottomSheetView::UpdateQueryLabel()
{
    char16_t text[SEARCH_QUERY_MAX_LENGTH + 2];
    for (u32 i = 0; i < _queryLength; i++)
    {
        text[i] = (char16_t)(u8)_query[i];
    }
    text[_queryLength] = u'_';
    text[_queryLength + 1] = 0;
    _queryLabel->SetText(text);
}
