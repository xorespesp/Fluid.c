#include "vis.h"
#include "misc.h"
#include <Windows.h>
#include <shellscalingapi.h>
#pragma comment(lib, "Shcore.lib")

#define DEFAULT_GRID_PX_SIZE  8
#define DEFAULT_FONT_SIZE     14

struct _vis_obj_t {
    int32_t grid_cols, grid_rows;
    int32_t num_pixel_channels;
    pixel_t* frm_buff;
    size_t frm_buff_sz;

    int32_t scaled_grid_pixel_size;
    int32_t scaled_width_pixels, scaled_height_pixels;

    bool_t fl_should_close;
    bool_t fl_cursor_entered;
    bool_t fl_render_overlay;
    bool_t fl_dpi_awareness;

    DPI_AWARENESS_CONTEXT org_dpi_ctx;
    ATOM class_atom;
    HWND hwnd;
    HDC hmemdc;
    HBITMAP hmembmp;
    LPVOID pvbits;
    HFONT hfont;

    vis_key_fn_t cb_key;
    vis_mousebtn_fn_t cb_mousebtn;
    vis_cursorenter_fn_t cb_cursorenter;
    vis_cursorpos_fn_t cb_cursorpos;

    LPVOID user_ctx;
    LPCSTR overlay_buff;
    int32_t overlay_buff_cch;
};

static inline float _apply_dpi_scale(uint32_t dpi, float size_in_px)
{
    const float dpi_scale_factor = (float)(dpi) / 96.0f/* default dpi */;
    return size_in_px * dpi_scale_factor;
}

static inline void _vis_deinit_window(vis_obj_t self)
{
    assert(self);

    if (self->hfont) {
        DeleteObject(self->hfont);
        self->hfont = NULL;
    }

    if (self->hmembmp) {
        DeleteObject(self->hmembmp);
        self->hmembmp = NULL;
    }

    if (self->hmemdc) {
        DeleteDC(self->hmemdc);
        self->hmemdc = NULL;
    }
}

static inline bool_t _vis_init_window(vis_obj_t self, uint32_t dpi)
{
    printf("%s() -> dpi %u\n", __func__, dpi);
    assert(self);

    const HWND hwnd = self->hwnd;

    // Apply DPI
    {
        self->scaled_grid_pixel_size = (int32_t)_apply_dpi_scale(dpi, DEFAULT_GRID_PX_SIZE);
        self->scaled_width_pixels = self->grid_cols * self->scaled_grid_pixel_size;
        self->scaled_height_pixels = self->grid_rows * self->scaled_grid_pixel_size;

        RECT scaled_rc = { 0, 0, self->scaled_width_pixels, self->scaled_height_pixels };
        if (!AdjustWindowRectExForDpi(
            &scaled_rc,
            (DWORD)GetWindowLongA(hwnd, GWL_STYLE),
            FALSE,
            (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE),
            dpi))
        {
            return FALSE;
        }

        if (!SetWindowPos(hwnd, NULL,
            0, 0,
            scaled_rc.right - scaled_rc.left,
            scaled_rc.bottom - scaled_rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE))
        {
            return FALSE;
        }
    }

    // Create memory dc
    const HDC hdc = GetDC(hwnd);
    self->hmemdc = CreateCompatibleDC(hdc);
    if (!self->hmemdc) {
        _vis_deinit_window(self);
        ReleaseDC(hwnd, hdc);
        return FALSE;
    }

    // Create device-independent bitmap
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = self->scaled_width_pixels;
    bmi.bmiHeader.biHeight = -self->scaled_height_pixels; // Add negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = (WORD)(self->num_pixel_channels * 8);
    bmi.bmiHeader.biCompression = BI_RGB;
    self->hmembmp = CreateDIBSection(
        self->hmemdc, 
        &bmi, 
        DIB_RGB_COLORS, 
        &self->pvbits, 
        NULL, 
        0
    );
    if (!self->hmembmp || !self->pvbits) {
        _vis_deinit_window(self);
        ReleaseDC(hwnd, hdc);
        return FALSE;
    }

    self->hfont = CreateFontA(
        (int32_t)_apply_dpi_scale(dpi, DEFAULT_FONT_SIZE),
        0,
        0,
        0,
        FW_MEDIUM,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Consolas"
    );
    if (!self->hfont) {
        _vis_deinit_window(self);
        ReleaseDC(hwnd, hdc);
        return FALSE;
    }

    // Setup memory dc
    SelectObject(self->hmemdc, self->hmembmp);
    SelectObject(self->hmemdc, self->hfont);
    SetBkMode(self->hmemdc, TRANSPARENT);
    SetTextColor(self->hmemdc, RGB(255, 255, 255));

    ReleaseDC(hwnd, hdc);
    return TRUE;
}

