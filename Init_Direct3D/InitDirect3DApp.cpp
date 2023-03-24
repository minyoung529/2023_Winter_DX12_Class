#include "InitDirect3DApp.h"
// 진입점

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// BuildInputLayout(); > 그냥 하면 문제가 생김 (순서가 꼬인다거나...)

	// 초기화 명령들을 준비하기 위해 명령 목록 재설정
	mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

	// 렌더링 파이프라인을 위한 초기화
	BuildCube();
	BuildInputLayout();
	BuildShader();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildPSO();

	// 초기화 명령 실행
	mCommandList->Close();
	//mCommandList->ExecuteIndirect
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// 초기화 완료까지 대기
	FlushCommandQueue();

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();

	// 창의 크기가 바뀌는 부분에서 투영 행렬 갱신
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.f);
	XMStoreFloat4x4(&mProj, proj);
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
	UpdateCamera(gt);
	UpdateObjectCBs(gt);
	UpdatePassCB(gt);
}

void InitDirect3DApp::UpdateCamera(const GameTimer& gt)
{
	// 구면 좌표 => 직교 좌표

	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * cosf(mPhi);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);

	// 시야 행렬 구축
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void InitDirect3DApp::UpdateObjectCBs(const GameTimer& gt)
{
	for (auto& e : mRenderItems)
	{
		// 현재 Render Object의 행렬 가지고 옴
		XMMATRIX world = XMLoadFloat4x4(&e->world);

		ObjectConstants objectConstants;
		XMStoreFloat4x4(&objectConstants.world, XMMatrixTranspose(world));

		UINT elementIdx = e->objCbIndex;
		UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;

		// 상수 버퍼에 복사
		memcpy(&mObjectMappedData[elementIdx * elementByteSize], &objectConstants, sizeof(ObjectConstants));
	}
}

void InitDirect3DApp::UpdatePassCB(const GameTimer& gt)
{
	PassConstants passConstants;
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	// 역행렬
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

	XMStoreFloat4x4(&passConstants.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passConstants.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passConstants.invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&passConstants.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(viewProj));

	memcpy(mPassMappedData, &passConstants, sizeof(PassConstants));
}

void InitDirect3DApp::DrawBegin(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Bisque, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	// 출력 병합 (마지막 단계)
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	mCommandList->SetPipelineState(mPSO.Get());
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 공용 상수 버퍼 뷰 설정
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	DrawRenderItems(gt);
}

void InitDirect3DApp::DrawRenderItems(const GameTimer& gt)
{
	UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;

	for (size_t i = 0; i < mRenderItems.size(); i++)
	{
		auto item = mRenderItems[i].get();

		//cbv
		D3D12_GPU_VIRTUAL_ADDRESS objCBAdress = mObjectCB->GetGPUVirtualAddress();
		objCBAdress += item->objCbIndex * objCBByteSize;	// 하나가 아니므로

		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAdress);

		//vertex
		mCommandList->IASetVertexBuffers(0, 1, &item->vertexBufferView);

		//index
		mCommandList->IASetIndexBuffer(&item->indexBufferView);

		//topology
		mCommandList->IASetPrimitiveTopology(item->primitiveTopology);

		// Render
		//mCommandList->DrawIndexedInstanced(item->indexCount, 1, 0, 0, 0);
		mCommandList->DrawInstanced(item->vertexCount, 1, 0, 0);
	}
}

void InitDirect3DApp::DrawEnd(const GameTimer& gt)
{
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	mCommandList->Close();

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	mSwapChain->Present(0, 0);
	mCurBackBuffer = (mCurBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

#pragma region  MOUSE
void InitDirect3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos = { x , y };
	SetCapture(mhMainWnd);
}

void InitDirect3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void InitDirect3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)	// LBUTTON HOLD 공전
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)	// 확대 축소
	{
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 150.0f);
	}

	mLastMousePos = { x, y };
}
#pragma endregion


