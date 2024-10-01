#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <wrl.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <vector>
#include <fstream>
#include <sstream>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"

#include "CData.h"
#include "MyMath.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")



#pragma region ///// 関数 /////

/// *****************************************************
/// Log関数
/// *****************************************************
// 出力ウィンドウに文字を出す
void Log(const std::string& message) {

	OutputDebugStringA(message.c_str());
}

void Log(const std::wstring& message) {

	OutputDebugStringW(message.c_str());
}

/// *****************************************************
/// ウィンドウブロージャー
/// *****************************************************
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	//メッセージに応じたゲーム固有の処理を行う
	switch (msg) {

		//ウィンドウが破棄された
	case WM_DESTROY:

		//osに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

/// *****************************************************
/// string->wstring
/// *****************************************************
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

/// *****************************************************
/// wstring->string
/// *****************************************************
std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

// *****************************************************
/// DebugLayer(デバッグレイヤー)の関数
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12Debug1> CreateDebugController() {

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if ((SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))) {

		// デバッグレイヤーを有効化
		debugController->EnableDebugLayer();

		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

	return debugController;
}

/// *****************************************************
/// CommandQueueを生成する為の関数
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(HRESULT hr, ID3D12Device* device) {

	// コマンドキューを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(
		&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));

	return commandQueue;
}

/// *****************************************************
/// CommandAllocatorを生成する為の関数
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(HRESULT hr, ID3D12Device* device) {

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	return commandAllocator;
}

/// *****************************************************
/// CommandListを生成する為の関数
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(HRESULT hr, ID3D12Device* device, ID3D12CommandAllocator* commandAllocator) {

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	return commandList;
}

/// *****************************************************
/// CompileShader関数
/// *****************************************************
Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
	// CompilerするShaderファイルへのパス
	const std::wstring& filePath,

	// Compilerに使用するProfile
	const wchar_t* profile,

	// 初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {

	/* 1. hlslファイルを読み込む */
	// これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

	// hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	// 読めなかったら止める
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

	/* 2. Compileする */
	LPCWSTR arguments[] = {
		filePath.c_str(), // コンパイル対象のhlslファイル名
		L"-E", L"main",   // エントリーポイントの指定。基本的にmain以外にはしない
		L"-T", profile,   // ShaderProfileの設定
		L"-Zi", L"-Qembed_debug",   // デバッグ用の情報を埋め込む
		L"-Od",    // 最適化を外しておく
		L"-Zpr",   // メモリレイアウトは行優先
	};

	// 実際にShaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,          // 読み込んだファイル
		arguments,                    // コンパイルオプション
		_countof(arguments),          // コンパイルオプションの数
		includeHandler,               // includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)   // コンパイル結果
	);

	// コンパイルエラーではなくDxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	/* 3. 警告・エラーが出ていないかを確認する */
	// 警告・エラーが出てたらログを出して止める
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());

		// 警告・エラーダメゼッタイ
		assert(false);
	}

	/* 4. Compile結果を受け取ってます */
	// コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

	// 実行用のバイナリを返却
	return shaderBlob;
}

