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

// ���� ����
// 4 ���� �ʿ� ����
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	XMFLOAT3 tangent;
};

// ���� ������Ʈ ��� (World)
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4(); // ���� ���
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4(); // ���� ���
};

// ���� ������Ʈ�� ���� ���
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

	// ���� ���� ��
	ComPtr<ID3D12Resource>		vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};

	// �ε��� ���� ��
	ComPtr<ID3D12Resource>		indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW		indexBufferView = {};

	// ���� ����
	int vertexCount = 0;

	// �ε��� ����
	int indexCount = 0;
};

// Material ����ü
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

// ������Ʈ ����ü
struct RenderItem
{
	RenderItem() = default;

	UINT		objCbIndex = -1;
	XMFLOAT4X4	world = MathHelper::Identity4x4();
	XMFLOAT4X4	texTransform = MathHelper::Identity4x4(); 

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// ���� ����
	GeometryInfo* geometry = nullptr;
	MaterialInfo* material = nullptr;
};

// ���� ���� ����ü
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

// ���� ��� (View Projection)
struct PassConstants
{
	XMFLOAT4X4 view = MathHelper::Identity4x4();
	XMFLOAT4X4 invView = MathHelper::Identity4x4(); // inverse
	XMFLOAT4X4 proj = MathHelper::Identity4x4();
	XMFLOAT4X4 invProj = MathHelper::Identity4x4();
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 inViewProj = MathHelper::Identity4x4();

	// ����Ʈ
	XMFLOAT4X4 shadowTransform = MathHelper::Identity4x4();

	// ���� ����
	XMFLOAT4 ambientColor = { 0.f, 0.f, 0.f,1.f };
	XMFLOAT3 eyePosW = { 0.f, 0.f, 0.f };
	UINT lightCount = MAXLIGHTS;
	LightInfo lights[MAXLIGHTS];

	// �Ȱ� ȿ�� ����
	XMFLOAT4 fogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	XMFLOAT2 fogPadding;
};

// Texture ����ü
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
	// �׽��� �ε�
	void LoadTextures();

	// ���� ���� ����
	void BuildBoxGeometry();
	void BuildGridGeometry();
	void BuildSphereGeometry();
	void BuildCylinderGeometry();
	void BuildQuadGeometry();
	void BuildSkullGeometry();

	// ���� ����
	void BuildMaterials();

	// �������� ������ �����
	void BuildRenderItems();

	// ����
	void BuildInputLayout();
	void BuildShader();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildPSO();

	array<const CD3DX12_STATIC_SAMPLER_DESC, 2> GetStaticSampler();

private:

	// ------- �Է� ��ġ ------- 
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;		// ���� �ݰ� X
	BYTE* mObjectMappedData = nullptr;				// ����Ǵ� ��
	UINT mObjectByteSize = 0;

	// ���� ������Ʈ ��Ƽ���� ��� ����
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// ������Ʈ���� �並 ���� �� ������... ��Ʈ �ñ״�ó�� �����ϰ� ���ش�
	// (��Ʈ �ñ״�ó > ����) or (��Ʈ �ñ״�ó > Desc ���̺� > ����) 
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	// ������ �� SRV ��
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	UINT mCbvSrvDescriptorSize = 0;

	// ���̴� ��
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;

	// ������ ���������� ������Ʈ ��
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;

	// ��ü ������Ʈ ����Ʈ
	vector<unique_ptr<RenderItem>> mRenderItems;

	// ������ ���̾�
	vector<RenderItem*> mItemLayer[(int)RenderLayer::Count];

	// ���� ���� ��
	unordered_map<string, unique_ptr<GeometryInfo>> mGeometries;

	// ���� ���� ��
	unordered_map<string, unique_ptr<MaterialInfo>> mMateirals;

	// �ؽ��� ���� ��
	unordered_map<string, unique_ptr<TextureInfo>> mTextures;


	// �׸��� ��
	unique_ptr<ShadowMap> mShadowMap;

	// ��ī�̹ڽ� �ؽ��� �ε���
	UINT mSkyboxTexHeapIndex = 0;

	// �׸��� �� �ؽ��� �ε���
	UINT mShadowMapHeapIndex = 0;

	// �׸��� �� ��ũ����
	CD3DX12_GPU_DESCRIPTOR_HANDLE mShadowMapSrv;

	// ��� �� : ���� �̵�
	DirectX::BoundingSphere mSceneBounds;

	// ����Ʈ ���� ���
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

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0,0 };

};