void InitDirect3DApp::BuildGeometry()
{
	// 정점 정보
	std::array<Vertex, 5> vertices =
	{
		Vertex({XMFLOAT3(0.0f,	 1.0f,	0.0f), XMFLOAT4(Colors::Cyan)}),
		Vertex({XMFLOAT3(-1.0f, -1.0f,	1.0f), XMFLOAT4(Colors::Magenta)}),
		Vertex({XMFLOAT3(1.0f,	-1.0f,	1.0f), XMFLOAT4(Colors::Yellow)}),
		Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue)}),
		Vertex({XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
	};

	std::array<std::uint16_t, 18> indices =
	{
		0, 2, 1,
		0, 4, 2,
		0, 3, 4,
		0, 1, 3,
		1, 2, 3,
		2, 4, 3
	};

	auto coneItem = make_unique<RenderItem>();
	coneItem->objCbIndex = 0;
	XMStoreFloat4x4(&coneItem->world, XMMatrixTranslation(0.0f, 5.0f, 0.0f));
	coneItem->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


#pragma region  VERTEX BUFFER
	// 정점 버퍼 생성
	coneItem->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = coneItem->vertexCount * sizeof(Vertex);

	// upload buffer (CPU, GPU 모두 건드릴 수 있음)
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);	// 정점의 크기만큼 버퍼 만듦

	md3dDevice->CreateCommittedResource
	(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&coneItem->vertexBuffer)
	);


	// --- 데이터를 복사하는 방법 ---
	// 직접적으로 접근 X

	void* vertexDataBuffer = nullptr; // 빈 버퍼에다 복사
	CD3DX12_RANGE vertexRange(0, 0);
	coneItem->vertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);	// 연다?
	memcpy(vertexDataBuffer, &vertices, vbByteSize);					// 복사
	coneItem->vertexBuffer->Unmap(0, nullptr);							// 다시 닫기

	// View
	coneItem->vertexBufferView.BufferLocation = coneItem->vertexBuffer->GetGPUVirtualAddress();
	coneItem->vertexBufferView.StrideInBytes = sizeof(Vertex);
	coneItem->vertexBufferView.SizeInBytes = vbByteSize;
#pragma endregion

#pragma region  INDEX BUFFER
	// 인덱스 버퍼 생성
	coneItem->indexCount = (UINT)indices.size();
	const UINT ibByteSize = coneItem->indexCount * sizeof(std::uint16_t);

	// upload buffer (CPU, GPU 모두 건드릴 수 있음)
	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);	// 정점의 크기만큼 버퍼 만듦

	md3dDevice->CreateCommittedResource
	(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,	// 읽기만 가능
		nullptr,
		IID_PPV_ARGS(&coneItem->indexBuffer)
	);

	void* IndexDataBuffer = nullptr; // 빈 버퍼에다 복사
	CD3DX12_RANGE indexRange(0, 0);
	coneItem->indexBuffer->Map(0, &indexRange, &IndexDataBuffer); // 연다?
	memcpy(IndexDataBuffer, &indices, ibByteSize);		// 복사
	coneItem->indexBuffer->Unmap(0, nullptr);						// 다시 닫기

	coneItem->indexBufferView.BufferLocation = coneItem->indexBuffer->GetGPUVirtualAddress();
	coneItem->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	coneItem->indexBufferView.SizeInBytes = ibByteSize;
#pragma endregion

	mRenderItems.push_back(move(coneItem));
}

