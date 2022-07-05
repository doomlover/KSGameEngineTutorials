
#include "Common.hlsl"

struct VertexPostProcess
{
    float3 PosL : POSITION;
};

struct PixelPostProcess
{
    float4 PosH : SV_POSITION;
};

PixelPostProcess VS(VertexPostProcess InVert)
{
    PixelPostProcess OutPixel;
    OutPixel.PosH = float4(InVert.PosL, 1.0f);
    return OutPixel;
}

float4 PS(PixelPostProcess InPix) : SV_Target
{
    uint SceneColorW, SceneColorH, NumMips;
    GSceneColorMap.GetDimensions(0, SceneColorW, SceneColorH, NumMips);
    float2 SceneColorUV = float2(InPix.PosH.x / SceneColorW, InPix.PosH.y / SceneColorH);
    float3 SceneColor = GSceneColorMap.Sample(GSamplerLinearClamp, SceneColorUV).xyz;
    // tonemapping
    float3 OutputColor = SceneColor / (SceneColor + 1.0f);
    // gamma
    const float Gamma = 1.0f / 2.2f;
    OutputColor = pow(OutputColor, float3(Gamma, Gamma, Gamma));
    return float4(OutputColor, 1.0f);
}