cbuffer CBufferStruct
{
	float4x4 WVP;
};

struct VS_OUTPUT
{
	float4 outPos : SV_POSITION;
	float4 outColor : COLOR;
};


VS_OUTPUT VS(float4 inPos : POSITION,float4 inColor : COLOR)
{
	VS_OUTPUT output;
	output.outPos = mul(inPos,WVP);
	output.outColor = inColor;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 1.0f, 1.0f);
    return input.outColor;
}
