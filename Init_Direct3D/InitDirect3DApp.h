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

// ���� ����
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
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
	XMFLOAT3 padding = { 0.f, 0.f, 0.f };
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

	// ���� ����
	XMFLOAT4 ambientColor = { 0.f, 0.f, 0.f,1.f };
	XMFLOAT3 eyePosW = { 0.f, 0.f, 0.f };
	UINT lightCount = MAXLIGHTS;
	LightInfo lights[MAXLIGHTS];
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
	void UpdatePassCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt) override;

	virtual void Draw(const GameTimer& gt) override;
	void DrawRenderItems(vector<RenderItem*>& renderItems);

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
	vector<unique_ptr< RenderItem>> mRenderItems;

	// ������ ���̾�
	vector<RenderItem*> mItemLayer[(int)RenderLayer::Count];

	// ���� ���� ��
	unordered_map<string, unique_ptr<GeometryInfo>> mGeometries;

	// ���� ���� ��
	unordered_map<string, unique_ptr<MaterialInfo>> mMateirals;

	// �ؽ��� ���� ��
	unordered_map<string, unique_ptr<TextureInfo>> mTextures;

	// ------- World / View / Projection -------
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();	// ���� ���
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ī�޶� ��ġ
	XMFLOAT3 mEyePos = { 0.f, 0.f, 0.f };

	// ���� ��ǥ ���
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;			// 45
	float mRadius = 5.0f;

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0,0 };

};