/// *****************************************************
/// BlendState(ブレンドステート)
/// *****************************************************
D3D12_BLEND_DESC CreateBlendState() {
	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

/// *****************************************************
/// RasterizerState(ラスタライザステート)
/// *****************************************************
D3D12_RASTERIZER_DESC CreateRasterizerState() {
	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	return rasterizerDesc;
}

/// *****************************************************
/// ShaderをCompileする(Vertex)
/// *****************************************************
Microsoft::WRL::ComPtr<IDxcBlob> CompileShaderVertex(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	return vertexShaderBlob;
}

/// *****************************************************
/// ShaderをCompileする(Pixel)
/// *****************************************************
Microsoft::WRL::ComPtr<IDxcBlob> CompileShaderPixel(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	return pixelShaderBlob;
}

/// *****************************************************
/// VertexResourceを生成する
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12Resource> CreateVertexResource(HRESULT hr, ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;  // UploadHeapを使う

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes; // リソースのサイズ。今回はVector4を3頂点分

	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際に頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

/// *****************************************************
/// DepthStencilStateの作成
/// *****************************************************
D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc() {

	// DepthStencilDescの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthの機能を有効化
	depthStencilDesc.DepthEnable = true;

	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	// 比較関数はLessEqual。
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	return depthStencilDesc;
}

/// *****************************************************
/// ViwPort
/// *****************************************************
D3D12_VIEWPORT CreateViwport(const int32_t kClientWindth, const int32_t kClientHeight) {
	// ビューポート
	D3D12_VIEWPORT viewport{};

	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = float(kClientWindth);
	viewport.Height = float(kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	return viewport;
}

/// *****************************************************
/// Scissor(シザー)
/// *****************************************************
D3D12_RECT CreateScissor(const int32_t kClientWindth, const int32_t kClientHeight) {
	// シザー矩形
	D3D12_RECT scissorRect{};

	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWindth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	return scissorRect;
}

/// *****************************************************
/// DescriptorHeapの作成関数
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

/// *****************************************************
/// Textureデータを読む
/// *****************************************************
DirectX::ScratchImage LoadTexTure(const std::string& filePath) {

	// テクスチャファイルを読み込んでプログラムで扱えるよにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

/// *****************************************************
/// TextureResourceの作成
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {

	/// ***************************
	/// metadataを基にResourceの設定
	/// ***************************
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行 or 配列Textureの配列数
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは２次元

	/// ***************************
	/// 利用するHeapの設定
	/// ***************************
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

	/// ***************************
	/// Resourceの生成
	/// ***************************
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読むだけ
		nullptr, // Clear最適値。
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

/// *****************************************************
/// データを転送するUploadTextureData関数の作成
/// *****************************************************
void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {

	// Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {

		// MipMapLevelを指定して各Imageを取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			UINT(img->rowPitch),  // １ラインサイズ
			UINT(img->slicePitch) // １枚サイズ
		);
		assert(SUCCEEDED(hr));
	}
}

/// *****************************************************
/// DepthStencilTextureの作成
/// *****************************************************
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTetureResource(ID3D12Device* device, int32_t width, int32_t height) {

	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width; // Textureの幅
	resourceDesc.Height = height; // Textureの高さ
	resourceDesc.MipLevels = 1; // mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行 or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // HEapの特殊な設定
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
		&depthClearValue, // Clear最適値
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

/// *****************************************************
/// GetCPUDescriptorHandleの作成
/// *****************************************************
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);

	return handleCPU;
}

/// *****************************************************
/// GetGPUDescriptorHandleの作成
/// *****************************************************
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);

	return handleGPU;
}

