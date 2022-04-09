
cbuffer PrimitiveConstBuffer : register(b0)
{
	float4x4 gWorld; 
	float4x4 gWorldInvTrans;
};

cbuffer PassConstBuffer : register(b1)
{
	float4x4 view_proj_mat;
	float3 directional_light_dir;
	float directional_light_intensity;
};

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormL : NORMAL;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float3 norm_w : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, view_proj_mat);
	
	vout.norm_w = mul(vin.NormL, (float3x3)gWorldInvTrans);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// normalize
	float3 normal = normalize(pin.norm_w);
	float n_dot_l = max(dot(directional_light_dir, normal), 0.0f);
	float3 out_color = float3(1.0f, 1.0f, 1.0f) * n_dot_l * directional_light_intensity;
    return float4(out_color, 1.0f);
}


