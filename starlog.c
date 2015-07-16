#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

typedef enum {
	HOTKEY_LOG,
	HOTKEY_QUIT
} HOTKEY_ID;

typedef struct {
	
} Starlog;

static void register_hotkey(int id, int mod, int vk)
{
	if(!RegisterHotKey(NULL, id, mod, vk)) {
		MessageBox(NULL, "could not register hotkey", "error",
			MB_ICONEXCLAMATION);
		ExitProcess(1);
	}
}

static void starlog_init(Starlog *s)
{
	register_hotkey(HOTKEY_LOG,
		MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT, 'L');
}

static void starlog_deinit(Starlog *s)
{
	
}

static void starlog_toggle_window_visibility(Starlog *s)
{
	
}

void __main(void) asm("__main");
void __main(void)
{
	Starlog s = {0};
	MSG msg;
	starlog_init(&s);
	while(GetMessage(&msg, NULL, 0, 0)) {
		if(msg.message != WM_HOTKEY) {
			continue;
		}
		if(msg.wParam == HOTKEY_QUIT) {
			break;
		}
		if(msg.wParam == HOTKEY_LOG) {
			starlog_toggle_window_visibility(&s);
		}
	}
	starlog_deinit(&s);
	ExitProcess(0);
}
