#pragma once

#include "D3dApp.h"
#include "D3dHeader.h"
#include <DirectXColors.h>
#include "../Common/MathHelper.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"
#include "../Common/Camera.h"
#include "ShadowMap.h"
#include "LoadM3d.h"
#include "SkinnedData.h"

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
	void UpdateSkinnedPassCBs(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt) override;
	
	virtual void Draw(const GameTimer& gt) override;
	void DrawRenderItems(vector<RenderItem*>& renderItems);
	void DrawSceneToShadowMap();

	virtual void DrawEnd(const GameTimer& gt) override;
	
	virtual void OnMouseDown(WPARAM btnState, int x, int y)  override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	// Skinned Model �ε�
	void LoadSkinnedModel();

	// �ؽ��� �ε�
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

	// ��Ű�� �ִϸ��̼ǿ� �Է� ������
	vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;		// ���� �ݰ� X
	BYTE* mObjectMappedData = nullptr;				// ����Ǵ� ��
	UINT mObjectByteSize = 0;

	// ���� ������Ʈ ��Ƽ���� ��� ����
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;	// �ٷ� ���ε尡 �� �ǹǷ�...
	UINT mMaterialByteSize = 0;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// Bone Transform ����
	ComPtr<ID3D12Resource> mSkinnedCB = nullptr;
	BYTE* mSkinnedMappedData = nullptr;
	UINT mSkinnedByteSize = 0;

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

	// Skinned Model Data
	UINT mSkinnedSrvHeapStart = 0;
	string mSkinnedModelFileName = "..\\Models\\soldier.m3d";
	SkinnedData mSkinnedInfo;
	vector<M3DLoader::Subset> mSkinnedSubsets;
	vector<M3DLoader::M3dMaterial> mSkinnedMats;
	vector<string> mSkinnedTextureName;

	unique_ptr<SkinnedModelInstance> mSkinnedModelInst;

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