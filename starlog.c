#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#define WIN_WIDTH 600
#define WIN_HEIGHT 200

#define IDC_SAVE_CLOSE 102
#define IDC_CLOSE 103

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

typedef enum {
	HOTKEY_LOG,
	HOTKEY_QUIT
} HOTKEY_ID;

typedef struct {
	HWND hwnd;
	HWND edit_log;
	HWND bu_save_close;
	HWND bu_close;
} Starlog;

static void
register_hotkey(int id, int mod, int vk)
{
	if(!RegisterHotKey(NULL, id, mod, vk)) {
		MessageBox(NULL, "could not register hotkey", "error",
			MB_ICONEXCLAMATION|MB_OK);
		ExitProcess(1);
	}
}

static void
center_window(HWND window)
{
	int x, y, w, h;
	RECT rc;
	GetWindowRect(window, &rc);
	w = rc.right-rc.left;
	h = rc.bottom-rc.top;
	x = (GetSystemMetrics(SM_CXSCREEN)-w)/2;
	y = (GetSystemMetrics(SM_CYSCREEN)-h)/2;
	SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
}

static void
starlog_open_window(Starlog *s)
{
	if(IsWindowVisible(s->hwnd)) {
		return;
	}
	center_window(s->hwnd);
	ShowWindow(s->hwnd, SW_SHOW);
	SetForegroundWindow(s->hwnd);
	SetFocus(s->edit_log);
}

static void
starlog_log(Starlog *s)
{
	const size_t MAX_BYTES = 1024;
	unsigned short *log = NULL;
	DWORD textlen, loglen;
	SYSTEMTIME t;

	GetLocalTime(&t);
	textlen = GetWindowTextLengthW(s->edit_log);
	if(!textlen) {
		return;
	}
	log = HeapAlloc(GetProcessHeap(), 0, sizeof(char)*MAX_BYTES);
	loglen = wsprintfW(log, L"~~~ %02d.%02d.%04d %02d:%02d:%02d ~~~\r\n",
		t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond);
	loglen += GetWindowTextW(s->edit_log, &log[loglen],
		sizeof(short)*(MAX_BYTES-loglen));
	log[loglen] = '\r';
	log[loglen+1] = '\n';
	log[loglen+2] = 0;
	if(loglen) {
		HANDLE fd = CreateFile(".\\star.log",
			FILE_APPEND_DATA, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(fd==INVALID_HANDLE_VALUE) {
			return;
		}
		DWORD n = lstrlenW(log);
		WriteFile(fd, log, n*sizeof(short), &n, 0);
		CloseHandle(fd);
	}
	HeapFree(GetProcessHeap(), 0, log);
}

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Starlog *s = (Starlog *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(msg) {
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_SAVE_CLOSE:
			starlog_log(s);
		case IDC_CLOSE:
			SetWindowText(s->edit_log, "");
			ShowWindow(hwnd, SW_HIDE);
			break;
		}
		break;
	case WM_CHAR:
		if(wParam =='A' && (GetKeyState(VK_CONTROL) & 0x8000)!=0) {
			SendMessage(s->edit_log, EM_SETSEL, 0, -1);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static void
starlog_init(Starlog *s, HINSTANCE hInstance)
{
	const char className[] = "starlog";
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = sizeof(Starlog *);
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!", "error",
			MB_ICONEXCLAMATION|MB_OK);
		ExitProcess(1);
	}
	s->hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		className, "starlog",
		WS_OVERLAPPED|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH, WIN_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if(!s->hwnd) {
		MessageBox(NULL, "Window Creation Failed!", "error",
			MB_ICONEXCLAMATION|MB_OK);
		ExitProcess(1);
	}

	s->edit_log = CreateWindowEx(0,
		"Edit", "",
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_MULTILINE|
		ES_AUTOVSCROLL|ES_AUTOHSCROLL|WS_EX_CONTROLPARENT|WS_BORDER,
		10, 10, WIN_WIDTH-40, WIN_HEIGHT-90,
		s->hwnd, NULL,
		hInstance, NULL);

	s->bu_save_close = CreateWindowEx(0,
		"Button", "SAVE && CLOSE",
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
		10, WIN_HEIGHT-75, 120, 24,
		s->hwnd, (HMENU)IDC_SAVE_CLOSE,
		hInstance, NULL);

	s->bu_close = CreateWindowEx(0,
		"Button", "CLOSE",
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
		140, WIN_HEIGHT-75, 120, 24,
		s->hwnd, (HMENU)IDC_CLOSE,
		hInstance, NULL);

	SetWindowLongPtr(s->hwnd, GWLP_USERDATA, (LONG_PTR)s);

	register_hotkey(HOTKEY_LOG,
		MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT, 'L');
}

static void
starlog_deinit(Starlog *s)
{

}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	Starlog s = {0};
	MSG msg;
	starlog_init(&s, hInstance);
	while(GetMessage(&msg, NULL, 0, 0)) {
		if(msg.message == WM_HOTKEY) {
			if(msg.wParam == HOTKEY_QUIT) {
				break;
			}
			if(msg.wParam == HOTKEY_LOG) {
				starlog_open_window(&s);
			}
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	starlog_deinit(&s);
	ExitProcess(0);
}
