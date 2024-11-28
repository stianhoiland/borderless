#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define false 0
#define true 1
#define null NULL // WHY IS EVERYTHING SCREAMING

typedef wchar_t             u16; // "Microsoft implements wchar_t as a two-byte unsigned value."
typedef unsigned long long  u64;
typedef signed long long    i64;
typedef signed int         bool;
typedef void *           handle;

static void ToggleBorderless(handle hwnd)
{
	static i64 style_mask = WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
	static i64 exstyle_mask = WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;
	static struct undo {
		handle hwnd;
		i64 style;
		i64 exstyle;
	} undo;
	bool revert = hwnd == undo.hwnd;
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
		return CallNextHookEx(null, code, wparam, lparam);
	}
	KBDLLHOOKSTRUCT *kbd = lparam;
	if (kbd->vkCode == VK_F11 && // key code is F11
		!(kbd->flags & LLKHF_UP) && // key was pressed, not released
		!!(kbd->flags & LLKHF_ALTDOWN) && // alt is pressed
		!!!(GetAsyncKeyState(kbd->vkCode) & 0x8000)) { // F11 key down is not a key repeat
			ToggleBorderless(GetForegroundWindow());
			return 1; // consume keyboard input
	}
	return CallNextHookEx(null, code, wparam, lparam);
}
int wWinMain(void *instance, void *_, u16 *args, int show)
{
	static const u16 *class = L"borderlessAppClass";
	handle mutex = CreateMutexW(null, false, class);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (MessageBoxW(null, L"Quit borderless?", L"borderless", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES) {
			PostMessageW(FindWindowExW(HWND_MESSAGE, null, class, null), WM_DESTROY, null, null);
		}
		return 0;
	}
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof wcex;
	wcex.lpfnWndProc = DefWindowProcW;
	wcex.hInstance = instance;
	wcex.lpszClassName = class;
	handle hwnd = CreateWindowExW(0, RegisterClassExW(&wcex), null, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, null, instance, null);
	handle hook = SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, null, 0);
	MessageBoxW(hwnd, L"Press Alt+F11 to toggle maximized borderless window.\nRe-launch borderless.exe to quit.", L"borderless", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	MSG msg = {0};
	while (GetMessageW(&msg, null, 0, 0) > 0) {
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
