cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
}

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
}

struct VertexIn
{
    float3 PosL : POSITION; // Local Position
    float4 Color : COLOR;   
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

// Vertex Shader
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    vout.Color = vin.Color;
    
    return vout;
}

// PS(VS())
float4 PS(VertexOut pin) : SV_Target // Default Target
{
    return pin.Color;
}