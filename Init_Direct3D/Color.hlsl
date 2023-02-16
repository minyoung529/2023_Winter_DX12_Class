#ifndef _COLOR_HLSLI
#define _COLOR_HLSLI

#include "Params.hlsl"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosL     : POSITION; // local
    float3 NormalL  : NORMAL;
    float3 Uv       : TEXCOORD;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW  : NORMAL;
    float2 Uv       : TEXCOORD;
};

// Vertex Shader
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    float4 Uv = mul(float4(vin.Uv, 0.0f), gTexTransform);
    vout.Uv = Uv.xy;
    return vout;
}

// PS(VS())
float4 PS(VertexOut pin) : SV_Target // Default Target
{
    float4 diffuseAlbedo = gDiffuseAlbedo ;
    
    if(gTex_On)
    {
        diffuseAlbedo *= gTexture_0.Sample(gSampler_0, pin.Uv);
    }
    
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    float4 ambient = gAmbiendLight * diffuseAlbedo;
    const float shininess = 1.0f - gRoughness;
    
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, pin.NormalW, toEyeW);
    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;

    return litColor;
}

#endif