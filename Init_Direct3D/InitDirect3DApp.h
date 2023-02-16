#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common/MathHelper.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"

using namespace DirectX;
using namespace std;

#define MAXLIGHTS 16

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

// 정점 정보
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

// 개별 오브젝트 상수 (World)
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4(); // 단위 행렬
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4(); // 단위 행렬
};

// 개별 오브젝트의 재질 상수
struct MatConstants
{
	XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };

	XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
	float roughness = 0.25f;

	UINT texture_on = 0;
	XMFLOAT3 padding = { 0.f, 0.f, 0.f };
};

struct GeometryInfo
{
	string name;

	// 정점 버퍼 뷰
	ComPtr<ID3D12Resource>		vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};

	// 인덱스 버퍼 뷰
	ComPtr<ID3D12Resource>		indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW		indexBufferView = {};

	// 정점 개수
	int vertexCount = 0;

	// 인덱스 개수
	int indexCount = 0;
};

// Material 구조체
struct MaterialInfo
{
	string name;

	int matCBIdx = -1;
	int diffuseSrvHeapIndex = -1;

	XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };

	XMFLOAT3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
	float roughness = 0.25f;
};

// 오브젝트 구조체
struct RenderItem
{
	RenderItem() = default;

	UINT		objCbIndex = -1;
	XMFLOAT4X4	world = MathHelper::Identity4x4();
	XMFLOAT4X4	texTransform = MathHelper::Identity4x4(); 

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 기하 정보
	GeometryInfo* geometry = nullptr;
	MaterialInfo* material = nullptr;
};

// 조명 정보 구조체
struct LightInfo
{
	UINT lightType = 0;
	XMFLOAT3 padding = { 0.f, 0.f, 0.f };
	XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
	float fallOffStart = 1.0f;
	XMFLOAT3 direction = { 0.f, -1.f, 0.f };
	float fallOffEnd = 10.f;
	XMFLOAT3 position = { 0.f, 0.f, 0.f };
	float spotPower = 64.f;
};

// 공용 상수 (View Projection)
struct PassConstants
{
	XMFLOAT4X4 view = MathHelper::Identity4x4();
	XMFLOAT4X4 invView = MathHelper::Identity4x4(); // inverse
	XMFLOAT4X4 proj = MathHelper::Identity4x4();
	XMFLOAT4X4 invProj = MathHelper::Identity4x4();
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();

	// 조명 정보
	XMFLOAT4 ambientColor = { 0.f, 0.f, 0.f,1.f };
	XMFLOAT3 eyePosW = { 0.f, 0.f, 0.f };
	UINT lightCount = MAXLIGHTS;
	LightInfo lights[MAXLIGHTS];
};

// Texture 구조체
struct TextureInfo
{
	string name;
	wstring fileName;

	ComPtr<ID3D12Resource> resource = nullptr;
	ComPtr<ID3D12Resource> uploadHeap = nullptr;
};

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;

	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCB(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt) override;

	virtual void Draw(const GameTimer& gt) override;
	void DrawRenderItems(vector<RenderItem*>& renderItems);

	virtual void DrawEnd(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)  override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	// 테스쳐 로드
	void LoadTextures();

	// 기하 도형 생성
	void BuildBoxGeometry();
	void BuildGridGeometry();
	void BuildSphereGeometry();
	void BuildCylinderGeometry();
	void BuildSkullGeometry();

	// 재질 생성
	void BuildMaterials();

	// 렌더링할 아이템 만들기
	void BuildRenderItems();

	// 세팅
	void BuildInputLayout();
	void BuildShader();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildPSO();

private:

	// ------- 입력 배치 ------- 
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// 개별 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mObjectCB = nullptr;		// 열고 닫고 X
	BYTE* mObjectMappedData = nullptr;				// 복사되는 것
	UINT mObjectByteSize = 0;

	// 개별 오브젝트 머티리얼 상수 버퍼
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// 공용 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// 오브젝트마다 뷰를 만들 수 없으니... 루트 시그니처가 관리하게 해준다
	// (루트 시그니처 > 버퍼) or (루트 시그니처 > Desc 테이블 > 버퍼) 
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	// 서술자 힙 SRV 힙
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	UINT mCbvSrvDescriptorSize = 0;

	// 셰이더 맵
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;

	// 렌더링 파이프라인 스테이트 맵
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;

	// 전체 오브젝트 리스트
	vector<unique_ptr< RenderItem>> mRenderItems;

	// 렌더링 레이어
	vector<RenderItem*> mItemLayer[(int)RenderLayer::Count];

	// 기하 구조 맵
	unordered_map<string, unique_ptr<GeometryInfo>> mGeometries;

	// 재질 구조 맵
	unordered_map<string, unique_ptr<MaterialInfo>> mMateirals;

	// 텍스쳐 구조 맵
	unordered_map<string, unique_ptr<TextureInfo>> mTextures;

	// ------- World / View / Projection -------
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();	// 단위 행렬
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// 카메라 위치
	XMFLOAT3 mEyePos = { 0.f, 0.f, 0.f };

	// 구면 좌표 제어값
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;			// 45
	float mRadius = 5.0f;

	// 마우스 좌표
	POINT mLastMousePos = { 0,0 };

};