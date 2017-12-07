struct Light
{
	float3 dir;
	float4 ambientIntensity;
	float4 lightIntensity;
};
struct PointLight
{
	float3 pos;
	float4 lightIntensity;
	float3 attr;
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

cbuffer CBufferPointLight : register(b2)
{
	PointLight pointLight;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : POSITION;	//试一下改成 SV_POSITION 会怎样，会报错。。。顺便一提这里不用SV是因为不想进行透视除法
	float2 Texture : TEXTURE;
	float3 Normal : NORMAL;
};
struct VS_SKYBOX_OUTPUT
{
	float4 Pos : SV_POSITION;
	float2 Texture : TEXTURE;
};


VS_OUTPUT VS(float4 inPos : POSITION,float2 inTexture : TEXTURE,float3 inNormal : NORMAL)
{
	VS_OUTPUT output;
	output.Pos = mul(inPos,WVP);
	output.Texture = inTexture;
	output.WorldPos = mul(inPos,worldSpace);
	output.Normal = mul(inNormal,worldSpace);
	return output;
}

VS_SKYBOX_OUTPUT SKYBOX_VS(float4 inPos : POSITION,float2 inTexture : TEXTURE,float3 inNormal : NORMAL)
{
	VS_SKYBOX_OUTPUT output;
	output.Pos = mul(inPos,WVP);
	//output.Pos = float4(mul(inPos,WVP).xy,mul(inPos,WVP).w * 0.99,mul(inPos,WVP).w);
	//output.Pos = float4(mul(inPos,WVP).xyww);
	output.Texture = inPos;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 1.0f, 1.0f);
    //return float4(ObjTexture.Sample( ObjSamplerState, input.Texture).x,ObjTexture.Sample( ObjSamplerState, input.Texture).y,ObjTexture.Sample( ObjSamplerState, input.Texture).z,0.2f);
	//clip(temp.a - 0.25);
	float d;
	float3 pointLightVector;

	float4 temp = ObjTexture.Sample( ObjSamplerState, input.Texture);
	float4 color = light.ambientIntensity * temp;
	color += light.lightIntensity * temp * saturate(dot(normalize(light.dir),normalize(input.Normal)));

	pointLightVector = pointLight.pos - input.WorldPos;
	d = length(pointLightVector);
	pointLightVector /= d;
	color += pointLight.lightIntensity / (pointLight.attr[0] + pointLight.attr[1]*d + pointLight.attr[2]*d*d) * temp * saturate(dot(pointLightVector,normalize(input.Normal)));

	return color;
	//return temp;
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	float4 temp = ObjTexture.Sample( ObjSamplerState, input.Texture);
	//clip(temp.a - 0.25);
	return temp;
}

float4 SKYBOX_PS(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
	float4 temp = ObjTexture.Sample( ObjSamplerState, input.Texture);
	//return temp;
	return float4(1.0f,1.0f,1.0f,0.0f);
}
