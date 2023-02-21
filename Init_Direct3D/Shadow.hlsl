#ifndef _SHADOW_HLSL_
#define _SHADOW_HLSL_

#include "Params.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
#ifdef SKINNED
    float3 BoneWeights  : WEIGHTS;
    uint4 BoneIndices   : BONEINDICES;
#endif // SKINNED
    
};

struct VertexOut
{
    float4 PosH : SV_POSITION; // 투영행렬
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
#ifdef SKINNED
    float weights[4] = { 0.0f,0.0f,0.0f,0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];
    
    float3 posL = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < 4; i++)
    {
        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTrnasforms[vin.BoneIndices[i]]).xyz;
    }
    
    vin.PosL = posL;
#endif // SKINNED

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