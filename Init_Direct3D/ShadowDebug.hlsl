#ifndef _SHADOW_DEBUG_HLSL_
#define _SHADOW_DEBUG_HLSL_

#include "Params.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 Uv : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION; // 투영행렬
    float2 Uv : TEXCOORDx;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.PosH = float4(vin.PosL, 1.0f);
    vout.Uv = vin.Uv;
    
    return vout;
}

// Null Pixel Shader?
// 렌더 타겟이 없기 때문에
float4 PS(VertexOut pin) : SV_Target
{
    return float4(gShadowMap.Sample(gSampler_0, pin.Uv).rrr, 1.0f);
}

#endif // _SHADOW_DEBUG_HLSL_