void InitDirect3DApp::BuildCube()
{
	// 정점 정보
	std::array<Vertex, 8> vertices =
	{
		Vertex({XMFLOAT3(-1.0f, 1.0f,	1.0f), XMFLOAT4(Colors::Magenta)}),
		Vertex({XMFLOAT3(1.0f,	1.0f,	1.0f), XMFLOAT4(Colors::Yellow)}),
		Vertex({XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(Colors::Blue)}),
		Vertex({XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT4(Colors::Red)}),

		Vertex({XMFLOAT3(-1.0f, -1.0f,	1.0f), XMFLOAT4(Colors::Magenta)}),
		Vertex({XMFLOAT3(1.0f,	-1.0f,	1.0f), XMFLOAT4(Colors::Yellow)}),
		Vertex({XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue)}),
		Vertex({XMFLOAT3(-1.0f,  -1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
	};

	std::array<std::uint16_t, 36> indices =
	{
		0,1,2,
		0,2,3,

		3,2,6,
		3,6,7,

		6,5,4,
		6,4,7,

		2,5,6,
		2,1,5,

		1,0,4,
		1,4,5,

		0,3,7,
		0,7,4
	};

	auto cube = make_unique<RenderItem>();
	cube->objCbIndex = 2;
	XMStoreFloat4x4(&cube->world, XMMatrixTranslation(5.0f, 5.0f, 0.0f));
	cube->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


#pragma region  VERTEX BUFFER
	// 정점 버퍼 생성
	cube->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = cube->vertexCount * sizeof(Vertex);

	// upload buffer (CPU, GPU 모두 건드릴 수 있음)
	md3dDevice->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vbByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,	// 읽기만 가능
		nullptr,
		IID_PPV_ARGS(&cube->vertexBuffer)
	);


	// --- 데이터를 복사하는 방법 ---
	// 직접적으로 접근 X
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	cube->vertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);	// 연다
	memcpy(vertexDataBuffer, &vertices, vbByteSize);					// 복사
	cube->vertexBuffer->Unmap(0, nullptr);							// 다시 닫기

	cube->vertexBufferView.BufferLocation = cube->vertexBuffer->GetGPUVirtualAddress();
	cube->vertexBufferView.StrideInBytes = sizeof(Vertex);
	cube->vertexBufferView.SizeInBytes = vbByteSize;
#pragma endregion

#pragma region  INDEX BUFFER
	// 인덱스 버퍼 생성
	cube->indexCount = (UINT)indices.size();
	const UINT ibByteSize = cube->indexCount * sizeof(std::uint16_t);

	// upload buffer (CPU, GPU 모두 건드릴 수 있음)
	md3dDevice->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ibByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,	// 읽기만 가능
		nullptr,
		IID_PPV_ARGS(&cube->indexBuffer)
	);

	void* IndexDataBuffer = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	cube->indexBuffer->Map(0, &indexRange, &IndexDataBuffer);		// 연다
	memcpy(IndexDataBuffer, &indices, ibByteSize);					// 복사
	cube->indexBuffer->Unmap(0, nullptr);							// 다시 닫기

	cube->indexBufferView.BufferLocation = cube->indexBuffer->GetGPUVirtualAddress();
	cube->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	cube->indexBufferView.SizeInBytes = ibByteSize;
#pragma endregion

	mRenderItems.push_back(move(cube));
}

