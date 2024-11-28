#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

typedef unsigned long long u64;
typedef signed long long i64;

static void ToggleBorderless(HWND hwnd)
{
	static i64 style_mask = WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
	static i64 exstyle_mask = WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;
	static struct undo {
		HWND hwnd;
		i64 style;
		i64 exstyle;
	} undo;
	BOOL revert = hwnd == undo.hwnd;
	if (undo.hwnd) {
		SendMessageW(undo.hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		SetWindowLongPtrW(undo.hwnd, GWL_STYLE, undo.style);
		SetWindowLongPtrW(undo.hwnd, GWL_EXSTYLE, undo.exstyle);
		//UpdateWindow(undo.hwnd);
		undo = (struct undo){0};
	}
	if (hwnd && !revert) {
		undo.hwnd = hwnd;
		undo.style = GetWindowLongPtrW(hwnd, GWL_STYLE);
		undo.exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
		SetWindowLongPtrW(undo.hwnd, GWL_STYLE, undo.style & ~style_mask);
		SetWindowLongPtrW(undo.hwnd, GWL_STYLE, undo.exstyle & ~exstyle_mask);
		SendMessageW(undo.hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		UpdateWindow(undo.hwnd);
	}
}
static i64 KeyboardProc(int code, u64 wparam, i64 lparam)
{
	if (code < 0) { // "If code is less than zero, the hook procedure must pass the message to..."
		return CallNextHookEx(NULL, code, wparam, lparam);
	}
	KBDLLHOOKSTRUCT *kbd = lparam;
	if (kbd->vkCode == VK_F11 && // key code is F11
		!(kbd->flags & LLKHF_UP) && // key was pressed, not released
		!!(kbd->flags & LLKHF_ALTDOWN) && // alt is pressed
		!!!(GetAsyncKeyState(kbd->vkCode) & 0x8000)) { // F11 key down is not a key repeat
			ToggleBorderless(GetForegroundWindow());
			return 1; // consume keyboard input
	}
	return CallNextHookEx(NULL, code, wparam, lparam);
}
int wWinMain(HINSTANCE instance, HINSTANCE previnstance, WCHAR *args, int show)
{
	static const WCHAR *class = L"borderlessAppClass";
	HANDLE mutex = CreateMutexW(NULL, 0, class);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (MessageBoxW(NULL, L"Quit borderless?", L"borderless", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES) {
			PostMessageW(FindWindowExW(HWND_MESSAGE, NULL, class, NULL), WM_DESTROY, NULL, NULL);
		}
		return 0;
	}
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof wcex;
	wcex.lpfnWndProc = DefWindowProcW;
	wcex.hInstance = instance;
	wcex.lpszClassName = class;
	HWND hwnd = CreateWindowExW(0, RegisterClassExW(&wcex), NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, instance, NULL);
	HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, NULL, 0);
	MessageBoxW(hwnd, L"Press Alt+F11 to toggle maximized borderless window.\nRe-launch borderless.exe to quit.", L"borderless", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	MSG msg = {0};
	while (GetMessageW(&msg, NULL, 0, 0) > 0) {
		DispatchMessageW(&msg);
		if (msg.message == WM_DESTROY) PostQuitMessage(0);
	}
	UnhookWindowsHookEx(hook);
	ToggleBorderless(0); // undo before quitting
	if (mutex) {
		ReleaseMutex(mutex);
	}
	return msg.wParam;
}
