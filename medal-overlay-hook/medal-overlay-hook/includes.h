#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>
#include <vector>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

using Present_t = LRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT );
using ResizeBuffers_t = HRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT );
typedef LRESULT( CALLBACK* WNDPROC )( HWND, UINT, WPARAM, LPARAM );