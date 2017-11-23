struct Light
{
	float3 dir;
	float4 ambientIntensity;
	float4 diffuseIntensity;
};

cbuffer CBufferMatrix : register(b0)
{
	float4x4 WVP;
	float4x4 worldSpace;
};

cbuffer CBufferLight : register(b1)
{
	Light light;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
	float4 outPos : SV_POSITION;
	float2 outTexture : TEXTURE;
	float3 outNormal : NORMAL;
};


VS_OUTPUT VS(float4 inPos : POSITION,float2 inTexture : TEXTURE,float3 inNormal : NORMAL)
{
	VS_OUTPUT output;
	output.outPos = mul(inPos,WVP);
	output.outTexture = inTexture;
	output.outNormal = mul(inNormal,worldSpace);
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 1.0f, 1.0f);
    //return float4(ObjTexture.Sample( ObjSamplerState, input.outTexture).x,ObjTexture.Sample( ObjSamplerState, input.outTexture).y,ObjTexture.Sample( ObjSamplerState, input.outTexture).z,0.2f);
	//clip(temp.a - 0.25);

	float4 temp = ObjTexture.Sample( ObjSamplerState, input.outTexture);
	float4 color = light.ambientIntensity * temp;
	color += light.diffuseIntensity * temp * saturate(dot(normalize(light.dir),normalize(input.outNormal)));
	return color;
	//return temp;
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	float4 temp = ObjTexture.Sample( ObjSamplerState, input.outTexture);
	clip(temp.a - 0.25);
	return temp;
}