static inline void _vis_update_window_frame(vis_obj_t self)
{
    assert(self);
    const int32_t grid_cols = self->grid_cols;
    const int32_t scaled_grid_pixel_size = self->scaled_grid_pixel_size;
    const int32_t scaled_width_pixels = self->scaled_width_pixels;
    const int32_t scaled_height_pixels = self->scaled_height_pixels;
    const int32_t num_pixel_channels = self->num_pixel_channels;
    const int32_t stride_bytes = scaled_width_pixels * num_pixel_channels;
    const HDC hmemdc = self->hmemdc;
    const pixel_t* const frm_buff = self->frm_buff;
    uint8_t* const pvbits = (uint8_t*)self->pvbits;

    for (int32_t y = 0; y < scaled_height_pixels; y += scaled_grid_pixel_size) {
        for (int32_t x = 0; x < scaled_width_pixels; x += scaled_grid_pixel_size) {
            const pixel_t src = frm_buff[(y / scaled_grid_pixel_size) * grid_cols + (x / scaled_grid_pixel_size)];
            for (int32_t yy = 0; yy < scaled_grid_pixel_size; ++yy) {
                for (int32_t xx = 0; xx < scaled_grid_pixel_size; ++xx) {
                    const int32_t pvbits_idx = (y + yy) * stride_bytes + (x + xx) * num_pixel_channels;
                    *(pixel_t*)(pvbits + pvbits_idx) = src; // byte order: BGRA
                }
            }
        }
    }

    if (self->fl_render_overlay) {
        if (self->overlay_buff) {
            const int32_t margin = 8;
            RECT rc = { margin, margin, scaled_width_pixels - margin, scaled_height_pixels - margin };
            DrawTextA(hmemdc, 
                self->overlay_buff, 
                self->overlay_buff_cch, 
                &rc, 
                DT_LEFT | DT_TOP | DT_WORDBREAK
            );
        }
    }
}

static vis_key_e _vis_translate_vkcode(DWORD vkcode)
{
    if (vkcode >= 'A' && vkcode <= 'Z') {
        return (vis_key_e)(VIS_KEY_A + (vkcode - 'A'));
    }

    if (vkcode >= '0' && vkcode <= '9') {
        return (vis_key_e)(VIS_KEY_0 + (vkcode - '0'));
    }

    if (vkcode >= VK_F1 && vkcode <= VK_F12) {
        return (vis_key_e)(VIS_KEY_F1 + (vkcode - VK_F1));
    }

    switch (vkcode) {
    case VK_ESCAPE: return VIS_KEY_ESCAPE;
    case VK_BACK: return VIS_KEY_BACK;
    case VK_RETURN: return VIS_KEY_RETURN;
    case VK_SPACE: return VIS_KEY_SPACE;
    case VK_LEFT: return VIS_KEY_LEFT;
    case VK_UP: return VIS_KEY_UP;
    case VK_RIGHT: return VIS_KEY_RIGHT;
    case VK_DOWN: return VIS_KEY_DOWN;
    case VK_MULTIPLY: return VIS_KEY_MULTIPLY;
    case VK_ADD:  return VIS_KEY_ADD;
    case VK_SUBTRACT: return VIS_KEY_SUBTRACT;
    case VK_DIVIDE: return VIS_KEY_DIVIDE;
    }

    return VIS_KEY_UNKNOWN;
}

