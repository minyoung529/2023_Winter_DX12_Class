#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common/MathHelper.h"

using namespace DirectX;
using namespace std;

// ���� ����
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT4 color;
};

// ���� ������Ʈ ��� (World)
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4(); // ���� ���
};

// ������Ʈ ����ü
struct RenderItem
{
	RenderItem() = default;

	UINT objCbIndex = -1;
	XMFLOAT4X4 world = MathHelper::Identity4x4();

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

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


// ���� ��� (View Projection)
struct PassConstants
{
	XMFLOAT4X4 view = MathHelper::Identity4x4();
	XMFLOAT4X4 invView = MathHelper::Identity4x4(); // inverse

	XMFLOAT4X4 proj = MathHelper::Identity4x4();
	XMFLOAT4X4 invProj = MathHelper::Identity4x4();

	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
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
	void UpdatePassCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt) override;

	virtual void Draw(const GameTimer& gt) override;
	void DrawRenderItems(const GameTimer& gt);

	virtual void DrawEnd(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)  override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	void BuildGeometry();
	void BuildCube();
	void BuildLandGeometry();
	void BuildInputLayout();
	void BuildShader();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildPSO();

private:

	// ------- �Է� ��ġ ------- 
	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// Vertex Shader, Pixel Shader
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;	// ���� �ݰ� X
	BYTE* mObjectMappedData = nullptr;			// ����Ǵ� ��
	UINT mObjectByteSize = 0;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mPassCB = nullptr;	// ���� �ݰ� X
	BYTE* mPassMappedData = nullptr;			// ����Ǵ� ��
	UINT mPassByteSize = 0;

	// ������Ʈ���� �並 ���� �� ������... ��Ʈ �ñ״�ó�� �����ϰ� ���ش�
	// (��Ʈ �ñ״�ó > ����) or (��Ʈ �ñ״�ó > Desc ���̺� > ����) 
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	// ���������� ���� ��ü
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	// �������� ������Ʈ ����Ʈ
	vector<unique_ptr< RenderItem>> mRenderItems;

	// ------- World / View / Projection -------
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();	// ���� ���
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ���� ��ǥ ���
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;			// 45
	float mRadius = 5.0f;

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0,0 };

};