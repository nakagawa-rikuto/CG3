#include "Object3d.hlsli"

//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}

/// ******************************
/// 初めてのPixelShader
/// ******************************

struct Material {
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct DirectionalLight {
    float4 color;      // ライトの色
    float3 direction;  // ライトの向き
    float intensity;   // ライトの明るさ(輝度)
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixlShaderOutput {
    float4 color : SV_TARGET0;
};

//SRVのregisterはt
Texture2D<float4> gTexture : register(t0);

// Samplerのregisterはs
SamplerState gSampler : register(s0);


PixlShaderOutput main(VertexShaderOutput input) {
    PixlShaderOutput output;
    output.color = gMaterial.color;
    
    float4 transformdUV = mul(float4(input.texcood, 0.0f, 1.0f), gMaterial.uvTransform);
    
    //TextureをSamplingする
    float4 textureColor = gTexture.Sample(gSampler, transformdUV.xy);
    
    // Samplingしたtextureの色とmaterialの色を乗算して合成っしてみる
    output.color = gMaterial.color * textureColor;
    
    if (gMaterial.enableLighting != 0) { // Lightingする場合
        // N = normal, dot = dot(), L = Light
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;
    } else { // Lightingしない場合。前回までと同じ演算
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}