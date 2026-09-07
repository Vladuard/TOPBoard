#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <fstream>
#include <commdlg.h>

// ---- НАСТРОЙКИ ----
COLORREF penColor = RGB(255, 0, 0);
int penSize = 3;
bool isErasing = false;
bool isDrawing = false;
bool isActive = false;
std::vector<POINT> points;

// ---- КНОПКИ ----
struct Button {
    RECT rect;
    std::string label;
    bool isHovered;
    void (*action)(HWND);
};

std::vector<Button> buttons;
bool showButtons = false;

// ---- ПРОТОТИПЫ ----
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SaveDrawing(HWND hWnd);
void AddButton(const std::string& label, int x, int y, int w, int h, void (*action)(HWND));
void DrawButtons(HWND hWnd, HDC hdc);
void HandleButtonClick(HWND hWnd, POINT pt);

// ---- ДЕЙСТВИЯ ДЛЯ КНОПОК ----
void Action_Pen(HWND hWnd) { isErasing = false; MessageBox(hWnd, "Карандаш", "Инструмент", MB_OK); }
void Action_Eraser(HWND hWnd) { isErasing = true; MessageBox(hWnd, "Ластик", "Инструмент", MB_OK); }
void Action_Clear(HWND hWnd) { points.clear(); InvalidateRect(hWnd, NULL, TRUE); }
void Action_Save(HWND hWnd) { SaveDrawing(hWnd); }
void Action_Color(HWND hWnd) { 
    if (penColor == RGB(255,0,0)) penColor = RGB(0,0,255);
    else if (penColor == RGB(0,0,255)) penColor = RGB(0,255,0);
    else penColor = RGB(255,0,0);
}
void Action_Size(HWND hWnd) {
    if (penSize == 1) penSize = 3;
    else if (penSize == 3) penSize = 5;
    else if (penSize == 5) penSize = 10;
    else penSize = 1;
}
void Action_Toggle(HWND hWnd) {
    isActive = !isActive;
    showButtons = isActive;
    if (isActive) {
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    } else {
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        points.clear();
        InvalidateRect(hWnd, NULL, TRUE);
    }
    InvalidateRect(hWnd, NULL, TRUE);
}
void Action_Close(HWND hWnd) { DestroyWindow(hWnd); }

// ---- ТОЧКА ВХОДА ----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ScreenMarkerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Ошибка регистрации", "Ошибка", MB_ICONERROR);
        return 1;
    }

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        "ScreenMarkerClass", "Screen Marker",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBox(NULL, "Не удалось создать окно", "Ошибка", MB_ICONERROR);
        return 1;
    }

    RegisterTouchWindow(hWnd, 0);
    SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

    // ---- СОЗДАЁМ КНОПКИ ----
    int bw = 80, bh = 50;
    int startX = 20, startY = 20;

    AddButton("Рисовать", startX, startY, bw, bh, Action_Toggle);
    AddButton("Карандаш", startX + bw + 10, startY, bw, bh, Action_Pen);
    AddButton("Ластик", startX + (bw+10)*2, startY, bw, bh, Action_Eraser);
    AddButton("Очистить", startX + (bw+10)*3, startY, bw, bh, Action_Clear);
    AddButton("Сохранить", startX + (bw+10)*4, startY, bw, bh, Action_Save);
    AddButton("Цвет", startX + (bw+10)*5, startY, bw, bh, Action_Color);
    AddButton("Размер", startX + (bw+10)*6, startY, bw, bh, Action_Size);
    AddButton("Закрыть", startX + (bw+10)*7, startY, bw, bh, Action_Close);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// ---- ДОБАВИТЬ КНОПКУ ----
void AddButton(const std::string& label, int x, int y, int w, int h, void (*action)(HWND)) {
    Button btn;
    btn.rect = {x, y, x + w, y + h};
    btn.label = label;
    btn.isHovered = false;
    btn.action = action;
    buttons.push_back(btn);
}

