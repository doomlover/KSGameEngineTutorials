
Texture2D GShadowMap : register(t0);
Texture2D GSceneColorMap : register(t1);

SamplerComparisonState GSamplerShadow : register(s0);
SamplerState GSamplerLinearClamp : register(s1);

cbuffer PrimitiveConstBuffer : register(b0)
{
	float4x4 World;
	float4x4 WorldInvTrans;
	float4 BaseColorFactor;
	float4 RoughnessMetallicFactor;
};

cbuffer PassConstBuffer : register(b1)
{
	float4x4 ViewProj;
    float4x4 LightProj;
    float4x4 LightProjTex;
	float4 D_LightDirAndIns;
	float3 EyePosW;
};

struct Vertex
{
	float3 PosL : POSITION;
	float3 NormL : NORMAL;
};

struct Pixel
{
	float4 PosH : SV_POSITION;
	float4 PosW : POSITION0;
    float4 ShadowPosH : POSITION1;
	float3 NormW : NORMAL;
};

struct PixelShadowPass
{
    float4 PosH : SV_POSITION;
};

float CalcShadowFactor(float4 ShadowPosH)
{
    float DepthToLight = ShadowPosH.z;
    uint Width, Height, NumMips;
    GShadowMap.GetDimensions(0, Width, Height, NumMips);
    float dx = 1.0f / (float)Width;
    const float2 Offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, dx), float2(0.0f, dx), float2(dx, dx)
    };
    float ShadowFactor = 0.0f;
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        ShadowFactor += GShadowMap.SampleCmpLevelZero(
            GSamplerShadow, ShadowPosH.xy + Offsets[i], DepthToLight).r;
    }
    return ShadowFactor/9.0f;
}