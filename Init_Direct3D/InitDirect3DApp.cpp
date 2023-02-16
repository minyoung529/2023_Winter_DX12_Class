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

	LoadTextures();

	// 기하 도형 생성
	BuildBoxGeometry();
	BuildGridGeometry();
	BuildSphereGeometry();
	BuildCylinderGeometry();
	BuildSkullGeometry();

	// 재질 생성
	BuildMaterials();

	// 렌더링할 오브젝트 생성
	BuildRenderItems();

	// 렌더링 설정 수정
	BuildInputLayout();
	BuildShader();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildDescriptorHeaps();
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
	UpdateMaterialCB(gt);
	UpdatePassCB(gt);
}

void InitDirect3DApp::UpdateCamera(const GameTimer& gt)
{
	// 구면 좌표 => 직교 좌표

	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);

	// 시야 행렬 구축
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void InitDirect3DApp::UpdateObjectCBs(const GameTimer& gt)
{
	for (auto& e : mRenderItems)
	{
		XMMATRIX world = XMLoadFloat4x4(&e->world);
		XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

		ObjectConstants objectConstants;
		XMStoreFloat4x4(&objectConstants.world, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objectConstants.texTransform, XMMatrixTranspose(texTransform));

		UINT elementIdx = e->objCbIndex;
		UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;

		memcpy(&mObjectMappedData[elementIdx * elementByteSize], &objectConstants, sizeof(ObjectConstants));
	}
}

void InitDirect3DApp::UpdateMaterialCB(const GameTimer& gt)
{
	for (auto& e : mMateirals)
	{
		MaterialInfo* mat = e.second.get();
		MatConstants matConstants;

		matConstants.diffuseAlbedo = mat->diffuseAlbedo;
		matConstants.fresnelR0 = mat->fresnelR0;
		matConstants.roughness = mat->roughness;
		matConstants.texture_on = (mat->diffuseSrvHeapIndex == -1 ? 0 : 1);

		UINT elementIdx = mat->matCBIdx;
		UINT elementByteSize = (sizeof(MatConstants) + 255) & ~255;

		memcpy(&mMaterialMappedData[elementIdx * elementByteSize], &matConstants, sizeof(MatConstants));
	}
}

void InitDirect3DApp::UpdatePassCB(const GameTimer& gt)
{
	PassConstants passConstants;
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMStoreFloat4x4(&passConstants.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passConstants.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passConstants.invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&passConstants.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(viewProj));

	passConstants.ambientColor = { 0.25f, 0.25f, 0.35f, 1.0f };
	passConstants.eyePosW = mEyePos;
	passConstants.lightCount = 1;

	passConstants.lights[0].lightType = 0; // DIR
	passConstants.lights[0].direction = { 0.5773f, -0.57737f, 0.5773f };
	passConstants.lights[0].strength = { 0.6f, 0.6f, 0.6f };

	// --------
	for (int i = 0; i < 5; ++i)
	{
		passConstants.lights[i * 2 + 1].position = XMFLOAT3{ -5.f, 3.5f, -10.f + i * 5.0f };
		passConstants.lights[i * 2 + 1].lightType = 1;
		passConstants.lights[i * 2 + 1].strength = { 0.6f, 0.6f, 0.6f };
		passConstants.lights[i * 2 + 1].fallOffStart = 2;
		passConstants.lights[i * 2 + 1].fallOffEnd = 5;

		passConstants.lights[i * 2 + 2].position = XMFLOAT3{ 5.f, 3.5f, -10.f + i * 5.0f };
		passConstants.lights[i * 2 + 2].lightType = 1;
		passConstants.lights[i * 2 + 2].strength = { 0.6f, 0.6f, 0.6f };
		passConstants.lights[i * 2 + 1].fallOffStart = 2;
		passConstants.lights[i * 2 + 2].fallOffEnd = 5;
	}

	passConstants.lightCount = 11;

	memcpy(&mPassMappedData[0], &passConstants, sizeof(PassConstants));
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

	// 서술자 렌더링 파이프라인 묶기
	ID3D12DescriptorHeap* descrpitorHeap[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descrpitorHeap), descrpitorHeap);

	// 루트 시그니처, 상수 버퍼뷰 설정
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 공용 상수 버퍼 뷰 설정
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mItemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTest"].Get());
	DrawRenderItems(mItemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mItemLayer[(int)RenderLayer::Transparent]);
}

