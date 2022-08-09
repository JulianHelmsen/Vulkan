#include "engine/core/window.h"
#include <Windows.h>

namespace utils {
	void convert_to_wstring(LPWSTR buffer, size_t elem_count, const char* source) {
		for (size_t i = 0; i < elem_count; i++) {
			if (source[i] == '\0')
				break;
			buffer[i] = source[i];
		}
	}
}
LRESULT EventWindowProc(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	window* owner = (window*) GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (msg) {
	case WM_CLOSE:
		owner->destroy();
		break;
	case WM_DESTROY:
		break;

	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT InitialWindowProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	
	if (Msg == WM_CREATE) {
		LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)EventWindowProc);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

window::window(const char* title, int width, int height) : m_window_handle(0), m_closed(false) {

	WNDCLASSEX cls = { 0 };
	cls.cbSize = sizeof(cls);
	cls.hInstance = GetModuleHandle(NULL);
	cls.lpszClassName = L"VK_ENGINE_WINDOW_CLASS";
	cls.lpfnWndProc = InitialWindowProc;
	RegisterClassEx(&cls);

	const void* user_ptr = this;
	WCHAR wtitle[100] = { 0 };
	utils::convert_to_wstring(wtitle, sizeof(wtitle) / sizeof(wtitle[0]), title);
	HWND window_handle = CreateWindowEx(WS_EX_ACCEPTFILES, cls.lpszClassName, wtitle, WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, cls.hInstance, (LPVOID)user_ptr);
	if (window_handle == NULL) {
		m_closed = true;
		return;
	}
	m_window_handle = (uint64_t)window_handle;
	ShowWindow(window_handle, SW_SHOW);
}

void window::poll_events() {
	MSG msg = {};
	HWND hwnd = (HWND)m_window_handle;
	if (PeekMessage(&msg, hwnd, 0, 0, TRUE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void window::wait_events() {
	MSG msg = {};
	HWND hwnd = (HWND) m_window_handle;
	if (GetMessage(&msg, hwnd, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


void window::destroy() {
	HWND hwnd = (HWND)m_window_handle;
	DestroyWindow(hwnd);
	m_window_handle = true;
	m_closed = true;
}