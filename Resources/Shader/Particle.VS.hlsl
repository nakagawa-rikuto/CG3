#include "Particle.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0; // float4
    float2 texcoord : TEXCOORD0; // float2
    float3 normal : NORMAL0; // float3
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) //
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrices[instanceId].WVP);
    output.texcood = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices[instanceId].World));
    return output;
}