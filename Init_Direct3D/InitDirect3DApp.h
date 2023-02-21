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
	// Skinned Model 로드
	void LoadSkinnedModel();

	// 텍스쳐 로드
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

	// 스키닝 애니메이션용 입력 조립기
	vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

	// 개별 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mObjectCB = nullptr;		// 열고 닫고 X
	BYTE* mObjectMappedData = nullptr;				// 복사되는 것
	UINT mObjectByteSize = 0;

	// 개별 오브젝트 머티리얼 상수 버퍼
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;	// 바로 업로드가 안 되므로...
	UINT mMaterialByteSize = 0;

	// 공용 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// Bone Transform 버퍼
	ComPtr<ID3D12Resource> mSkinnedCB = nullptr;
	BYTE* mSkinnedMappedData = nullptr;
	UINT mSkinnedByteSize = 0;

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

	// Skinned Model Data
	UINT mSkinnedSrvHeapStart = 0;
	string mSkinnedModelFileName = "..\\Models\\soldier.m3d";
	SkinnedData mSkinnedInfo;
	vector<M3DLoader::Subset> mSkinnedSubsets;
	vector<M3DLoader::M3dMaterial> mSkinnedMats;
	vector<string> mSkinnedTextureName;

	unique_ptr<SkinnedModelInstance> mSkinnedModelInst;

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