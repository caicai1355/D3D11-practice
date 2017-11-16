cbuffer CBufferStruct
{
	float4x4 WVP;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
	float4 outPos : SV_POSITION;
	float2 outTexture : TEXTURE;
};


VS_OUTPUT VS(float4 inPos : POSITION,float2 inTexture : TEXTURE)
{
	VS_OUTPUT output;
	output.outPos = mul(inPos,WVP);
	output.outTexture = inTexture;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 1.0f, 1.0f);
    //return float4(ObjTexture.Sample( ObjSamplerState, input.outTexture).x,ObjTexture.Sample( ObjSamplerState, input.outTexture).y,ObjTexture.Sample( ObjSamplerState, input.outTexture).z,0.2f);
	float4 temp = ObjTexture.Sample( ObjSamplerState, input.outTexture);
	clip(temp.a - 0.25);
	return temp;
}
