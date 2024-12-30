// Clicker.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include <chrono>
#include <thread>
#include <windowsx.h>
#include "framework.h"
#include "Clicker.h"

#define MAX_LOADSTRING 100
#define MOUSE_LOCATION_X(x) ((ULONG)x * 0xFFFF / GetSystemMetrics(SM_CXSCREEN))
#define MOUSE_LOCATION_Y(y) ((ULONG)y * 0xFFFF / GetSystemMetrics(SM_CYSCREEN))
constexpr int TIMER_INTERVAL = 100;

enum State {
    Idle,
    MouseDown,
    TargetMouseDown,
    DstMouseDown,
    ConfirmMouseDown,
};

HWND activeWindow = NULL;
WCHAR activeWindowClass[MAX_LOADSTRING];
Settings settings;
bool isEnabled = true;
bool isWorking = false;
State state = State::Idle;
int remainTime = 10000;
int advancedRemainTime = 1000;
HBRUSH blueBrush;
RECT remainBarRect = { 5L, 20L, 105L, 25L };
bool isAdvancedChecked = false;

// このコード モジュールに含まれる関数の宣言を転送します:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
void SetLabelText(HWND);
static void DoMouseDown(ULONG x, ULONG y, bool isLeft = true);
static void DoMouseUp(bool isLeft = true);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 設定ファイルの読み込み
    settings = LoadSettings();
    remainTime = settings.interval;

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
        //case IDC_TEST_BUTTON:
        //    {
        //        activeWindow = GetForegroundWindow();

        //    }
        //    break;
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
                remainBarRect.left + (remainBarRect.right - remainBarRect.left) * remainTime / settings.interval, 
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
            if (!isEnabled) break;

            // アクティブウィンドウを取得してクラス名がSettings.classNameと等しいかどうかを判断する
            activeWindow = GetForegroundWindow();
            GetClassNameW(activeWindow, activeWindowClass, MAX_LOADSTRING);
            isWorking = lstrcmpW(activeWindowClass, settings.className.c_str()) == 0;
            // labelの書き換え
            SetLabelText(hWnd);

            if (isWorking)
            {
                switch (state)
                {
                case Idle:
                    {
                        remainTime -= TIMER_INTERVAL;
                        // remainTimeがなくなったらmouseDownする
                        if (remainTime <= 0)
                        {
                            // TODO: Advanced

                            // remainTimeをリセット
                            remainTime = settings.interval;

                            RECT rect;
                            GetWindowRect(activeWindow, &rect);
                            long x = (rect.left + rect.right) / 2;
                            long y = (rect.top + rect.bottom) / 2;
                            DoMouseDown(x, y);
                            state = State::MouseDown;
                        }
                    }
                    break;
                case MouseDown:
                    {
                        DoMouseUp();
                        state = State::Idle;
                    }
                    break;
                case TargetMouseDown:
                    {
                        // NOT IMPLEMENTED
                    }
                    break;
                case DstMouseDown:
                    {
                        // NOT IMPLEMENTED
                    }
                    break;
                case ConfirmMouseDown:
                    {
                        // NOT IMPLEMENTED
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

void SetLabelText(HWND hWnd)
{
    SetDlgItemTextW(hWnd, IDC_WORKING_LABEL, isEnabled ? isWorking ? L"Working" : L"Pending" : L"Disabled");
}

static void DoMouseDown(ULONG x, ULONG y, bool isLeft)
{
    INPUT input = {
        INPUT_MOUSE,
        MOUSE_LOCATION_X(x),
        MOUSE_LOCATION_Y(y),
        0,
        MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | (isLeft ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN)
    };
    SendInput(1, &input, sizeof(INPUT));
}

static void DoMouseUp(bool isLeft)
{
    INPUT input = {
        INPUT_MOUSE,
        0,
        0,
        0,
        isLeft ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP
    };
    SendInput(1, &input, sizeof(INPUT));
}