#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common/MathHelper.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"
#include "../Common/Camera.h"
#include "ShadowMap.h"

using namespace DirectX;
using namespace std;

#define MAXLIGHTS 16

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Debug,
	SkyBox,
	Count
};

// 정점 정보
// 4 맞출 필요 없음
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	XMFLOAT3 tangent;
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
	UINT normal_on = 0;
	XMFLOAT2 padding = { 0.f, 0.f };
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
	int normalSrvHeapIndex = -1;

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
	XMFLOAT4X4 inViewProj = MathHelper::Identity4x4();

	// 라이트
	XMFLOAT4X4 shadowTransform = MathHelper::Identity4x4();

	// 조명 정보
	XMFLOAT4 ambientColor = { 0.f, 0.f, 0.f,1.f };
	XMFLOAT3 eyePosW = { 0.f, 0.f, 0.f };
	UINT lightCount = MAXLIGHTS;
	LightInfo lights[MAXLIGHTS];

	// 안개 효과 정보
	XMFLOAT4 fogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	XMFLOAT2 fogPadding;
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
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt) override;
	
	virtual void Draw(const GameTimer& gt) override;
	void DrawRenderItems(vector<RenderItem*>& renderItems);
	void DrawSceneToShadowMap();

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
	void BuildQuadGeometry();
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

	array<const CD3DX12_STATIC_SAMPLER_DESC, 2> GetStaticSampler();

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
	vector<unique_ptr<RenderItem>> mRenderItems;

	// 렌더링 레이어
	vector<RenderItem*> mItemLayer[(int)RenderLayer::Count];

	// 기하 구조 맵
	unordered_map<string, unique_ptr<GeometryInfo>> mGeometries;

	// 재질 구조 맵
	unordered_map<string, unique_ptr<MaterialInfo>> mMateirals;

	// 텍스쳐 구조 맵
	unordered_map<string, unique_ptr<TextureInfo>> mTextures;


	// 그림자 맵
	unique_ptr<ShadowMap> mShadowMap;

	// 스카이박스 텍스쳐 인덱스
	UINT mSkyboxTexHeapIndex = 0;

	// 그림자 맵 텍스쳐 인덱스
	UINT mShadowMapHeapIndex = 0;

	// 그림자 맵 디스크립터
	CD3DX12_GPU_DESCRIPTOR_HANDLE mShadowMapSrv;

	// 경계 구 : 광원 이동
	DirectX::BoundingSphere mSceneBounds;

	// 라이트 관련 행렬
	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;

	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirection = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
	XMFLOAT3 mRotatedLightDirection;

	// ------- World / View / Projection -------
	Camera mCamera;

	// 마우스 좌표
	POINT mLastMousePos = { 0,0 };

};