#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../Common/d3dUtil.h"
#include "../Common/GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// 0
// using namespaces
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL; // (ComPtr)


class D3DApp
{
protected:
	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	static D3DApp* GetApp();

	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void OnResize();
	virtual void Update(const GameTimer& gt) = 0;
	virtual void DrawBegin(const GameTimer& gt) = 0;
	virtual void Draw(const GameTimer& gt) = 0;
	virtual void DrawEnd(const GameTimer& gt) = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	bool InitMainWindow();

	void CalculateFrameStats();

protected:
	bool InitDirect3D();		// Device �����
	void FlushCommandQueue();	// Fence
	void CreateCommandObject();	// Comamnd~ 
	void CreateSwapChain();
	void CreateRtvDescriptorHeaps();
	void CreateDsvDescriptorHeaps();
	void CreateViewPort();

public:
	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

protected:

	static D3DApp* mApp;

	// Window 
	HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND      mhMainWnd = nullptr; // main window handle
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	std::wstring mMainWndCaption = L"d3d App";

	int mClientWidth = 800;
	int mClientHeight = 600;

	// Used to keep track of the �delta-time?and game time (?.4).
	GameTimer mTimer;

protected:
	// Device
	ComPtr<IDXGIFactory4>	mdxgiFactory;     // ���丮�� ���� Device ����
	ComPtr<ID3D12Device>	md3dDevice;

	// Fence
	ComPtr<ID3D12Fence>		mFence;
	UINT64					mCurrentFence = 0;

	// Descriptor ũ�� ���
	UINT					mRtvDescriptorSize = 0;	// Render Target View
	UINT					mDsvDescriptorSize = 0; // Depth Stencil View
	UINT					mCbvSrvUavDescriptorSize = 0;	// Constant, 

	// 4X MSAA (Multi Sampling)
	// Feature Level�� �����Ǹ� ������ ����
	// ��� �ʿ�� ������ Ȯ���ϱ� ����~
	bool					m4xMsaaState = false;
	UINT					m4xMsaaQuality = 0;

	D3D_DRIVER_TYPE			md3DriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT				mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT				mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Command ~
	ComPtr<ID3D12CommandQueue>			mCommandQueue;
	ComPtr<ID3D12CommandAllocator>		mDirectCmdListAlloc; // cmdlist�� ����� ���� �ʿ���
	ComPtr<ID3D12GraphicsCommandList>	mCommandList;

	// Swap Chain
	ComPtr<IDXGISwapChain>	mSwapChain;
	static const int		SwapChainBufferCount = 2;	// back, front
	int						mCurBackBuffer = 0;
	ComPtr<ID3D12Resource>	mSwapChainBuffer[SwapChainBufferCount];

	ComPtr<ID3D12DescriptorHeap>		mRtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE			mRtvView[SwapChainBufferCount] = {};	// �ּ�?

	// Depth Stencil Buffer
	ComPtr<ID3D12Resource>				mDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>		mDsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE			mDsvView = {};

	// Viewport
	D3D12_VIEWPORT			mScreenViewport;
	D3D12_RECT				mScissorRect;
};