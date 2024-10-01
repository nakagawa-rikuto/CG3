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
};