void InitDirect3DApp::BuildLandGeometry()
{
	float width = 160.0f;
	float depth = 160.0f;
	UINT n = 50, m = 50;

	UINT vertexCount = n * m;
	UINT faceCount = (n - 1) * (m - 1) * 2;

	float halfWidth = width * 0.5f;
	float halfDepth = depth * 0.5f;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	vector<Vertex> vertices;
	vertices.resize(vertexCount);

	// 정점 정보 채우기
	for (int i = 0; i < m; i++)
	{
		// -N~N
		float z = halfDepth - i * dz;

		for (int j = 0; j < n; j++)
		{
			float x = -halfWidth + j * dx;
			float y = 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));

			vertices[i * n + j].pos = XMFLOAT3(x, y, z);

			if (vertices[i * n + j].pos.y < -10.f)
				vertices[i * n + j].color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
			else if (vertices[i * n + j].pos.y < 5.f)
				vertices[i * n + j].color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
			else if (vertices[i * n + j].pos.y < 12.f)
				vertices[i * n + j].color = XMFLOAT4(0.1f, 0.48f, 0.62f, 1.0f);
			else if (vertices[i * n + j].pos.y < 20.f)
				vertices[i * n + j].color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
			else
				vertices[i * n + j].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	// 인덱스 정보
	std::vector<std::uint16_t> indices;
	indices.resize(faceCount * 3);

	UINT k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (UINT j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}

	auto landItem = make_unique<RenderItem>();
	landItem->objCbIndex = 1;
	landItem->world = MathHelper::Identity4x4();
	landItem->primitiveTopology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점 버퍼
	{
		landItem->vertexCount = (UINT)vertices.size();
		const UINT vbByteSize = landItem->vertexCount * sizeof(Vertex);

		D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

		md3dDevice->CreateCommittedResource
		(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&landItem->vertexBuffer)
		);

		void* vertexDataBuffer = nullptr;
		CD3DX12_RANGE vertexRange(0, 0);
		landItem->vertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
		memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
		landItem->vertexBuffer->Unmap(0, nullptr);

		landItem->vertexBufferView.BufferLocation = landItem->vertexBuffer->GetGPUVirtualAddress();
		landItem->vertexBufferView.StrideInBytes = sizeof(Vertex);
		landItem->vertexBufferView.SizeInBytes = vbByteSize;
	}

	// 인덱스 버퍼
	{
		landItem->indexCount = (UINT)indices.size();
		const UINT ibByteSize = landItem->indexCount * sizeof(uint16_t);

		D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

		md3dDevice->CreateCommittedResource
		(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&landItem->indexBuffer)
		);

		void* indexDataBuffer = nullptr;
		CD3DX12_RANGE indexRange(0, 0);
		landItem->indexBuffer->Map(0, &indexRange, &indexDataBuffer);
		memcpy(indexDataBuffer, indices.data(), ibByteSize);
		landItem->indexBuffer->Unmap(0, nullptr);

		landItem->indexBufferView.BufferLocation = landItem->indexBuffer->GetGPUVirtualAddress();
		landItem->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		landItem->indexBufferView.SizeInBytes = ibByteSize;
	}

	mRenderItems.push_back(move(landItem));
}

void InitDirect3DApp::BuildInputLayout()
{
	mInputLayout =
	{
		// Shader와 연결됨
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, // 0 ~ 11 (12)
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} // 12 ~ 27 (16)
	};
}

void InitDirect3DApp::BuildShader()
{
	mvsByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "PS", "ps_5_0");
}

void InitDirect3DApp::BuildConstantBuffers()
{
	// 개별 오브젝트 상수 버퍼
	{
		UINT size = sizeof(ObjectConstants);
		mObjectByteSize = (size + 255) & ~255;
		// 올림을 해서 256의 배수 값으로 바꿔주는 코드~~

		D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mObjectByteSize);

		md3dDevice->CreateCommittedResource
		(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mObjectCB)
		);

		// 그냥 열어놓음
		mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mObjectMappedData));
	}

	// 공용 상수 버퍼
	{
		UINT size = sizeof(PassConstants);
		mPassByteSize = (size + 255) & ~255;
		// 올림을 해서 256의 배수 값으로 바꿔주는 코드~~

		D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mPassByteSize);

		md3dDevice->CreateCommittedResource
		(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mPassCB)
		);

		// 그냥 열어놓음
		mPassCB->Map(0, nullptr, reinterpret_cast<void**>(&mPassMappedData));
	}
}

void InitDirect3DApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER param[2];
	param[0].InitAsConstantBufferView(0);	// 0번 -> b0 -> CBV
	param[1].InitAsConstantBufferView(1);	// 1번 -> b1 -> CBV

	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param);
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;

	D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);

	md3dDevice->CreateRootSignature
	(
		0,
		blobSignature->GetBufferPointer(),
		blobSignature->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)
	);
}

void InitDirect3DApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // Default로 하면 기본값으로 세팅
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;

	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.DSVFormat = mDepthStencilFormat;

	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

	md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
}
