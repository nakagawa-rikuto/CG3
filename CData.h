#pragma once
#include "MyMath.h"
#include <vector>

/// *****************************************************
///　頂点データの拡張
/// *****************************************************
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

/// *****************************************************
/// Transform情報を作る
/// *****************************************************
struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

/// *****************************************************
///　マテリアルを拡張
/// *****************************************************
struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

///-------------------------------------------/// 
/// マテリアルデータ
///-------------------------------------------///
struct MaterialData {
	std::string textureFilePath;
};

/// *****************************************************
///　TransformationMatrixを拡張
/// *****************************************************
struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

/// *****************************************************
///　平行光源を拡張
/// *****************************************************
struct DirectionalLight {
	Vector4 color;     // ライトの色
	Vector3 direction; // ライトの向き
	float intensity;   // ライトの明るさ(輝度)
};

/// *****************************************************
///　ModelDataの構造体
/// *****************************************************
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

/// *****************************************************
///　BlendMode
/// *****************************************************
enum BlendMode {
	//!< ブレンドなし
	kBlendModeNone,

	//!< 通常ブレンド。
	KBlendModeNormal,

	//!< 加算
	kBlendModeAdd,

	//!< 減算
	kBlendModeSubtract,

	//!< 乗算
	kBlendModeMultily,

	//!< スクリーン
	kBlendModeScreen,

	// 利用しない
	kCountOfBlendMode,
};

///-------------------------------------------/// 
/// ShaderType
///-------------------------------------------///
enum ShaderType {
	Object3d, // Objec3D
	Particle, // Particle
};

///=====================================================/// 
/// Particleの構造体
///=====================================================///
struct ParticleData {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

// インスタンス数
const uint32_t kNumMaxInstance = 10; 

///-------------------------------------------/// 
/// エミッタ
///-------------------------------------------///
struct Emitter {
	Transform transform;
	uint32_t count;
	float frequency;
	float frequencyTime;
};

///-------------------------------------------/// 
/// Field
///-------------------------------------------///
struct AccelerationField {
	Vector3 acceleration;
	AABB area;
};