static LRESULT WINAPI _vis_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const vis_obj_t self = (vis_obj_t)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    assert(self);

    switch (uMsg) {
    case WM_CREATE: {
        if (!_vis_init_window(self, GetDpiForWindow(hwnd))) {
            return -1; // NOTE: If returns –1, the window is destroyed and the `CreateWindowEx` returns a NULL handle.
        }
        return 0;
    }
    case WM_CLOSE: {
        self->fl_should_close = TRUE;
        return 0;
    }
    case WM_DESTROY: {
        _vis_deinit_window(self);
        PostQuitMessage(0);
        return 0;
    }
    case WM_DPICHANGED: {
        const uint32_t dpiX = LOWORD(wParam), dpiY = HIWORD(wParam);
        UNUSED_PARAM(dpiY);
        _vis_deinit_window(self);
        _vis_init_window(self, dpiX);
    }
    case WM_ERASEBKGND: { return 1; }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        _vis_update_window_frame(self);
        BitBlt(hdc,
            0, 0,
            self->scaled_width_pixels, self->scaled_height_pixels,
            self->hmemdc,
            0, 0,
            SRCCOPY
        );
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        const WORD vkcode = LOWORD(wParam); // vk code
        const WORD key_flags = HIWORD(lParam); // key flags
        const BOOL is_ext_key = (key_flags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix
        const BOOL is_key_released = (key_flags & KF_UP) == KF_UP; // transition-state flag, 1 on keyup
        const WORD scan_code = MAKEWORD(LOBYTE(key_flags), (is_ext_key) ? 0xE0 : 0x00); // scan code
        const BOOL was_key_down = (key_flags & KF_REPEAT) == KF_REPEAT; // previous key-state flag, 1 on autorepeat
        const WORD repeat_count = LOWORD(lParam); // repeat count, > 0 if several keydown messages was combined into one message

        vis_key_e key = _vis_translate_vkcode(vkcode);
        if (key != VIS_KEY_UNKNOWN) {
            vis_action_e action = is_key_released ? VIS_ACTION_RELEASE : (was_key_down && repeat_count ? VIS_ACTION_REPEAT : VIS_ACTION_PRESS);
            vis_modkey_e mods = 0;
            if (GetKeyState(VK_SHIFT) & 0x8000) { mods |= VIS_MODKEY_SHIFT; }
            if (GetKeyState(VK_CONTROL) & 0x8000) { mods |= VIS_MODKEY_CTRL; }
            if (GetKeyState(VK_MENU) & 0x8000) { mods |= VIS_MODKEY_ALT; }
            if (GetKeyState(VK_CAPITAL) & 0x0001) { mods |= VIS_MODKEY_CAPSLOCK; }
            if (GetKeyState(VK_NUMLOCK) & 0x0001) { mods |= VIS_MODKEY_NUMLOCK; }

            SAFE_DISPATCH(self->cb_key, self,
                key,
                (int)scan_code,
                action,
                mods
            );
        }

        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        vis_mousebtn_e btn = (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) ? VIS_MOUSEBTN_L : VIS_MOUSEBTN_R;
        vis_action_e action = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN) ? VIS_ACTION_PRESS : VIS_ACTION_RELEASE;
        vis_modkey_e mods = 0;
        if (wParam & MK_CONTROL) { mods |= VIS_MODKEY_CTRL; }
        if (wParam & MK_SHIFT) { mods |= VIS_MODKEY_SHIFT; }

        SAFE_DISPATCH(self->cb_mousebtn, self,
            btn,
            action,
            mods
        );

        return 0;
    }
    case WM_MOUSEMOVE: {
        const int32_t
            cursur_grid_xpos = ((int32_t)LOWORD(lParam)) / self->scaled_grid_pixel_size,
            cursur_grid_ypos = ((int32_t)HIWORD(lParam)) / self->scaled_grid_pixel_size;

        if (!self->fl_cursor_entered) {
            self->fl_cursor_entered = TRUE;
            TRACKMOUSEEVENT tme = { sizeof(tme) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            SAFE_DISPATCH(self->cb_cursorenter, self,
                self->fl_cursor_entered
            );
        }

        SAFE_DISPATCH(self->cb_cursorpos, self,
            cursur_grid_xpos,
            cursur_grid_ypos
        );

        return 0;
    }
    case WM_MOUSELEAVE: {
        self->fl_cursor_entered = FALSE;
        SAFE_DISPATCH(self->cb_cursorenter, self,
            self->fl_cursor_entered
        );

        return 0;
    }
    } // switch

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

static LRESULT WINAPI _vis_wnd_proc_init(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE) {
        assert(lParam);
        const vis_obj_t self = (vis_obj_t)(((CREATESTRUCT*)lParam)->lpCreateParams);
        assert(self);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)self); // Set thunk
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)_vis_wnd_proc);
        self->hwnd = hwnd; // NOTE: Since window creation was successful, properly set the `self->hwnd` field to be referenced in `WM_CREATE` handler.
    }

    // https://stackoverflow.com/q/42216797
    // NOTE: You should let `DefWindowProcW` function handle the `WM_NCCREATE` message. 
    // If you return `TRUE` as MSDN suggests, an untitled window will be created.
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