void InitDirect3DApp::DrawRenderItems(vector<RenderItem*>& renderItems)
{
	UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
	UINT matCBByteSize = (sizeof(MatConstants) + 255) & ~255;

	for (size_t i = 0; i < renderItems.size(); i++)
	{
		auto item = renderItems[i];

		if (item->geometry == nullptr)
			continue;

		//cbv
		D3D12_GPU_VIRTUAL_ADDRESS objCBAdress = mObjectCB->GetGPUVirtualAddress();
		objCBAdress += item->objCbIndex * objCBByteSize;	// 하나가 아니므로
		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAdress);

		D3D12_GPU_VIRTUAL_ADDRESS materialCBAdress = mMaterialCB->GetGPUVirtualAddress();
		materialCBAdress += item->material->matCBIdx * matCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, materialCBAdress);

		// 텍스쳐 버퍼 서술자 설정
		if (item->material->diffuseSrvHeapIndex != -1)
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(item->material->diffuseSrvHeapIndex, mCbvSrvDescriptorSize);

			mCommandList->SetGraphicsRootDescriptorTable(3, tex);
		}

		//vertex
		mCommandList->IASetVertexBuffers(0, 1, &item->geometry->vertexBufferView);
		//index
		mCommandList->IASetIndexBuffer(&item->geometry->indexBufferView);
		//topology
		mCommandList->IASetPrimitiveTopology(item->primitiveTopology);

		// Render
		mCommandList->DrawIndexedInstanced(item->geometry->indexCount, 1, 0, 0, 0);
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


void InitDirect3DApp::LoadTextures()
{
	{
		auto brickTex = make_unique<TextureInfo>();
		brickTex->name = "bricksTex";
		brickTex->fileName = L"../Textures/bricks.dds";
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12
		(
			md3dDevice.Get(),
			mCommandList.Get(),
			brickTex->fileName.c_str(),
			brickTex->resource,
			brickTex->uploadHeap
		));
		mTextures[brickTex->name] = move(brickTex);
	}
	{
		auto tileTex = make_unique<TextureInfo>();
		tileTex->name = "tileTex";
		tileTex->fileName = L"../Textures/tile.dds";
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12
		(
			md3dDevice.Get(),
			mCommandList.Get(),
			tileTex->fileName.c_str(),
			tileTex->resource,
			tileTex->uploadHeap
		));
		mTextures[tileTex->name] = move(tileTex);

	}
	{
		auto stoneTex = make_unique<TextureInfo>();
		stoneTex->name = "stoneTex";
		stoneTex->fileName = L"../Textures/stone.dds";
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12
		(
			md3dDevice.Get(),
			mCommandList.Get(),
			stoneTex->fileName.c_str(),
			stoneTex->resource,
			stoneTex->uploadHeap
		));
		mTextures[stoneTex->name] = move(stoneTex);
	}
	{
		auto fenceTex = make_unique<TextureInfo>();
		fenceTex->name = "fenceTex";
		fenceTex->fileName = L"../Textures/WireFence.dds";
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12
		(
			md3dDevice.Get(),
			mCommandList.Get(),
			fenceTex->fileName.c_str(),
			fenceTex->resource,
			fenceTex->uploadHeap
		));
		mTextures[fenceTex->name] = move(fenceTex);
	}
}


void InitDirect3DApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);

	//정점 정보
	std::vector<Vertex> vertices(box.Vertices.size());

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].pos = box.Vertices[i].Position;
		vertices[k].normal = box.Vertices[i].Normal;
		vertices[k].uv = box.Vertices[i].TexC;
	}

	//인덱스 정보
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));

	// 기하 데이터 입력
	auto geo = std::make_unique<GeometryInfo>();
	geo->name = "Box";

	//정점 버퍼 및 뷰
	geo->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->vertexBuffer));

	void* vertexDataBuff = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	geo->vertexBuffer->Map(0, &vertexRange, &vertexDataBuff);
	memcpy(vertexDataBuff, vertices.data(), vbByteSize);
	geo->vertexBuffer->Unmap(0, nullptr);

	geo->vertexBufferView.BufferLocation = geo->vertexBuffer->GetGPUVirtualAddress();
	geo->vertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->vertexBufferView.SizeInBytes = vbByteSize;

	//인덱스 버퍼 및 뷰
	geo->indexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->indexCount * sizeof(uint16_t);

	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->indexBuffer));

	void* indexDataBuff = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	geo->indexBuffer->Map(0, &indexRange, &indexDataBuff);
	memcpy(indexDataBuff, indices.data(), ibByteSize);
	geo->indexBuffer->Unmap(0, nullptr);

	geo->indexBufferView.BufferLocation = geo->indexBuffer->GetGPUVirtualAddress();
	geo->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	geo->indexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->name] = std::move(geo);
}


