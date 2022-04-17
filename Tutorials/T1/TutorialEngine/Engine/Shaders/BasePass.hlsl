
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
	float4 D_LightDirAndIns;
	float3 EyePosW;
};

struct Vertex
{
	float3 PosL  : POSITION;
	float3 NormL : NORMAL;
};

struct Pixel
{
	float4 PosH  : SV_POSITION;
	float4 PosW  : POSITION;
	float3 NormW : NORMAL;
};

Pixel VS(Vertex InVert)
{
	Pixel Out;
	// trans pos from local to homogeneous clip space.
	Out.PosW = mul(float4(InVert.PosL, 1.0f), World);
	Out.PosH = mul(Out.PosW, ViewProj);
	// trans normal from local to world
	Out.NormW = mul(InVert.NormL, (float3x3)WorldInvTrans);

	return Out;
}

float4 PS(Pixel InPix) : SV_Target
{
	// normalize
	float3 NormW = normalize(InPix.NormW);
	float NdotL = max(dot(NormW, D_LightDirAndIns.xyz), 0.0f);
	float3 OutColor = BaseColorFactor.xyz * (NdotL * D_LightDirAndIns.w + 0.05f);

	float3 ToEye = normalize(EyePosW - InPix.PosW.xyz);
	float3 H = normalize(ToEye + D_LightDirAndIns.xyz);
	float HdotN = max(dot(H, NormW), 0.0f);
	OutColor += 0.65f * pow(HdotN, 32);

	return float4(OutColor, 1.0f);
}


