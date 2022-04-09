
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
	float4x4 gWorldInvTrans;
};

cbuffer cbPass : register(b1)
{
	float4x4 gViewProj;
};

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormL : NORMAL;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float3 NormW : NORMAL;
    //float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	vout.NormW = mul(vin.NormL, (float3x3)gWorldInvTrans);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// normalize
	pin.NormW = normalize(pin.NormW);
	float4 out_color = pow(float4(pin.NormW * 0.5 + 0.5, 1.0), 1/2.2);
    return out_color;
}


