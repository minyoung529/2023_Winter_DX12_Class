#ifndef _PARAMS_HLSLI
#define _PARAMS_HLSLI

struct Light
{
    int lightType;
    float3 padding;
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
}

cbuffer cbPerMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
}

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    
    float4 gAmbiendLight;
    float3 gEyePosW;
    int gLightCount;
    Light gLights[10];
}
#endif