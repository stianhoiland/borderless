#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <WINDOWS.H>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static VOID ToggleBorderless(HWND hwnd)
{
	static LONG_PTR style_mask = WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
	static LONG_PTR exstyle_mask = WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME;
	static struct undo {
		HWND hwnd;
		LONG_PTR style;
		LONG_PTR exstyle;
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
static LRESULT KeyboardProc(INT code, WPARAM wparam, LPARAM lparam)
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
			return 1; // consume F11 key down event
	}
	return CallNextHookEx(NULL, code, wparam, lparam);
}
static LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_CREATE) MessageBoxW(hwnd, L"Press Alt+F11 to toggle maximized borderless window.\nRe-launch borderless.exe to quit.", L"borderless", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	if (message == WM_CLOSE) ToggleBorderless(0);
	return DefWindowProcW(hwnd, message, wparam, lparam);
}
INT wWinMain(HINSTANCE instance, HINSTANCE previnstance, WCHAR *args, INT show)
{
	static CONST WCHAR *class = L"borderlessGamingUtilityClass";
	HANDLE mutex = CreateMutexW(NULL, 0, class);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (MessageBoxW(NULL, L"Quit borderless?", L"borderless", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES) {
			PostMessageW(FindWindowExW(HWND_MESSAGE, NULL, class, NULL), WM_CLOSE, 0, 0);
		}
		return 0;
	}
	HWND hwnd = CreateWindowExW(0, RegisterClassExW(&(WNDCLASSEXW){.cbSize = sizeof(WNDCLASSEXW), .lpfnWndProc = WindowProc, .hInstance = instance, .lpszClassName = class}), NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
	HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, NULL, 0);
	MSG msg = {0};
	while (GetMessageW(&msg, hwnd, 0, 0) > 0) DispatchMessageW(&msg);
	UnhookWindowsHookEx(hook);
	if (mutex) {
		ReleaseMutex(mutex);
	}
	return msg.wParam;
}
