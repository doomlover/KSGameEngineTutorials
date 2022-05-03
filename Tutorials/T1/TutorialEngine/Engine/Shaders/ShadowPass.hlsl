
#include "Common.hlsl"

PixelShadowPass VS(Vertex InVert)
{
	PixelShadowPass Out;
	// trans to world
	float4 PosW = mul(float4(InVert.PosL, 1.0f), World);
	// trans to light space and project to clip space
	Out.PosH = mul(PosW, LightProj);

	return Out;
}

void PS(PixelShadowPass InPix)
{
	
}


