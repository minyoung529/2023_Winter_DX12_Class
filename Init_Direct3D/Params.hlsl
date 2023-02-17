#ifndef _PARAMS_HLSLI
#define _PARAMS_HLSLI

#define MAXLIGHTS 16

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
    float4x4 gTexTransform;
}

cbuffer cbPerMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    int gTex_On;
    int gNormal_On;
    float2 g_texPadding;
}

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowTransform;
    
    float4 gAmbiendLight;
    float3 gEyePosW;
    int gLightCount;
    Light gLights[MAXLIGHTS];
    
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 fogPadding;
}

TextureCube gCubeMap    : register(t0);
Texture2D gTexture_0    : register(t1);
Texture2D gNormal_0     : register(t2);
Texture2D gShadowMap    : register(t3);

SamplerState gSampler_0 : register(s0);
SamplerComparisonState gSampleShadow : register(s1);

// Transform normal map sample to world space 
// -1~1 => world space
float3 NormalSampleToWorld(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f; // 0~1
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N); // 
    float3 B = cross(N, T); // BINORMAL
    
    float3x3 TBN = float3x3(T, B, N);
 
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW; // 노말 정보 추출
}

float CalcShadowFactor(float4 shadowPosH)
{
    // 그... 그.... 그.... 그림자계수
    shadowPosH.xyz /= shadowPosH.w; // 동차좌표계 (투영좌표 만듦)
    float depth = shadowPosH.z;
 
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    
    float dx = 1.0f / (float) width;
    
    float percentLit = 0.0f;
    const float2 offset[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, dx), float2(0.0f, dx), float2(dx, dx)
    };

    [unroll]
    for (int i = 0; i < 9; i++)
    {
        // 이 함수 때문에 샘플링을 다른 걸 씀
        // 화면 크기를 보간?
        percentLit += gShadowMap.SampleCmpLevelZero(gSampleShadow, shadowPosH.xy + offset[i], depth).r;
    }

    // 평균값 구해서 돌려줌
    return percentLit / 9.0f;
}
#endif