void InitDirect3DApp::BuildGridGeometry()
{
	GeometryGenerator generator;
	GeometryGenerator::MeshData grid = generator.CreateGrid(20.0f, 30.0f, 60, 40);

	//정점 정보
	std::vector<Vertex> vertices(grid.Vertices.size());

	UINT k = 0;
	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].pos = grid.Vertices[i].Position;
		vertices[k].uv = grid.Vertices[i].TexC;
		vertices[k].normal = grid.Vertices[i].Normal;
	}

	//인덱스 정보
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));

	// 기하 데이터 입력
	auto geo = std::make_unique<GeometryInfo>();
	geo->name = "Grid";

	//정점 버퍼 및 뷰
	geo->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->vertexBuffer));

	void* vertexDataBuff = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	geo->vertexBuffer->Map(0, &vertexRange, &vertexDataBuff);
	memcpy(vertexDataBuff, vertices.data(), vbByteSize);
	geo->vertexBuffer->Unmap(0, nullptr);

	geo->vertexBufferView.BufferLocation = geo->vertexBuffer->GetGPUVirtualAddress();
	geo->vertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->vertexBufferView.SizeInBytes = vbByteSize;

	//인덱스 버퍼 및 뷰
	geo->indexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->indexCount * sizeof(uint16_t);

	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->indexBuffer));

	void* indexDataBuff = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	geo->indexBuffer->Map(0, &indexRange, &indexDataBuff);
	memcpy(indexDataBuff, indices.data(), ibByteSize);
	geo->indexBuffer->Unmap(0, nullptr);

	geo->indexBufferView.BufferLocation = geo->indexBuffer->GetGPUVirtualAddress();
	geo->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	geo->indexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->name] = std::move(geo);
}
void InitDirect3DApp::BuildSphereGeometry()
{
	GeometryGenerator generator;
	GeometryGenerator::MeshData sphere = generator.CreateSphere(0.5f, 20, 20);

	//정점 정보
	std::vector<Vertex> vertices(sphere.Vertices.size());

	UINT k = 0;
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].pos = sphere.Vertices[i].Position;
		vertices[k].uv = sphere.Vertices[i].TexC;
		vertices[k].normal = sphere.Vertices[i].Normal;
	}

	//인덱스 정보
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

	// 기하 데이터 입력
	auto geo = std::make_unique<GeometryInfo>();
	geo->name = "Sphere";

	//정점 버퍼 및 뷰
	geo->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->vertexBuffer));

	void* vertexDataBuff = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	geo->vertexBuffer->Map(0, &vertexRange, &vertexDataBuff);
	memcpy(vertexDataBuff, vertices.data(), vbByteSize);
	geo->vertexBuffer->Unmap(0, nullptr);

	geo->vertexBufferView.BufferLocation = geo->vertexBuffer->GetGPUVirtualAddress();
	geo->vertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->vertexBufferView.SizeInBytes = vbByteSize;

	//인덱스 버퍼 및 뷰
	geo->indexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->indexCount * sizeof(uint16_t);

	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->indexBuffer));

	void* indexDataBuff = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	geo->indexBuffer->Map(0, &indexRange, &indexDataBuff);
	memcpy(indexDataBuff, indices.data(), ibByteSize);
	geo->indexBuffer->Unmap(0, nullptr);

	geo->indexBufferView.BufferLocation = geo->indexBuffer->GetGPUVirtualAddress();
	geo->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	geo->indexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->name] = std::move(geo);
}
void InitDirect3DApp::BuildCylinderGeometry()
{
	GeometryGenerator generator;
	GeometryGenerator::MeshData cylinder = generator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);


	//정점 정보
	std::vector<Vertex> vertices(cylinder.Vertices.size());

	UINT k = 0;
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].pos = cylinder.Vertices[i].Position;
		vertices[k].uv = cylinder.Vertices[i].TexC;
		vertices[k].normal = cylinder.Vertices[i].Normal;
	}

	//인덱스 정보
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	// 기하 데이터 입력
	auto geo = std::make_unique<GeometryInfo>();
	geo->name = "Cylinder";

	//정점 버퍼 및 뷰
	geo->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->vertexBuffer));

	void* vertexDataBuff = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	geo->vertexBuffer->Map(0, &vertexRange, &vertexDataBuff);
	memcpy(vertexDataBuff, vertices.data(), vbByteSize);
	geo->vertexBuffer->Unmap(0, nullptr);

	geo->vertexBufferView.BufferLocation = geo->vertexBuffer->GetGPUVirtualAddress();
	geo->vertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->vertexBufferView.SizeInBytes = vbByteSize;

	//인덱스 버퍼 및 뷰
	geo->indexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->indexCount * sizeof(uint16_t);

	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->indexBuffer));

	void* indexDataBuff = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	geo->indexBuffer->Map(0, &indexRange, &indexDataBuff);
	memcpy(indexDataBuff, indices.data(), ibByteSize);
	geo->indexBuffer->Unmap(0, nullptr);

	geo->indexBufferView.BufferLocation = geo->indexBuffer->GetGPUVirtualAddress();
	geo->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	geo->indexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->name] = std::move(geo);
}
void InitDirect3DApp::BuildSkullGeometry()
{
	ifstream fin("../Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"../Models/skull.txt Not Found.", 0, 0);
		return;
	}

	UINT vCnt = 0, tCnt = 0;
	string ignore;

	fin >> ignore >> vCnt;
	fin >> ignore >> tCnt;
	fin >> ignore >> ignore >> ignore >> ignore;

	vector<Vertex> vertices(vCnt);
	for (int i = 0; i < vCnt; i++)
	{
		fin >> vertices[i].pos.x >> vertices[i].pos.y >> vertices[i].pos.z;
		fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;
	}

	fin >> ignore >> ignore >> ignore;
	vector<uint32_t> indices(tCnt * 3);

	for (int i = 0; i < indices.size(); i++)
	{
		fin >> indices[i];
	}
	fin.close();

	// 기하 데이터 입력
	auto geo = std::make_unique<GeometryInfo>();
	geo->name = "Skull";

	//정점 버퍼 및 뷰
	geo->vertexCount = (UINT)vertices.size();
	const UINT vbByteSize = geo->vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->vertexBuffer));

	void* vertexDataBuff = nullptr;
	CD3DX12_RANGE vertexRange(0, 0);
	geo->vertexBuffer->Map(0, &vertexRange, &vertexDataBuff);
	memcpy(vertexDataBuff, vertices.data(), vbByteSize);
	geo->vertexBuffer->Unmap(0, nullptr);

	geo->vertexBufferView.BufferLocation = geo->vertexBuffer->GetGPUVirtualAddress();
	geo->vertexBufferView.StrideInBytes = sizeof(Vertex);
	geo->vertexBufferView.SizeInBytes = vbByteSize;

	//인덱스 버퍼 및 뷰
	geo->indexCount = (UINT)indices.size();
	const UINT ibByteSize = geo->indexCount * sizeof(uint32_t);

	heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

	md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&geo->indexBuffer));

	void* indexDataBuff = nullptr;
	CD3DX12_RANGE indexRange(0, 0);
	geo->indexBuffer->Map(0, &indexRange, &indexDataBuff);
	memcpy(indexDataBuff, indices.data(), ibByteSize);
	geo->indexBuffer->Unmap(0, nullptr);

	geo->indexBufferView.BufferLocation = geo->indexBuffer->GetGPUVirtualAddress();
	geo->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	geo->indexBufferView.SizeInBytes = ibByteSize;

	mGeometries[geo->name] = std::move(geo);
}
void InitDirect3DApp::BuildMaterials()
{
	{
		auto bricks0 = make_unique<MaterialInfo>();
		bricks0->name = "bricks0";
		bricks0->matCBIdx = bricks0->diffuseSrvHeapIndex = 0;
		bricks0->diffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
		bricks0->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		bricks0->roughness = 0.1f;
		mMateirals[bricks0->name] = move(bricks0);
	}
	{
		auto stone0 = make_unique<MaterialInfo>();
		stone0->name = "stone0";
		stone0->matCBIdx = 1;
		stone0->diffuseSrvHeapIndex = 2;
		stone0->diffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
		stone0->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		stone0->roughness = 0.3f;
		mMateirals[stone0->name] = move(stone0);
	}
	{
		auto tile0 = make_unique<MaterialInfo>();
		tile0->name = "tile0";
		tile0->matCBIdx = 2;
		tile0->diffuseSrvHeapIndex = 1;
		tile0->diffuseAlbedo = XMFLOAT4(Colors::LightGray);
		tile0->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		tile0->roughness = 0.2f;
		mMateirals[tile0->name] = move(tile0);
	}
	{
		auto skull = make_unique<MaterialInfo>();
		skull->name = "skull";
		skull->matCBIdx = 3;
		skull->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
		skull->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		skull->roughness = 0.3f;
		mMateirals[skull->name] = move(skull);
	}
	{
		auto fence = make_unique<MaterialInfo>();
		fence->name = "fence";
		fence->matCBIdx = 4;
		fence->diffuseSrvHeapIndex = 3;
		fence->diffuseAlbedo = XMFLOAT4(Colors::White);
		fence->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		fence->roughness = 0.2f;
		mMateirals[fence->name] = move(fence);
	}
}

