Texture2D g_TextureDiffuse;

SamplerState g_SamplerLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register( b0 )
{
	float viewport_width;
	float viewport_height;

	float window_xpos;
	float window_ypos;

	float window_width;
	float window_height;
	float pad[2];
};

struct VS_INPUT
{
    float3 vPosition        : POSITION;
    float2 vTexCoord0       : TEXCOORD0;
};

struct VS_OUTPUT
{
    float2 vTexCoord0       : TEXCOORD0;
    float4 vPosition        : SV_POSITION;
};

struct PS_INPUT
{
    float2 vTexCoord0       : TEXCOORD0;
};

VS_OUTPUT QuadVS( VS_INPUT Input )
{
    VS_OUTPUT Output;
    

	float2 coords = Input.vPosition.xy;
	coords = coords*0.5 + 0.5;
	coords = coords*float2(window_width/viewport_width, window_height/viewport_height);
	coords = coords*2 - 1;
	coords += float2(window_xpos/viewport_width,window_ypos/viewport_height);

	Output.vPosition = float4(coords,0,1);
    Output.vTexCoord0 = Input.vTexCoord0;
    return Output;
}

float4 QuadPS( PS_INPUT Input) : SV_TARGET
{
    float2 vTexCoord0Mod = Input.vTexCoord0;
    vTexCoord0Mod.y = 1.0f - vTexCoord0Mod.y;
    return g_TextureDiffuse.Sample( g_SamplerLinear, vTexCoord0Mod );
}
