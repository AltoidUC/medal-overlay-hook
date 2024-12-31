#include "includes.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

Present_t oPresent = nullptr;
ResizeBuffers_t oResizeBuffers = nullptr;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void InitImGui( )
{
	ImGui::CreateContext( );

	ImGuiIO& io = ImGui::GetIO( );
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX11_Init( pDevice, pContext );
}

LRESULT __stdcall WndProc( const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( true && ImGui_ImplWin32_WndProcHandler( hWnd, uMsg, wParam, lParam ) )
		return true;

	return CallWindowProc( oWndProc, hWnd, uMsg, wParam, lParam );
}

std::uintptr_t __stdcall PresentHook( IDXGISwapChain* swapchain, UINT SyncInterval, UINT Flags )
{
	static bool Setup = false;
	if ( !Setup )
	{
		if ( SUCCEEDED( swapchain->GetDevice( __uuidof( ID3D11Device ), ( void** )&pDevice ) ) )
		{
			pDevice->GetImmediateContext( &pContext );
			DXGI_SWAP_CHAIN_DESC sd;
			swapchain->GetDesc( &sd );
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			swapchain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
			pDevice->CreateRenderTargetView( pBackBuffer, NULL, &mainRenderTargetView );
			pBackBuffer->Release( );
			oWndProc = ( WNDPROC )SetWindowLongPtr( window, GWLP_WNDPROC, ( LONG_PTR )WndProc );
			InitImGui( );
			Setup = true;
		}
		else
		{
			return oPresent( swapchain, SyncInterval, Flags );
		}
	}

	ImGui_ImplDX11_NewFrame( );
	ImGui_ImplWin32_NewFrame( );
	ImGui::NewFrame( );

	ImGui::Begin( "Window" );
	ImGui::BulletText( "Rendering with ImGui by hijacking Medal!" );
	ImGui::End( );

	ImGui::Render( );

	pContext->OMSetRenderTargets( 1, &mainRenderTargetView, NULL );
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
	return oPresent( swapchain, SyncInterval, Flags );
}

std::uintptr_t __stdcall ResizeBuffersHook( IDXGISwapChain* SwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags )
{
	if ( mainRenderTargetView )
	{
		pContext->OMSetRenderTargets( 0, 0, 0 );
		mainRenderTargetView->Release( );
	}

	ID3D11Texture2D* Buffer = nullptr;
	SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( void** )&Buffer );

	pDevice->CreateRenderTargetView( Buffer, NULL, &mainRenderTargetView );
	Buffer->Release( );

	pContext->OMSetRenderTargets( 1, &mainRenderTargetView, NULL );

	D3D11_VIEWPORT Vp{};
	Vp.Width = static_cast< float >( Width );
	Vp.Height = static_cast< float >( Height );
	Vp.MinDepth = 0.f;
	Vp.MaxDepth = 1.f;
	Vp.TopLeftX = 0;
	Vp.TopLeftY = 0;

	pContext->RSSetViewports( 1, &Vp );
	return oResizeBuffers( SwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags );
}

std::uintptr_t GetMedalModule( )
{
	std::uintptr_t Module = ( std::uintptr_t )GetModuleHandleA( "medal-hook32.dll" );
	if ( !Module )
	{
		Module = ( std::uintptr_t )GetModuleHandleA( "medal-hook64.dll" );
	}

	return Module;
}

std::uintptr_t GetMedalPresentPtr( )
{
#ifdef _WIN64
	return 0x125E10;
#else
	return 0xEC110;
#endif
}

std::uintptr_t GetMedalResizeBuffersPtr( )
{
#ifdef _WIN64
	return 0x125E08;
#else
	return 0xEC10C;
#endif
}

void InitHook( )
{
	FILE* f;
	AllocConsole( );
	freopen_s( &f, "CONOUT$", "w", stdout );

	std::uintptr_t MedalModule = GetMedalModule( );

	if ( !MedalModule )
		return;

	// Store originals.
	std::uintptr_t* MedalOriginalPresent = reinterpret_cast< std::uintptr_t* >( MedalModule + GetMedalPresentPtr( ) );
	std::uintptr_t* MedalOriginalResizeBuffers = reinterpret_cast< std::uintptr_t* >( MedalModule + GetMedalResizeBuffersPtr( ) );

	if ( !MedalOriginalPresent || !MedalOriginalResizeBuffers )
		return;

	// Log.
	printf( "[+] MedalModule -> 0x%p\n", reinterpret_cast< void* >( MedalModule ) );
	printf( "[+] MedalOriginalPresent -> 0x%p\n", MedalOriginalPresent );
	printf( "[+] MedalOriginalResizeBuffers -> 0x%p\n", MedalOriginalResizeBuffers );

	// Set original.
	oPresent = reinterpret_cast< Present_t >( *MedalOriginalPresent );
	oResizeBuffers = reinterpret_cast< ResizeBuffers_t >( *MedalOriginalResizeBuffers );

	// Hook.
	*reinterpret_cast< std::uintptr_t** >( MedalOriginalPresent ) = reinterpret_cast< std::uintptr_t* >( &PresentHook );
	*reinterpret_cast< std::uintptr_t** >( MedalOriginalResizeBuffers ) = reinterpret_cast< std::uintptr_t* >( &ResizeBuffersHook );

	// XREF for offsets: E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 89 2D
	// Present x86: 55 8B EC 8B 0D ? ? ? ? 64 A1 ? ? ? ? 57 8B 3C 88 80 BF ? ? ? ? ? 75 ? E8 ? ? ? ? 83 BF ? ? ? ? ? 7E ? FF 75 ? A1 ? ? ? ? FF 75 ? C7 87 ? ? ? ? ? ? ? ? FF 75 ? FF D0
	// Present: x64 48 89 6C 24 ? 57 41 54 41 57
}

BOOL WINAPI DllMain( HMODULE Mod, DWORD Reason, LPVOID Reserved )
{
	if ( Reason == DLL_PROCESS_ATTACH )
	{
		InitHook( );
	}

	return TRUE;
}