#ifndef _SHADOW_HLSL_
#define _SHADOW_HLSL_

#include "Params.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION; // �������
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj); 
    
    // ���� ������ ������ ��ǥ
    
    return vout;
}

// Null Pixel Shader?
// ���� Ÿ���� ���� ������
void PS(VertexOut vout)
{
}

#endif // _SHADOW_HLSL_