void InitDirect3DApp::BuildRenderItems()
{
	{
		auto box = make_unique<RenderItem>();
		XMStoreFloat4x4(&box->world, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
		box->objCbIndex = 0;
		box->material = mMateirals["fence"].get();
		box->geometry = mGeometries["Box"].get();
		box->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		mItemLayer[(int)RenderLayer::AlphaTested].push_back(box.get());
		mRenderItems.push_back(move(box));
	}

	{
		auto grid = make_unique<RenderItem>();
		grid->world = MathHelper::Identity4x4();
		grid->objCbIndex = 1;
		grid->geometry = mGeometries["Grid"].get();
		grid->material = mMateirals["tile0"].get();
		XMStoreFloat4x4(&grid->texTransform, XMMatrixScaling(8.f, 8.f, 1.f));
		grid->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		mItemLayer[(int)RenderLayer::Opaque].push_back(grid.get());
		mRenderItems.push_back(move(grid));
	}

	{
		auto skull = make_unique<RenderItem>();
		XMStoreFloat4x4(&skull->world, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.f, 1.0f, 0.f));
		skull->objCbIndex = 2;
		skull->material = mMateirals["skull"].get();
		skull->geometry = mGeometries["Skull"].get();
		skull->primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		mItemLayer[(int)RenderLayer::Transparent].push_back(skull.get());
		mRenderItems.push_back(move(skull));
	}

	UINT objCBIdx = 6;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylinder = make_unique<RenderItem>();
		auto rightCylinder = make_unique<RenderItem>();
		auto leftSphere = make_unique<RenderItem>();
		auto rightSphere = make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.f, 1.5f, -10.f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(5.f, 1.5f, -10.f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.f, 3.5f, -10.f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(5.f, 3.5f, -10.f + i * 5.0f);

		XMStoreFloat4x4(&leftCylinder->world, leftCylWorld);
		leftCylinder->objCbIndex = objCBIdx++;
		leftCylinder->geometry = mGeometries["Cylinder"].get();
		leftCylinder->material = mMateirals["bricks0"].get();
		mItemLayer[(int)RenderLayer::Opaque].push_back(leftCylinder.get());
		mRenderItems.push_back(move(leftCylinder));

		XMStoreFloat4x4(&rightCylinder->world, rightCylWorld);
		rightCylinder->objCbIndex = objCBIdx++;
		rightCylinder->geometry = mGeometries["Cylinder"].get();
		rightCylinder->material = mMateirals["bricks0"].get();
		mItemLayer[(int)RenderLayer::Opaque].push_back(rightCylinder.get());
		mRenderItems.push_back(move(rightCylinder));

		XMStoreFloat4x4(&leftSphere->world, leftSphereWorld);
		leftSphere->objCbIndex = objCBIdx++;
		leftSphere->geometry = mGeometries["Sphere"].get();
		leftSphere->material = mMateirals["stone0"].get();
		mItemLayer[(int)RenderLayer::Opaque].push_back(leftSphere.get());
		mRenderItems.push_back(move(leftSphere));

		XMStoreFloat4x4(&rightSphere->world, rightSphereWorld);
		rightSphere->objCbIndex = objCBIdx++;
		rightSphere->geometry = mGeometries["Sphere"].get();
		rightSphere->material = mMateirals["stone0"].get();
		mItemLayer[(int)RenderLayer::Opaque].push_back(rightSphere.get());
		mRenderItems.push_back(move(rightSphere));
	}
}

