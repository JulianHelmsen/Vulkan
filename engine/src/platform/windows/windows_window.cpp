#include "engine/core/window.h"
#include <Windows.h>
#include <stdio.h>

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
		on_window_close_requested(owner);
		break;
	case WM_DESTROY:
		break;
	case WM_SIZE:
		on_window_resize(owner, LOWORD(lParam), HIWORD(lParam));
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void on_window_close_requested(window* window) {
	window->m_close_requested = true;
}

void on_window_resize(window* window, int new_width, int new_height) {
	window->m_width = new_width;
	window->m_height = new_height;
	if(window->m_resize_handler)
		window->m_resize_handler(new_width, new_height);
}



LRESULT InitialWindowProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam) {

	if (Msg == WM_CREATE) {
		LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)EventWindowProc);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}


window::window(const char* title, int width, int height) : m_window_handle(0), m_close_requested(false), m_closed(false) {

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
	m_window_handle = (window_handle_t)window_handle;
	ShowWindow(window_handle, SW_SHOW);
}

bool window::is_minimized() const {
	return IsIconic((HWND) m_window_handle);
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