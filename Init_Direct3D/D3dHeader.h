#pragma once

#include "SkinnedData.h"
#include "../Common/d3dUtil.h"
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

#define MAXLIGHTS 16

enum class RenderLayer : int
{
	Opaque = 0,
	SkinnedOpaque,
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

// �ִϸ��̼��� �ִ� ���� ����
struct SkinnedVertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	XMFLOAT3 tangent;
	XMFLOAT3 boneWeights;
	BYTE boneIndices[4];
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

	// �ε��� ����
	UINT startIndexLocation = 0;
	// ���ؽ� ����
	int baseVertexLocation = 0;
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

// �ִϸ��̼� ó���� ����
struct SkinnedModelInstance
{
	SkinnedData* skinnedInfo = nullptr;	// �ϳ��� Ŭ�� ������ ������ ��
	vector<XMFLOAT4X4> finalTransforms;
	string clipName;
	float timePos = 0.0f;

	void UpdateSkinnedAnimation(float dt)
	{
		timePos += dt;

		// Ŭ�� �̸� - �ð�?
		// �ݺ� ����
		if (timePos > skinnedInfo->GetClipEndTime(clipName))
		{
			timePos = 0;
		}

		skinnedInfo->GetFinalTransforms(clipName, timePos, finalTransforms);
	}
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

	UINT skinnedCBIndex = 0;
	SkinnedModelInstance* skinnedModelInst = nullptr;
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

// BoneTransform ��� ����
struct SkinnedConstants
{
	XMFLOAT4X4 boneTransform[96];
};

// Texture ����ü
struct TextureInfo
{
	string name;
	wstring fileName;

	ComPtr<ID3D12Resource> resource = nullptr;
	ComPtr<ID3D12Resource> uploadHeap = nullptr;
};
