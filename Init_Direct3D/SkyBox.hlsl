#ifndef _SKYBOX_HLSLI
#define _SKYBOX_HLSLI

#include "Params.hlsl"

struct VertexIn
{
    float3 PosL : POSITION; // local
    float3 NormalL : NORMAL;
    float3 Uv : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION; 
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    posW.xyz += gEyePosW;                   // sphere가 카메라를 감싸는 느낌?
    
    vout.PosH = mul(posW, gViewProj).xyww; // 투영행렬 z = 1
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gSampler_0, pin.PosL);
}

#endif