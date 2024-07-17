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
constexpr int BAR_WIDTH = 200;
constexpr int ENABLED_CHECKBOX_ID = 101;

HWND activeWindow = NULL;
WCHAR activeWindowClass[MAX_LOADSTRING];
Settings settings;
HWND enabledCheckBox = NULL;
bool isEnabled = true;
bool isWorking = false;
bool isClicking = false;
int remainTime = 10000;
HBRUSH blueBrush;

// このコード モジュールに含まれる関数の宣言を転送します:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
void SetLabelText(HWND);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 設定ファイルの読み込み
    settings = LoadSettings();
    remainTime = settings.interval;
    //

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
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ENABLED_CHECK:
            {
                int checked = Button_GetCheck(GetDlgItem(hWnd, IDC_ENABLED_CHECK));
                isEnabled = checked == BST_CHECKED;
                // labelの書き換え
                SetLabelText(hWnd);
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
            Rectangle(hdc, 10, 40, 10 + BAR_WIDTH * remainTime / settings.interval, 50);
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
                // クリック動作中でなければremainTimeを減らす
                if (!isClicking)
                {
                    remainTime -= TIMER_INTERVAL;
                }
                
                // remainTimeがなくなったらクリック動作をする
                if (remainTime <= 0 && !isClicking)
                {
                    isClicking = true;

                    // remainTimeをリセット
                    remainTime = settings.interval;

                    RECT rect;
                    GetWindowRect(activeWindow, &rect);
                    long x = (rect.left + rect.right) / 2;
                    long y = (rect.top + rect.bottom) / 2;
                    INPUT input1 = {
                        INPUT_MOUSE, 
                        MOUSE_LOCATION_X(x), 
                        MOUSE_LOCATION_Y(y),
                        0,
                        MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN 
                    };
                    SendInput(1, &input1, sizeof(INPUT));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    INPUT input2 = {
                        INPUT_MOUSE,
                        0,
                        0,
                        0,
                        MOUSEEVENTF_LEFTUP
                    };
                    SendInput(1, &input2, sizeof(INPUT));

                    isClicking = false;
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