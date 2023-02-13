#ifndef _COLOR_HLSLI
#define _COLOR_HLSLI

#include "Params.hlsl"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosL : POSITION; // local
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

// Vertex Shader
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    return vout;
}

// PS(VS())
float4 PS(VertexOut pin) : SV_Target // Default Target
{
    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    float4 ambient = gAmbiendLight * gDiffuseAlbedo;
    const float shininess = 1.0f - gRoughness;
    
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, pin.NormalW, toEyeW);
    float4 litColor = ambient + directLight;
    
    litColor.a = gDiffuseAlbedo.a;

    return litColor;
}

#endif