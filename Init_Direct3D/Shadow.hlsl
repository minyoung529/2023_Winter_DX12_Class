#ifndef _SHADOW_HLSL_
#define _SHADOW_HLSL_

#include "Params.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION; // 투영행렬
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj); 
    
    // 광원 기준의 정점의 좌표
    
    return vout;
}

// Null Pixel Shader?
// 렌더 타겟이 없기 때문에
void PS(VertexOut vout)
{
}

#endif // _SHADOW_HLSL_