/// *****************************************************
///　Objectファイルを読む関数
/// *****************************************************
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	// 1.中で必要となる変数の宣言 //
	ModelData modelData; // 構築するModelData
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ１行を格納するもの

	// 2.ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3. 実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		// 頂点情報を得る
		// 頂点位置
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
			// 頂点テクスチャ座標
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
			// 頂点法線
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);

			// 三角形を作る
			// 面
		} else if (identifier == "f") {

			VertexData triangle[3];

			// 面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点の要素へのIndexは[位置/UV/法線]で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}

				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				//position.x *= -1.0f; // 位置の反転
				position.y *= -1.0f;
				//normal.x *= -1.0f; // 法線の反転
				normal.y *= -1.0f;
				VertexData vertex = { position, texcoord, normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position, texcoord, normal };
			}

			// 頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
	}

	// 4. ModelDataを返す
	return modelData;

	//ModelData modelData; // 構築するModelData
	//std::vector<Vector4> positions; // 位置
	//std::vector<Vector3> normals; // 法線
	//std::vector<Vector2> texcoords; // テクスチャ座標
	//std::string line; // ファイルから読んだ１行を格納するもの

	//// 2.ファイルを開く
	//std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	//assert(file.is_open()); // 開けなかったら停止する

	//// 3. 実際にファイルを読み、ModelDataを構築していく
	//while (std::getline(file, line)) {
	//	std::string identifier; // 行の先頭にある識別子
	//	std::istringstream s(line);
	//	s >> identifier; // 先頭の識別子を読む

	//	// 頂点情報を得る
	//	if (identifier == "v") { // 頂点位置
	//		Vector4 position;
	//		s >> position.x >> position.y >> position.z;
	//		position.w = 1.0f;
	//		positions.push_back(position);
	//	} else if (identifier == "vt") { // 頂点テクスチャ座標
	//		Vector2 texcoord;
	//		s >> texcoord.x >> texcoord.y;
	//		texcoords.push_back(texcoord);
	//	} else if (identifier == "vn") { // 頂点法線
	//		Vector3 normal;
	//		s >> normal.x >> normal.y >> normal.z;
	//		normals.push_back(normal);
	//	} else if (identifier == "f") { // 面
	//		VertexData triangle[3];

	//		// 面は三角形限定。その他は未対応
	//		for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
	//			std::string vertexDefinition; // 頂点定義
	//			s >> vertexDefinition;

	//			// 頂点の要素へのIndexは[位置/UV/法線]で格納されているので、分解してIndexを取得する
	//			std::istringstream v(vertexDefinition);
	//			uint32_t elementIndices[3] = { 0, 0, 0 };
	//			for (int32_t element = 0; element < 3; ++element) {
	//				std::string index;
	//				if (std::getline(v, index, '/')) {
	//					if (!index.empty()) {
	//						elementIndices[element] = std::stoi(index);
	//					}
	//				}
	//			}

	//			// インデックスの範囲をチェックして、有効範囲内ならばその値を使用、範囲外ならデフォルト値を使用
	//			Vector4 position = (elementIndices[0] > 0 && elementIndices[0] <= positions.size()) ? positions[elementIndices[0] - 1] : Vector4{ 0, 0, 0, 1 };
	//			Vector2 texcoord = (elementIndices[1] > 0 && elementIndices[1] <= texcoords.size()) ? texcoords[elementIndices[1] - 1] : Vector2{ 0, 0 };
	//			Vector3 normal = (elementIndices[2] > 0 && elementIndices[2] <= normals.size()) ? normals[elementIndices[2] - 1] : Vector3{ 0, 0, 1 };

	//			// 位置と法線のz座標を反転する
	//			//position.z *= -1.0f;
	//			//normal.z *= -1.0f;
	//			VertexData vertex = { position, texcoord, normal };
	//			triangle[faceVertex] = vertex;
	//		}

	//		// 頂点を逆順で登録することで、周り順を逆にする
	//		modelData.vertices.push_back(triangle[2]);
	//		modelData.vertices.push_back(triangle[1]);
	//		modelData.vertices.push_back(triangle[0]);
	//	}
	//}

	//// 4. ModelDataを返す
	//return modelData;
}

/// <summary>
/// WVPの作成関数
/// </summary>
/// <param name="transform"></param>
/// <param name="cameraMatrix"></param>
/// <param name="kClientWindth"></param>
/// <param name="kClientHeight"></param>
/// <returns></returns>
Matrix4x4 MakeWVPMatrix(Transform transform, Matrix4x4 cameraMatrix, const int32_t kClientWindth, const int32_t kClientHeight) {

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWindth) / float(kClientHeight), 0.1f, 100.0f);

	// wvp
	Matrix4x4 wvpMatrix = Mutiply(worldMatrix, Mutiply(viewMatrix, projectionMatrix));

	return wvpMatrix;
}

Matrix4x4 MakeUVMatrix(Transform transform) {

	Matrix4x4 UVTransformMatrix = MakeScalseMatrix(transform.scale);
	UVTransformMatrix = Mutiply(UVTransformMatrix, MakeRotateZMatrix(transform.rotate.z));
	UVTransformMatrix = Mutiply(UVTransformMatrix, MakeTranslateMatrix(transform.translate));

	return UVTransformMatrix;
}