void InitDirect3DApp::BuildInputLayout()
{
	// 입력 조리기
	mInputLayout =
	{
		// Shader와 연결됨
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, // 0 ~ 11 (12)
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, // 12 ~ 23 (12)
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} // 24 ~ 31 (8)
	};
}

void InitDirect3DApp::BuildShader()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Color.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Color.hlsl", alphaTestDefines, "PS", "ps_5_0");
}

void InitDirect3DApp::BuildConstantBuffers()
{
	// 개별 오브젝트 상수 버퍼
	{
		UINT size = sizeof(ObjectConstants);
		mObjectByteSize = (size + 255) & ~255;
		mObjectByteSize *= mRenderItems.size();
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

	// 개별 오브젝트 머티리얼 상수 버퍼
	{
		UINT size = sizeof(MatConstants);
		mMaterialByteSize = (size + 255) & ~255;
		mMaterialByteSize *= mMateirals.size();
		// 올림을 해서 256의 배수 값으로 바꿔주는 코드~~

		D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mMaterialByteSize);

		md3dDevice->CreateCommittedResource
		(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mMaterialCB)
		);

		// 그냥 열어놓음
		mMaterialCB->Map(0, nullptr, reinterpret_cast<void**>(&mMaterialMappedData));
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
	CD3DX12_DESCRIPTOR_RANGE texTable[]
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0), // t0
	};

	CD3DX12_ROOT_PARAMETER param[4];
	param[0].InitAsConstantBufferView(0);	// 0번 -> b0 -> CBV (개별)
	param[1].InitAsConstantBufferView(1);	// 1번 -> b1 -> CBV (개별 머티리얼)
	param[2].InitAsConstantBufferView(2);	// 2번 -> b2 -> CBV (공용)
	param[3].InitAsDescriptorTable(_countof(texTable), texTable);	// 2번 -> b2 -> CBV (공용)

	D3D12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param, 1, &samplerDesc);
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;

	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	md3dDevice->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature));
}