vis_obj_t vis_create(int32_t cols, int32_t rows, const char* title) {
    vis_obj_t newobj = (vis_obj_t)calloc(1, sizeof(struct _vis_obj_t));
    if (!newobj) {
        return NULL;
    }

    newobj->grid_cols = cols;
    newobj->grid_rows = rows;
    newobj->num_pixel_channels = sizeof(pixel_t);
    newobj->frm_buff_sz = newobj->grid_cols * newobj->grid_rows;
    newobj->frm_buff = (pixel_t*)calloc(newobj->frm_buff_sz, sizeof(pixel_t));
    if (!newobj->frm_buff) {
        vis_destroy(&newobj);
        return NULL;
    }

    newobj->org_dpi_ctx = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    newobj->fl_dpi_awareness = newobj->org_dpi_ctx != NULL;

    char class_name_buff[30];
    snprintf(class_name_buff, sizeof(class_name_buff),
        "VISWCLASS_%X_%X"
        , GetCurrentProcessId()
        , GetCurrentThreadId()
    );

    WNDCLASSA wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _vis_wnd_proc_init;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = class_name_buff;

    newobj->class_atom = RegisterClassA(&wc);
    if (!newobj->class_atom) {
        vis_destroy(&newobj);
        return NULL;
    }
    
    if (!CreateWindowExA(
        0,
        wc.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        wc.hInstance,
        newobj))
    {
        vis_destroy(&newobj);
        return NULL;
    }

    // NOTE: Since `CreateWindow` succeeded, `newobj->hwnd` field is valid.
    ShowWindow(newobj->hwnd, SW_SHOW);
    return newobj;
}

void vis_destroy(vis_obj_t* pself) {
    if (pself && *pself) {
        if ((*pself)->hwnd) {
            DestroyWindow((*pself)->hwnd);
        }

        if ((*pself)->class_atom) {
            UnregisterClassA((LPCSTR)((UINT_PTR)((*pself)->class_atom)), GetModuleHandleA(NULL));
        }

        if ((*pself)->org_dpi_ctx) {
            SetThreadDpiAwarenessContext((*pself)->org_dpi_ctx);
        }

        SAFE_FREE((*pself)->frm_buff);
        SAFE_FREE(*pself);
    }
}

void vis_close(vis_obj_t self) {
    assert(self);
    PostMessageA(self->hwnd, WM_CLOSE, 0, 0);
}

bool_t vis_poll(vis_obj_t self) {
    assert(self);
    MSG msg;
    while (PeekMessageA(&msg, self->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !self->fl_should_close;
}

bool_t vis_should_close(vis_obj_t self) {
    assert(self);
    return !!self->fl_should_close;
}

void vis_set_user_context(vis_obj_t self, void* user_ctx) {
    assert(self);
    self->user_ctx = user_ctx;
}

void* vis_get_user_context(vis_obj_t self) {
    assert(self);
    return self->user_ctx;
}

void vis_set_key_cb(vis_obj_t self, vis_key_fn_t cb) {
    assert(self);
    self->cb_key = cb;
}

void vis_set_mousebtn_cb(vis_obj_t self, vis_mousebtn_fn_t cb) {
    assert(self);
    self->cb_mousebtn = cb;
}

void vis_set_cursorenter_cb(vis_obj_t self, vis_cursorenter_fn_t cb) {
    assert(self);
    self->cb_cursorenter = cb;
}

void vis_set_cursorpos_cb(vis_obj_t self, vis_cursorpos_fn_t cb) {
    assert(self);
    self->cb_cursorpos = cb;
}

void vis_set_overlay_visibility(vis_obj_t self, bool_t visible) {
    assert(self);
    self->fl_render_overlay = visible;
}

void vis_set_overlay_text(vis_obj_t self, const char* str, int32_t cch) {
    assert(self);
    assert(cch >= 0 && !!str == !!cch);
    self->overlay_buff = str;
    self->overlay_buff_cch = cch;
}

int32_t vis_get_cols(vis_obj_t self) {
    assert(self);
    return self->grid_cols;
}

int32_t vis_get_rows(vis_obj_t self) {
    assert(self);
    return self->grid_rows;
}

void vis_draw(vis_obj_t self, int32_t col, int32_t row, pixel_t clr) {
    assert(self);
    const size_t idx = (row * self->grid_cols) + col;
    assert(0 <= idx && idx < self->frm_buff_sz);
    self->frm_buff[idx] = clr;
}

void vis_update(vis_obj_t self) {
    assert(self);
    InvalidateRgn(self->hwnd, NULL, FALSE);
}