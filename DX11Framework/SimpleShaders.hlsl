Texture2D diffuseTex : register(t0);

SamplerState bilinearSampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 DiffuseLight;
    float4 DiffuseMaterial;
    float3 LightDir;
    float Count;
    float4 ambientColour;
    float4 ambientLighting;
    float4 specularLight;
    float4 specularMaterial;
    float3 cameraPosition;
    float specPower;
    int hasTexture;
    
}

struct VS_Out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float3 PosW : POSITION0;
    float3 WorldVertexNormal : wvNormal;
    float2 TexCoord : TEXCOORD;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD)
{   
    VS_Out output = (VS_Out)0;
    
    float4 Pos4 = float4(Position.x, Position.y + sin(Count), Position.z, 1.0f);
    output.position = mul(Pos4, World);
    output.PosW = output.position;
    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);
    
    float3 NormalW = mul(float4(Normal, 0), World).xyz;    
    
    output.WorldVertexNormal = NormalW;
    
    output.TexCoord = TexCoord;
    
    output.normal = Normal;
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    float3 NormalDir = normalize(input.WorldVertexNormal);
    float DiffuseAmount = saturate(dot(LightDir, NormalDir));
    
    float4 texColor = diffuseTex.Sample(bilinearSampler, input.TexCoord);
    
    ////Specular lighting Stuff
    float3 reflectedDir = reflect(-LightDir, NormalDir);
    float3 ObjectToCamera = normalize(cameraPosition - input.PosW);
    float SpecularDot = saturate(dot(ObjectToCamera, reflectedDir));
    //float3 SpecularIntensity = reflectedDir * ObjectToCamera;
    
    float SpecularIntensity = pow(SpecularDot, specPower);
    
    float4 TotalColour;
    
    if (hasTexture == 1)
    {
        float4 PotentialDiffuse = DiffuseLight * texColor;
        TotalColour = (PotentialDiffuse * DiffuseAmount);
        TotalColour += (texColor * ambientLighting);
        TotalColour += SpecularIntensity * (specularLight * specularMaterial);

    }
    else
    {
        float4 PotentialDiffuse = DiffuseLight * DiffuseMaterial;
        TotalColour = (PotentialDiffuse * DiffuseAmount);
        TotalColour += (ambientColour * ambientLighting);
        TotalColour += SpecularIntensity * (specularLight * specularMaterial);
    }

    
    input.color = TotalColour;
    
    return input.color;
}
//float3 noramlizedDir = normalizer(LightDir);
//float d = dot(normalize, normalizedDir);
//float intensity = saturate(-1);
//float reflectedDir = reflect(LightDir, norm);
//float power = pow(2, 10);