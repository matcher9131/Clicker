// Clicker.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include <chrono>
#include <thread>
#include <windowsx.h>
#include "framework.h"
#include "Clicker.h"
#include <cstdlib>

#define MAX_LOADSTRING 100
#define MOUSE_LOCATION_X(x) ((ULONG)(x) * 0xFFFF / GetSystemMetrics(SM_CXSCREEN))
#define MOUSE_LOCATION_Y(y) ((ULONG)(y) * 0xFFFF / GetSystemMetrics(SM_CYSCREEN))
constexpr int TIMER_INTERVAL = 100;

enum State {
    Idle,
    MouseDown,
    BeforeTargetMouseDown,
    TargetMouseLeftDown,
    TargetMouseRightDown,
    BeforeDstMouseDown,
    DstMouseDown,
    BeforeConfirmMouseDown,
    ConfirmMouseDown,
};

// ターゲットウィンドウがアクティブでないときはNULL
HWND targetWindow = NULL;
WCHAR activeWindowClass[MAX_LOADSTRING];
Settings settings;
bool isEnabled = true;
State state = State::Idle;
int remainMouseDownTime = TIMER_INTERVAL;
int remainIdleTime = 10000;
constexpr int MOUSE_COOLDOWN = 1000;
int remainMouseCooldown = MOUSE_COOLDOWN;
HBRUSH blueBrush;
RECT remainBarRect = { 5L, 20L, 105L, 25L };
bool isAdvancedChecked = false;
constexpr int offsetX = 512;
constexpr int offsetY = 32;
int targetIndex = -1;

// このコード モジュールに含まれる関数の宣言を転送します:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
bool SetTargetWindow();
void SetLabelText(HWND);
bool TryMouseDown(ULONG x, ULONG y, bool isRight = false);
bool TryMouseUp(bool isLeft = false);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 設定ファイルの読み込み
    settings = LoadSettings();
    remainIdleTime = settings.interval;

    // remainBarに使用するブラシの用意
    blueBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0x00, 0xFF));
    if (blueBrush == NULL) {
        return 1;
    }

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DialogProc);

    return 0;
}


LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            SetTimer(hWnd, 1, TIMER_INTERVAL, NULL);
            Button_SetCheck(GetDlgItem(hWnd, IDC_ENABLED_CHECK), BST_CHECKED);
            // labelの書き換え
            SetLabelText(hWnd);
            // remainBarRectをDLUからpixelに変換
            MapDialogRect(hWnd, &remainBarRect);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            // Escキー以外の理由でこのメッセージが飛んできた場合にのみEndDialogする
            if ((GetKeyState(VK_ESCAPE) & 0x8000) == 0) EndDialog(hWnd, IDOK);
            return TRUE;
        case IDC_ENABLED_CHECK:
            {
                int checked = Button_GetCheck(GetDlgItem(hWnd, IDC_ENABLED_CHECK));
                isEnabled = checked == BST_CHECKED;
                // labelの書き換え
                SetLabelText(hWnd);
            }
            break;
        case IDC_ADVANCED_CHECK:
            {
                int checked = Button_GetCheck(GetDlgItem(hWnd, IDC_ADVANCED_CHECK));
                isAdvancedChecked = checked == BST_CHECKED;
            }
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            SelectObject(hdc, blueBrush);
            Rectangle(
                hdc, 
                remainBarRect.left, 
                remainBarRect.top, 
                remainBarRect.left + (remainBarRect.right - remainBarRect.left) * remainIdleTime / settings.interval,
                remainBarRect.bottom
            );
            // 
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        DeleteObject(blueBrush);
        PostQuitMessage(0);
        break;
    case WM_TIMER:
        if (wParam == 1)
        {
            if (remainMouseDownTime > 0)
            {
                remainMouseDownTime -= TIMER_INTERVAL;
            }
            if (remainMouseCooldown > 0)
            {
                remainMouseCooldown -= TIMER_INTERVAL;
            }

            if (!isEnabled) break;

            SetTargetWindow();
            // labelの書き換え
            SetLabelText(hWnd);

            if (targetWindow != NULL)
            {
                switch (state)
                {
                case Idle:
                    {
                        remainIdleTime -= TIMER_INTERVAL;
                        // remainTimeがなくなったらmouseDownする
                        if (remainIdleTime <= 0)
                        {
                            if (isAdvancedChecked)
                            {
                                constexpr int x = offsetX + 32 * 3 + 16;
                                constexpr int y = offsetY + 32 * 5 + 16;
                                HDC hdc = GetDC(targetWindow);
                                COLORREF color = GetPixel(hdc, x, y);
                                ReleaseDC(targetWindow, hdc);
                                if (color != 0x0B0B2D)
                                {
                                    targetIndex = 23;
                                    state = State::BeforeTargetMouseDown;
                                    break;
                                }
                            }

                            // remainTimeをリセット
                            remainIdleTime = settings.interval;

                            if (TryMouseDown(320, 240))
                            {
                                state = State::MouseDown;
                            }
                        }
                    }
                    break;
                case MouseDown:
                    {
                        if (TryMouseUp())
                        {
                            state = State::Idle;
                        }
                    }
                    break;
                case BeforeTargetMouseDown:
                    {
                        if (remainMouseCooldown > 0) break;

                        if (targetIndex < 0)
                        {
                            state = State::Idle;
                            break;
                        }

                        HDC hdc = GetDC(targetWindow);
                        int left = offsetX + 32 * (targetIndex % 4);
                        int top = offsetY + 32 * (targetIndex / 4);
                        if (GetPixel(hdc, left + 14, top + 2) == 0x38B3FF && GetPixel(hdc, left + 16, top + 2) == 0x38B3FF)
                        {
                            if (TryMouseDown(left + 16, top + 16, true))
                            {
                                state = State::TargetMouseRightDown;
                                --targetIndex;
                            }
                        }
                        else if (GetPixel(hdc, left + 10, top + 10) == 0x001762 && GetPixel(hdc, left + 12, top + 10) == 0x001762)
                        {
                            if (TryMouseDown(left + 16, top + 16, true))
                            {
                                state = State::TargetMouseRightDown;
                                --targetIndex;
                            }
                        }
                        else
                        {
                            if (TryMouseDown(left + 16, top + 16))
                            {
                                state = State::TargetMouseLeftDown;
                                --targetIndex;
                            }
                        }
                        ReleaseDC(targetWindow, hdc);
                    }
                    break;
                case TargetMouseLeftDown:
                    {
                        if (TryMouseUp())
                        {
                            state = State::BeforeTargetMouseDown;
                        }
                    }
                    break;
                case TargetMouseRightDown:
                    {
                        if (TryMouseUp(true))
                        {
                            state = State::BeforeDstMouseDown;
                        }
                    }
                    break;
                case BeforeDstMouseDown:
                    {
                        if (remainMouseCooldown > 0) break;

                        constexpr int x = offsetX + 32 * 3 + 16;
                        constexpr int y = offsetY + 32 * 6 + 16;
                        if (TryMouseDown(x, y, true))
                        {
                            state = State::DstMouseDown;
                        }
                    }
                    break;
                case DstMouseDown:
                    {
                        if (TryMouseUp(true))
                        {
                            state = State::BeforeConfirmMouseDown;
                        }
                    }
                    break;
                case BeforeConfirmMouseDown:
                    {
                        if (remainMouseCooldown > 0) break;

                        if (TryMouseDown(256, 234))
                        {
                            state = State::ConfirmMouseDown;
                        }
                    }
                    break;
                case ConfirmMouseDown:
                    {
                        if (TryMouseUp())
                        {
                            state = State::BeforeTargetMouseDown;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            // 再描画
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return FALSE;
}

bool SetTargetWindow()
{
    HWND activeWindow = GetForegroundWindow();
    GetClassNameW(activeWindow, activeWindowClass, MAX_LOADSTRING);
    if (lstrcmpW(activeWindowClass, settings.className.c_str()) == 0)
    {
        targetWindow = activeWindow;
        return true;
    }
    else
    {
        targetWindow = NULL;
        return false;
    }
}

void SetLabelText(HWND hWnd)
{
    SetDlgItemTextW(hWnd, IDC_WORKING_LABEL, isEnabled ? targetWindow != NULL ? L"Working" : L"Pending" : L"Disabled");
}

// x, yはウィンドウ左上を原点とする相対座標
bool TryMouseDown(ULONG x, ULONG y, bool isRight)
{
    if (remainMouseCooldown > 0 || targetWindow == NULL) return false;

    RECT rect;
    GetWindowRect(targetWindow, &rect);
    INPUT input = {
        INPUT_MOUSE,
        MOUSE_LOCATION_X(rect.left + x),
        // GetSystemMetrics(SM_CYCAPTION)はタイトルバーの高さ
        MOUSE_LOCATION_Y(rect.top + GetSystemMetrics(SM_CYCAPTION) + y),
        0,
        MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | (isRight ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN)
    };
    SendInput(1, &input, sizeof(INPUT));
    remainMouseDownTime = TIMER_INTERVAL;
    return true;
}

bool TryMouseUp(bool isRight)
{
    if (remainMouseDownTime > 0) return false;

    INPUT input = {
        INPUT_MOUSE,
        0,
        0,
        0,
        isRight ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP
    };
    SendInput(1, &input, sizeof(INPUT));
    remainMouseCooldown = MOUSE_COOLDOWN;
    return true;
}
