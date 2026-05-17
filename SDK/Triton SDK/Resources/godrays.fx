uniform float4x4 mvProj;
uniform float3 origin;
uniform float spacing;
uniform float3 extinction;
uniform float3 sunDir;
uniform float3 sunColor;
uniform float3 viewer;

uniform float3x3 basis;
uniform float3x3 invBasis;
uniform float2 textureSize;
uniform float3 viewDir;
uniform float visibility;
uniform float fade;

struct VSInput {
float4 position :
    POSITION;
};

#ifdef DX9

struct VSOutput {
float4 position :
    POSITION;
float3 intensity :
    TEXCOORD0;
float3 view :
    TEXCOORD1;
};

struct PSSceneIn {
float4 pos :
    POSITION0;
float3 intensity :
    TEXCOORD0;
float3 view :
    TEXCOORD1;
};

struct PSSceneOut {
float4 color :
    COLOR;
};

TEXTURE displacementMap;
TEXTURE slopeFoamMap;

sampler2D gDisplacementSampler = sampler_state {
    Texture = (displacementMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D gSlopeFoamSampler = sampler_state {
    Texture = (slopeFoamMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

#else

struct VSOutput {
float4 position :
    SV_Position;
float3 intensity :
    TEXCOORD0;
float3 view :
    TEXCOORD1;
};

struct PSSceneIn {
float4 pos :
    SV_Position;
float3 intensity :
    TEXCOORD0;
float3 view :
    TEXCOORD1;
};

struct PSSceneOut {
float4 color :
    SV_Target;
};

Texture2D displacementMap;
Texture2D slopeFoamMap;

SamplerState gTriLinearSamWrap {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

#endif

float3 CalculateWaterNormal(inout float3 v)
{
    float3 localV = mul(v, basis);
    float2 texCoords = localV.xy / textureSize;

#ifdef DX9
    float4 tc4 = float4(texCoords.x, texCoords.y, 0.0f, 0.0f);
    float3 slopesAndFoam = tex2Dlod(gSlopeFoamSampler, tc4).xyz;
    float3 displacement = tex2Dlod(gDisplacementSampler, tc4).xyz;
#else
    float3 slopesAndFoam = slopeFoamMap.SampleLevel(gTriLinearSamWrap, texCoords, 0).xyz;
    float3 displacement = displacementMap.SampleLevel(gTriLinearSamWrap, texCoords, 0).xyz;
#endif

    localV += displacement;

    v = mul(localV, invBasis);

    float3 sx = float3(1.0, 0.0, slopesAndFoam.x);
    float3 sy = float3(0.0, 1.0, slopesAndFoam.y);

    float3 nNorm = mul(normalize(cross(sx, sy)), invBasis);

    return nNorm;
}

float FastFresnel(float3 I, float3 N)
{
    const float R0 = 0.0204;
    return R0 + (1.0 - R0) * pow(1 - dot(I, N), 5.0);
}

VSOutput VS(VSInput Input)
{
    VSOutput Out;

    float4 position = Input.position;

    float3 worldPos = position.xyz * float3(spacing, 1, spacing);
    worldPos += origin;

    float3 normal = CalculateWaterNormal(worldPos);
    float vertDist = length(worldPos - viewer);
    float totalDist = position.w + vertDist;

    if (position.w > 0) {
        float3 refr = refract(-sunDir, normal, 1.0 / 1.333);
        worldPos += refr * position.w;
        Out.intensity = float3(0, 0, 0);
    } else {
        float tr = 1.0 - FastFresnel(sunDir, normal);
        Out.intensity = exp(-totalDist * extinction) * tr * fade;
    }

    Out.view = worldPos - viewer;

    float4 clipPos = mul(float4(worldPos, 1.0), mvProj);
    clipPos.z = 0.01;

    Out.position = clipPos;

    return Out;
}

float ComputeMieScattering(in float3 V, in float3 sunDir)
{
    const float g = 0.924;
    //    const float g = 0.7;

    float cosTheta = dot(normalize(V), normalize(sunDir));

    float num = 1.0 - g * g;
    float den = (1.0 + g * g - 2.0 * g * cosTheta);

    den = sqrt(den);
    den = den * den * den;

    float phase = (1.0 / (4.0 * 3.14159)) * (num / den);

    return phase;
}

#ifdef DX9
PSSceneOut PS(float4 pos : VPOS, float3 pIntensity : TEXCOORD0, float3 pView : TEXCOORD1)
{
#else
PSSceneOut PS(PSSceneIn input)
{
    float3 pView = input.view;
    float3 pIntensity = input.intensity;
#endif
    float3 up = mul(float3(0, 0, 1), invBasis);
    float3 refractedLight = refract(-sunDir, up, 1.0 / 1.333);

    float phase = ComputeMieScattering(-normalize(pView), refractedLight);

    float3 finalIntensity = pIntensity * float3(phase, phase, phase);
    finalIntensity *= exp(-length(pView) * (3.912 / visibility) * 8.0);

    finalIntensity *= sunColor;

    float4 color = float4(finalIntensity, 1.0);

    color = clamp(color, 0.0, 1.0);

    PSSceneOut output;
    output.color = color;
    return output;
}

#ifdef DX11
technique11 ColorTech {
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}
#endif

#ifdef DX10
technique10 ColorTech {
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
#endif

#ifdef DX10LEVEL9
technique10 ColorTech {
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0_level_9_1, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0_level_9_1, PS()));
    }
}
#endif

#ifdef DX9
technique {
    pass P0
    {
        SetVertexShader(CompileShader(vs_3_0, VS()));
#ifdef PS30
        SetPixelShader(CompileShader(ps_3_0, PS()));
#else
        SetPixelShader(CompileShader(ps_2_0, PS()));
#endif
    }
}
#endif
