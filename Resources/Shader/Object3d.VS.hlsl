#include "Object3d.hlsli"

//float4 main( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}

/// ******************************
/// 初めてのVertexShader
/// ******************************
struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput {
    float4 position : POSITION0; // float4
    float2 texcoord : TEXCOORD0; // float2
    float3 normal : NORMAL0; // float3
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcood = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    return output;
}