void InitDirect3DApp::BuildDescriptorHeaps()
{
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// srv 힙
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto bricksTex = mTextures["bricksTex"]->resource;
	auto tileTex = mTextures["tileTex"]->resource;
	auto stoneTex = mTextures["stoneTex"]->resource;
	auto fenceTex = mTextures["fenceTex"]->resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);	// 다음 Descriptor

	srvDesc.Format = tileTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);	// 다음 Descriptor

	srvDesc.Format = stoneTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);	// 다음 Descriptor

	srvDesc.Format = fenceTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = fenceTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);
}

void InitDirect3DApp::BuildPSO()
{
	// 불투명 오브젝트 렌더링 파이프라인 상태
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // Default로 하면 기본값으로 세팅
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	// 불투명 오브젝트 렌더링 파이프라인 상태
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transDesc = psoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transBlendDesc;

	transBlendDesc.BlendEnable = true;
	transBlendDesc.LogicOpEnable = false;
	transBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;	// 내 원본을 투명하게 처리
	transBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;	// 렌더 타겟은 알파 X
	transBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transBlendDesc.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

	transDesc.BlendState.RenderTarget[0] = transBlendDesc;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transDesc, IID_PPV_ARGS(&mPSOs["transparent"])));


	// 알파 테스트 오브젝트 렌더링 파이프라인 상태
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaDesc = psoDesc;
	alphaDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()), mShaders["alphaTestedPS"]->GetBufferSize()
	};

	alphaDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaDesc, IID_PPV_ARGS(&mPSOs["alphaTest"])));

}
