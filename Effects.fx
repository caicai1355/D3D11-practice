struct Light
{
	float3 dir;
	float4 ambientIntensity;
	float4 lightIntensity;
};
struct PointLight
{
	float3 pos;
	float range;
	float4 lightIntensity;
	float3 attr;
};
struct SpotLight
{
	float3 pos;
	float range;
	float4 lightIntensity;
	float3 distanceAttr;
	float3 dir;
	float deflectAttr;
};

cbuffer CBufferMatrix : register(b0)
{
	float4x4 WVP;
	float4x4 worldSpace;
	float4 difColor;
	bool hasTexture;
	bool hasNormalMap;
};

cbuffer CBufferLight : register(b1)
{
	Light light;
};

cbuffer CBufferPointLight : register(b2)
{
	PointLight pointLight;
};

cbuffer CBufferSpotLight : register(b3)
{
	SpotLight spotLight;
};

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
SamplerState ObjSamplerState;
TextureCube SkyBoxTexture : register(t2);

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : POSITION;	//试一下改成 SV_POSITION 会怎样，会报错。。。顺便一提这里不用SV是因为不想进行透视除法
	float2 Texture : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};
struct VS_SKYBOX_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Texture : TEXCOORD;
};


VS_OUTPUT VS(float4 inPos : POSITION,float2 inTexture : TEXTURE,float3 inNormal : NORMAL,float3 inTangent : TANGENT)
{
	VS_OUTPUT output;
	output.Pos = mul(inPos,WVP);
	output.Texture = inTexture;
	output.WorldPos = mul(inPos,worldSpace);
	output.Normal = mul(inNormal,worldSpace);
	output.Tangent = mul(inTangent,worldSpace);
	return output;
}

VS_SKYBOX_OUTPUT SKYBOX_VS(float4 inPos : POSITION,float2 inTexture : TEXTURE)
{
	VS_SKYBOX_OUTPUT output;
	output.Pos = float4(mul(inPos,WVP).xyww);
	output.Texture = inPos;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{

	float PointD,SpotD;
	float3 pointLightVector,spotLightVector;
	float3 norLightDir,norSpotLightDir;

	float4 temp;
	input.Normal = normalize(input.Normal);
	if(hasTexture == true)
		temp = ObjTexture.Sample( ObjSamplerState, input.Texture);
	else
		temp = difColor;
	float alpha = temp.a;

	if(hasNormalMap == true)
	{
		float3 Bitangent;
		float3 textureMapNormal;
		input.Tangent = normalize(input.Tangent);
		input.Tangent = normalize(input.Tangent - input.Normal * dot(input.Tangent,input.Normal));
		Bitangent = cross(input.Normal,input.Tangent);
		textureMapNormal = NormalMap.Sample( ObjSamplerState, input.Texture);
		textureMapNormal = -1.0f + 2.0f * textureMapNormal;
		input.Normal = normalize(mul(textureMapNormal,float3x3(input.Tangent,Bitangent,input.Normal)));
	}

	//ambient light
	float4 color = light.ambientIntensity * temp;
	//directional light
	norLightDir = normalize(light.dir);
	color += light.lightIntensity * temp * saturate(dot(-norLightDir,input.Normal));
	//point light
	pointLightVector = pointLight.pos - input.WorldPos;
	PointD = length(pointLightVector);
	if(PointD < pointLight.range)
	{
		pointLightVector /= PointD;
		color += pointLight.lightIntensity / (pointLight.attr[0] + pointLight.attr[1]*PointD + pointLight.attr[2]*PointD*PointD) * temp * saturate(dot(pointLightVector,input.Normal));
	}
	//spot light
	spotLightVector = spotLight.pos - input.WorldPos;
	SpotD = length(spotLightVector);
	if(SpotD < spotLight.range)
	{
		norSpotLightDir = normalize(spotLight.dir);
		spotLightVector /= SpotD;
		color += spotLight.lightIntensity / (spotLight.distanceAttr[0] + spotLight.distanceAttr[1]*SpotD + spotLight.distanceAttr[2]*SpotD*SpotD) * pow(max(dot(spotLightVector,-norSpotLightDir),0.0f),spotLight.deflectAttr) * temp * saturate(dot(spotLightVector,input.Normal));
	}
	
	return float4(color.rgb,alpha);
	//return float4(0.0f, 0.0f, 1.0f, 1.0f);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	float4 temp = ObjTexture.Sample( ObjSamplerState, input.Texture);
	//clip(temp.a - 0.25);
	return temp;
}

float4 SKYBOX_PS(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
	float4 temp = SkyBoxTexture.Sample( ObjSamplerState, input.Texture);
	return temp;
	//return float4(1.0f,1.0f,1.0f,0.0f);
}