// ---- РИСОВАТЬ КНОПКИ ----
void DrawButtons(HWND hWnd, HDC hdc) {
    if (!showButtons) return;

    HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    for (auto& btn : buttons) {
        COLORREF bgColor = btn.isHovered ? RGB(180, 180, 255) : RGB(200, 200, 200);
        HBRUSH hBrush = CreateSolidBrush(bgColor);
        FillRect(hdc, &btn.rect, hBrush);
        DeleteObject(hBrush);

        Rectangle(hdc, btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawText(hdc, btn.label.c_str(), -1, (RECT*)&btn.rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

// ---- ОБРАБОТКА НАЖАТИЯ НА КНОПКУ ----
void HandleButtonClick(HWND hWnd, POINT pt) {
    for (auto& btn : buttons) {
        if (pt.x >= btn.rect.left && pt.x <= btn.rect.right &&
            pt.y >= btn.rect.top && pt.y <= btn.rect.bottom) {
            if (btn.action) {
                btn.action(hWnd);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return;
        }
    }
}

// ---- ОБРАБОТЧИК СООБЩЕНИЙ ----
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            if (!points.empty()) {
                COLORREF currentColor = isErasing ? RGB(255, 255, 255) : penColor;
                HPEN pen = CreatePen(PS_SOLID, isErasing ? 20 : penSize, currentColor);
                SelectObject(hdc, pen);

                for (size_t i = 1; i < points.size(); i++) {
                    MoveToEx(hdc, points[i-1].x, points[i-1].y, NULL);
                    LineTo(hdc, points[i].x, points[i].y);
                }
                DeleteObject(pen);
            }

            DrawButtons(hWnd, hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_TOUCH: {
            UINT cInputs = LOWORD(wParam);
            PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];

            if (GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT))) {
                for (UINT i = 0; i < cInputs; i++) {
                    TOUCHINPUT ti = pInputs[i];
                    POINT pt;
                    pt.x = ti.x / 100;
                    pt.y = ti.y / 100;
                    ScreenToClient(hWnd, &pt);

                    if (ti.dwFlags & TOUCHEVENTF_DOWN) {
                        if (showButtons) {
                            HandleButtonClick(hWnd, pt);
                        }
                        if (isActive && !showButtons) {
                            isDrawing = true;
                            points.push_back(pt);
                        }
                    } else if (ti.dwFlags & TOUCHEVENTF_MOVE) {
                        if (isActive && isDrawing && !showButtons) {
                            points.push_back(pt);
                            InvalidateRect(hWnd, NULL, FALSE);
                        }
                    } else if (ti.dwFlags & TOUCHEVENTF_UP) {
                        isDrawing = false;
                    }
                }
            }
            delete[] pInputs;
            CloseTouchInputHandle((HTOUCHINPUT)lParam);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            if (showButtons) {
                HandleButtonClick(hWnd, pt);
                return 0;
            }

            if (isActive) {
                isDrawing = true;
                points.push_back(pt);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            bool needRedraw = false;
            for (auto& btn : buttons) {
                bool hover = (pt.x >= btn.rect.left && pt.x <= btn.rect.right &&
                              pt.y >= btn.rect.top && pt.y <= btn.rect.bottom);
                if (btn.isHovered != hover) {
                    btn.isHovered = hover;
                    needRedraw = true;
                }
            }
            if (needRedraw) InvalidateRect(hWnd, NULL, TRUE);

            if (isActive && isDrawing && !showButtons) {
                points.push_back(pt);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            isDrawing = false;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---- СОХРАНЕНИЕ ----
void SaveDrawing(HWND hWnd) {
    OPENFILENAME ofn = {};
    char filename[MAX_PATH] = "drawing.bmp";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Bitmap Files\0*.bmp\0";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        RECT rect;
        GetClientRect(hWnd, &rect);
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;

        HDC hdc = GetDC(hWnd);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, w, h);
        SelectObject(memDC, hBitmap);
        BitBlt(memDC, 0, 0, w, h, hdc, 0, 0, SRCCOPY);

        BITMAP bmp;
        GetObject(hBitmap, sizeof(BITMAP), &bmp);

        BITMAPINFOHEADER bi = {};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmp.bmWidth;
        bi.biHeight = bmp.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;

        std::ofstream file(filename, std::ios::binary);
        if (file) {
            BITMAPFILEHEADER bf = {};
            bf.bfType = 0x4D42;
            bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidth * bmp.bmHeight * 3;
            bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

            file.write((char*)&bf, sizeof(bf));
            file.write((char*)&bi, sizeof(bi));

            int size = bmp.bmWidth * bmp.bmHeight * 3;
            char* pixels = new char[size];
            GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
            file.write(pixels, size);
            delete[] pixels;
            file.close();
            MessageBox(hWnd, "Сохранено!", "Успех", MB_OK);
        }

        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(hWnd, hdc);
    }
}