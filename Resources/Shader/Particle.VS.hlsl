#include "Particle.hlsli"

struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0; // float4
    float2 texcoord : TEXCOORD0; // float2
    float3 normal : NORMAL0; // float3
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) //
{
    VertexShaderOutput output;
    output.position = mul(input.position, gParticle[instanceId].WVP);
    output.texcood = input.texcoord;
    output.color = gParticle[instanceId].color;
    return output;
}