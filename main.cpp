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

#include "Function.h"
#include "CData.h"
#include "MyMath.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

/// *****************************************************
///　ReportLiveObjects
/// *****************************************************
struct D3DResourceLeakChecker {

	~D3DResourceLeakChecker() {

		// リソースリークチェック
		//IDXGIDebug1* debug;
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

//Windowsアプリケーションでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ReportLiveObjects
	D3DResourceLeakChecker leakCheck;

	/// *****************************************************
	/// COMの初期化
	/// *****************************************************
	CoInitializeEx(0, COINIT_MULTITHREADED);

#pragma region ///// ウィンドウの作成 /////

	/// *****************************************************
	/// ウィンドウを作ろう
	/// *****************************************************

	WNDCLASS wc{};

	// ウィンドウブロージャ
	wc.lpfnWndProc = WindowProc;

	// ウィンドウクラス名(何でも良い)
	wc.lpszClassName = L"CG2WindowClass";

	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウClassを登録する
	RegisterClass(&wc);

	// クライアント領域のサイズ 
	const int32_t kClientWindth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWindth, kClientHeight };

	// クライアントサイズを表す構造体にクライアント領域を入れる
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,         // 利用するクラス名
		L"CG2",                   // タイトルバーの文字(何でも良い)
		WS_OVERLAPPEDWINDOW,      // よく見るウィンドウスタイル
		CW_USEDEFAULT,            // 表示x座標(Windowsに任せる)
		CW_USEDEFAULT,            // 表示y座標(WindowsOSに任せる)
		wrc.right - wrc.left,     // ウィンドウ横幅
		wrc.bottom - wrc.top,     // ウィンドウ縦幅
		nullptr,                  // 縦ウィンドウハンドル
		nullptr,                  // メニューハンドル
		wc.hInstance,             // インスタンスハンドル
		nullptr                   // オプション
	);

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

#pragma endregion
	
#ifdef _DEBUG

	/// *****************************************************
	/// DebugLayer(デバッグレイヤー)
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController =
		CreateDebugController();
#endif

#pragma region ///// DirectX12の初期化 /////

	/// *****************************************************
	/// DirectX12を初期化しよう
	/// *****************************************************
	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	// HEWSULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判断できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでAssertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数、最初にNullptrを入れておく
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
		1, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr)); // 取得できないのは一大事

		//  ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(std::format(L"Use Adapater:{}\n", adapterDesc.Description));
			break;
		}

		useAdapter = nullptr;
	}

	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	// 昨日レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};

	const char* featrueLevelStrings[] = { "12.2", "12.1", "12.0" };

	//高い順に生成できるか試してみる
	for (size_t i = 0; i < _countof(featureLevels); ++i) {

		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		// 指定した機能レベルでデバイスが生成できたかを確認	
		if (SUCCEEDED(hr)) {
			//
			Log(std::format("FeatureLevel : {}\n", featrueLevelStrings[i]));
			break;
		}
	}

	//初期化完了のログを出す
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");

#pragma endregion
	
#pragma region ///// エラー・警告 /////
#ifdef _DEBUG

	/// *****************************************************
	/// エラー・警告、即ち停止
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 解放
		//infoQueue->Release();

		/// *****************************************************
		/// エラーと警告の抑制
		/// *****************************************************
		// 抑制するメッセージ
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			// https:/stackoverflow.com/questions/69805245/DirectX-12-application-is-crashing-in-windows11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
#endif
#pragma endregion

#pragma region ///// Command系の生成 /////

	/// *****************************************************
	/// CommandQueueを生成する
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue =
		CreateCommandQueue(hr, device.Get());

	/// *****************************************************
	///  CommandAllocatorを生成する
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator =
		CreateCommandAllocator(hr, device.Get());

	/// *****************************************************
	///  CommandListを生成する
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
		CreateCommandList(hr, device.Get(), commandAllocator.Get());

#pragma endregion

#pragma region ///// SwapChain /////

	/// *****************************************************
	///  SwapChainを生成する
	/// *****************************************************
	// スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWindth;      // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;     // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.SampleDesc.Count = 1;  // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;  // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // モニタに移したら、中身を破棄

	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

	/// *****************************************************
	///  SwapChainからResourceを引っ張ってくる
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	// うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

#pragma endregion 

#pragma region ///// Resourceの作成 /////
#pragma region ModelData
	/// *****************************************************
	/// ModelDataを使う
	/// *****************************************************
	// モデル読み込み
	ModelData modelData = LoadObjFile("Resources", "fence.obj");

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModel = CreateVertexResource(hr, device.Get(), sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファービューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress(); // リソースの先頭のアドレスから使う
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点サイズ
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData); // 1頂点サイズ

	// 頂点リソースにデータを書き込む
	VertexData* vertexDataModel = nullptr;
	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel)); // 書き込むためのアドレスを取得
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size()); // 頂点データをリソースにコピー

	/// *****************************************************
	/// Material(ModelData)用のResourceを作る
	/// *****************************************************
	// マテリアル(スフィア)用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceModel =
		CreateVertexResource(hr, device.Get(), sizeof(Material));

	// マテリアルにデータをModel書き込む
	Material* materialDataModel = nullptr;

	// 書き込むためのアドレスを取得
	materialResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&materialDataModel));

	// 色の書き込み
	materialDataModel->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// Lightingを有効化
	materialDataModel->enableLighting = true;

	// 単位行列で初期化
	materialDataModel->uvTransform = MakeIdenitiy4x4();

	/// *****************************************************
	/// TransformationMatrix(スフィア)用のResourceを作る
	/// *****************************************************
	// WVP(スフィア)用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceModel =
		CreateVertexResource(hr, device.Get(), sizeof(TransformationMatrix));

	// データを書き込む
	TransformationMatrix* wvpDataModel = nullptr;

	// 書き込むためのアドレスを取得
	wvpResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataModel));
#pragma endregion
#pragma region Sprite
	/// *****************************************************
	/// Sprite用のResourceの用意
	/// *****************************************************
	// スプライトの表示用Resource
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite =
		CreateVertexResource(hr, device.Get(), sizeof(VertexData) * 6);

	// スプライト用の頂点バッファービューを作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	// リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;

	// 1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	/// *****************************************************
	/// Material(スプライト)用のResourceを作る
	/// *****************************************************
	// Material(スプライト)用のResource
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite =
		CreateVertexResource(hr, device.Get(), sizeof(Material) * 3);

	// マテリアルにデータを書き込む
	Material* materialDataSprite = nullptr;

	// 書き込むためのアドレスを取得
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	// 色の書き込み
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// Lightingを有効化
	materialDataSprite->enableLighting = false;

	// 単位行列で初期化
	materialDataSprite->uvTransform = MakeIdenitiy4x4();

	/// *****************************************************
	/// Index(スプライト)のResourceの作成
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite =
		CreateVertexResource(hr, device.Get(), sizeof(uint32_t) * 6);

	// Viewの作成(IndexBufferView<IBV>)
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();

	// 使用するリソースのサイズはインデックス6つ分
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;

	// インデックスはUint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
#pragma endregion
#pragma region Sphere
	/// *****************************************************
	/// スフィア用のResourceの用意
	/// *****************************************************
	
	// スフィアの分割数
	const uint32_t kSubdivision = 32;

	// スフィアの表示用のResource
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere =
		CreateVertexResource(hr, device.Get(), sizeof(VertexData) * (kSubdivision * kSubdivision * 6));

	// スフィア用の頂点バッファービューを作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};

	// リソースの先頭のアドレスから使う
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();

	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * (kSubdivision * kSubdivision * 6);

	// 1頂点当たりのサイズ
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	/// *****************************************************
	/// Material(スフィア)用のResourceを作る
	/// *****************************************************
	// マテリアル(スフィア)用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSphere = 
		CreateVertexResource(hr, device.Get(), sizeof(Material));

	// マテリアルにデータを書き込む
	Material* materialDataSphere = nullptr;

	// 書き込むためのアドレスを取得
	materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));

	// 色の書き込み
	materialDataSphere->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// Lightingを有効化
	materialDataSphere->enableLighting = true;

	// 単位行列で初期化
	materialDataSphere->uvTransform = MakeIdenitiy4x4();

	/// *****************************************************
	/// Index(スフィア)のResourceの作成
	/// *****************************************************
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSphere =
		CreateVertexResource(hr, device.Get(), sizeof(uint32_t) * (kSubdivision * kSubdivision * 6));

	// Viewの作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};

	// リソースの先頭のアドレスから使う
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();

	// 使用するリソースのサイズはインデックス６つ分
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * (kSubdivision * kSubdivision * 6);

	// インデックスはUint32_tとする
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

	/// *****************************************************
	/// TransformationMatrix(スフィア)用のResourceを作る
	/// *****************************************************
	// WVP(スフィア)用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceSphere = 
		CreateVertexResource(hr, device.Get(), sizeof(TransformationMatrix));

	// データを書き込む
	TransformationMatrix* wvpDataSphere = nullptr;

	// 書き込むためのアドレスを取得
	wvpResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));
#pragma endregion

	/// *****************************************************
	/// 平行光源のResourceの作成
	/// *****************************************************
	// 平行光源のResource
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
		CreateVertexResource(hr, device.Get(), sizeof(DirectionalLight));

	// データを書き込む
	DirectionalLight* directionalLightData = nullptr;

	// 書き込むアドレスを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	/// *****************************************************
	/// depthStencilResourceの作成
	/// *****************************************************
	// DepthStencilTextureをウィンドウのサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource =
		CreateDepthStencilTetureResource(device.Get(), kClientWindth, kClientHeight);

	/// *****************************************************
	/// Transform周りを作る
	/// *****************************************************
	// Sprite用のTransformMatrix用のリソースを作る。
	Microsoft::WRL::ComPtr<ID3D12Resource> transformMatrixResourceSprite = 
		CreateVertexResource(hr, device.Get(), sizeof(TransformationMatrix));

	//データを書き込む
	TransformationMatrix* transformtionMatrixDataSprite = nullptr;

	// 書き込む為のアドレスを取得
	transformMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformtionMatrixDataSprite));

#pragma endregion

#pragma region ///// DescriptorHeap , RTV , SRV /////

	/// *****************************************************
	///  DescriptorHeapの生成
	/// *****************************************************
	// RTV用のディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = 
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// SRV用のディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = 
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// DSV用のヒープでディスクリプタの数は1。
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = 
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	/// *****************************************************
	///  DescriptorSizeの取得
	/// *****************************************************
	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	

	/// *****************************************************
	///  RTVの作成
	/// *****************************************************
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  // 2dテクスチャとして書き込む

	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	// まず1つ目を作る。1つ目は最初のところに作る。作る場所をコリらで指定してあげる必要がある
	rtvHandles[0] = GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 0);
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

	//2つ目のディスクリプタハンドルを得る(自力で)
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//2つ目を作る
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

	/// *****************************************************
	/// Textureの転送
	/// *****************************************************
	// Textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexTure("./Resources/fence.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device.Get(), metadata);
	UploadTextureData(textureResource.Get(), mipImages);

	// ２枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages2 = LoadTexTure("./Resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device.Get(), metadata2);
	UploadTextureData(textureResource2.Get(), mipImages2);

	/// *****************************************************
	///  SRVの作成
	/// *****************************************************
	// metadataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); 

	// metadata2を基に2個目のSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);

	// SRVの生成
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	/// *****************************************************
	///  DSVの作成
	/// *****************************************************
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture

	// DSVHeapの先頭にDSVをつくる
	device->CreateDepthStencilView(
		depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

#pragma endregion

#pragma region ///// Fence //////

	/// *****************************************************
	/// FenceとEventを生成する
	/// *****************************************************
	// 初期値0でFenceを作る{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

#pragma endregion

#pragma region ///// DXCの初期化 /////

	/// *****************************************************
	/// DXCの初期化
	/// *****************************************************
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

#pragma endregion

	// 現時点でIncludeはしないが、Includeに対応するための設定を行っておく
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

#pragma region ///// グラフィックスパイプライン //////

	/// ********************************************************************
	/// RootSignature
	/// ********************************************************************
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/// *****************************************************
	/// DescriptorRange
	/// *****************************************************
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始める
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// RootParameter作成。
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号0を使う

	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の高さ
	
	/// *****************************************************
	/// Samplerの設定
	/// *****************************************************
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	/// *******************************************************************
	/// InputLayout(インプットレイアウト)
	/// *******************************************************************
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	/// *******************************************************************
	/// VertexShader
	/// *******************************************************************
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShaderVertex(dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());

	/// *******************************************************************
	/// PixelShader
	/// *******************************************************************
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShaderPixel(dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());

	/// *********************************************************************
	/// PSO
	/// Pipeline State ObjectCreateVertexResource
	/// *********************************************************************
	// PSOの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();  // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;  // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize() }; // VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize() }; // PixelShader
	graphicsPipelineStateDesc.BlendState = CreateBlendState(); // BlendState
	graphicsPipelineStateDesc.RasterizerState = CreateRasterizerState(); // RasterizerState
	graphicsPipelineStateDesc.DepthStencilState = CreateDepthStencilDesc();

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 書き込むDSVの情報
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 実際に生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> grapicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&grapicsPipelineState));
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region ///// データの書き込み /////

	/// *****************************************************
	/// Resourceにデータを書き込む
	/// *****************************************************
	// 頂点リソースにだーたを書き込む
	//VertexData* vertexData = nullptr;
	VertexData* vertexDataSprite = nullptr;
	VertexData* vertexDataSphere = nullptr;

	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;
	uint32_t* indexDataSphere = nullptr;

	// 書き込むためのアドレスを取得
	vertexResourceSprite->Map(
		0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	vertexResourceSphere->Map(
		0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));
	indexResourceSprite->Map(
		0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexResourceSphere->Map(
		0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

	/* /////////////////////////
		      スプライト
	*/ /////////////////////////

	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };

	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;

	/// *****************************************************
	/// 平行光源をShaderで使う
	/// *****************************************************
	// デフォルト値はとりあえず以下のようにしておく
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	/// *****************************************************
	/// Transform情報を作る
	/// *****************************************************
	
	Transform transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform cameraTransform = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f, 0.0f, -10.0f} };
	Transform transformSprite = { {1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f } };
	Transform uvTransformSprite = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

#pragma endregion

#pragma region ///// Viewport ・ scissor /////

	/// *****************************************************
	/// ViewportとScissor(シザー)
	/// *****************************************************
	// Viewport
	D3D12_VIEWPORT viewport = 
		CreateViwport(kClientWindth, kClientHeight);

	// Scissor
	D3D12_RECT scissorRect = 
		CreateScissor(kClientWindth, kClientHeight);

#pragma endregion

#pragma region ///// ImGuiの初期化 /////

	/// *****************************************************
	/// ImGuiの初期化
	/// *****************************************************
	// ImGuiの初期化。
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

#pragma endregion

	/// *****************************************************
	/// SRVの切り替え
	/// *****************************************************
	bool useMonsterBall = true;

	/// *****************************************************
	/// メインループ
	/// *****************************************************
	MSG msg{};

	// ウィンドウのxボタンが押されるまでループ
	while (msg.message != WM_QUIT) {

		//Windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

			// フレームの先頭でImGuiに、ここからフレームが始まる旨を告げる
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			/// ******************************************************************
			/// ゲームの処理
			/// ******************************************************************
			
			// 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の初期に置き換える
			//ImGui::ShowDemoWindow();

#ifdef _DEBUG

			ImGui::Begin("Camera");
			ImGui::DragFloat3("Camera.rotate", &cameraTransform.rotate.x, 0.01f);
			ImGui::DragFloat3("Camera.scale", &cameraTransform.scale.x, 0.01f);
			ImGui::DragFloat3("Camera.translate", &cameraTransform.translate.x, 0.01f);
			ImGui::End();

			ImGui::Begin("Texture");
			ImGui::Checkbox("useMonsterBall", &useMonsterBall);
			ImGui::End();

			ImGui::Begin("info");
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
			ImGui::SliderAngle("SphereRotateX", &transform.rotate.x);
			ImGui::SliderAngle("SphereRotateY", &transform.rotate.y);
			ImGui::SliderAngle("SphereRotateZ", &transform.rotate.z);
			ImGui::ColorEdit4("Color", &materialDataModel->color.x);
			ImGui::ColorEdit4("LigthColor", &directionalLightData->color.x);
			ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f);
			ImGui::SliderAngle("LightIntensity", &directionalLightData->intensity);
			ImGui::End();

#endif // DEBUG

			// WorldMatrixを作る
			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);

			/// *****************************************************
			/// WorldViewProjectionMatrixを作る
			/// *****************************************************
			// WorldMatrixを作る
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 viewMatrixSprite = MakeIdenitiy4x4();
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWindth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 projectionMatrixSprite = MakeOrethographicMatrx(0.0f, 0.0f, float(kClientWindth), float(kClientHeight), 0.0f, 100.0f);

			// WVPMatrixを作る
			Matrix4x4 worldViewProjectionMatrix = Mutiply(worldMatrix, Mutiply(viewMatrix, projectionMatrix));
			Matrix4x4 worldViewProjectionMatrixSprite = Mutiply(worldMatrixSprite, Mutiply(viewMatrixSprite, projectionMatrixSprite));

			// CBufferの中身を更新する
			wvpDataModel->WVP = worldViewProjectionMatrix;
			wvpDataModel->World = MakeIdenitiy4x4();

			transformtionMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
			transformtionMatrixDataSprite->World = MakeIdenitiy4x4();

			/// *****************************************************
			/// コマンドを積み込んで確定させる
			/// *****************************************************
			// これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			/// *****************************************************
			/// TransitionBarrierを張るコード
			/// *****************************************************
			// TransitionBarrierの設定
			D3D12_RESOURCE_BARRIER barrier{};

			// 今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			// Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

			// バリアを張る対象のリソース。現在のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

			// 遷移前(現在)のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

			// 遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);
			
			// 描画先のRTVを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

			// 描画先のRTVとDSVを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
			
			// 指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };  // 青っぽい色。RGBAの順
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			
			/// *****************************************************
			/// ImGuiを
			/// *****************************************************
			// 描画用のDescriptorHeapの設定
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			/// *****************************************************
			/// コマンドを積む
			/// *****************************************************

			commandList->RSSetViewports(1, &viewport); // viewportを設定
			commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定

			// RootSignatureを設定.。PSOに設定しているけど別途設定が必要
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(grapicsPipelineState.Get()); // PSOを設定

			/// *****************************************************
			/// CBVを設定する
			/// *****************************************************

			// 形状を設定。
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			/* /////////////////////////
			        ModelDataの描画
			*/ ////////////////////////

			// マテリアルCBufferの場所設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceModel->GetGPUVirtualAddress());

			// VBVを設定
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);

			// 平行光源CBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

			// vertexBufferViewSphere用のBufferの場所設定
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceModel->GetGPUVirtualAddress());

			// ModelDataを使う
			//commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			//SRVのDescriptorTableの先頭を設定。Textureの貼り付け
			//useMonsterBallがTrueならTextureSRVHandleGPU2を貼り付け
			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			// ModelDataの数を利用する
			commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			/* /////////////////////////
					Spriteの描画
			*/ ////////////////////////

			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

			// VBVを設定
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

			// IBVを設定
			commandList->IASetIndexBuffer(&indexBufferViewSprite);

			// transformMatrixResourceSprite用のBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, transformMatrixResourceSprite->GetGPUVirtualAddress());

			// テクスチャの再設定
			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			// 描画!(DrawCall/ドローコール)
			//commandList->DrawInstanced(6, 1, 0, 0);
			//commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// ImGuiの内部コマンドを生成する
			ImGui::Render();

			// 実際のCommandListのImGuiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			/// *****************************************************
			/// 画面表示できるようにする
			/// *****************************************************
			// 画面に描く処理はすべて終わり、画面に移すので、状態を遷移
			// 今回はRenderTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);//}

			//コマンドリストの内容を確定させる。　すべてのコマンドを積んでからCloseすること
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			/// *****************************************************
			/// コマンドをキックする
			/// *****************************************************
			// GPUコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);

			/// *****************************************************
			/// GPUにSignalを送る
			/// *****************************************************
			// Fenceの値を更新
			fenceValue++;

			// GPUがここまでたどり着いたときに、Fenceの当た値を指定した値に代入するようにSignalを送る
			commandQueue->Signal(fence.Get(), fenceValue);

			/// *****************************************************
			/// Fenceの値を確認してGPUを持つ
			/// *****************************************************
			// Fenceの値が指定したSignal値にたどり着いているか確認する
			// GetCompletedValueの初期値はFence作成時に渡した初期値
			if (fence->GetCompletedValue() < fenceValue) {

				// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);

				// イベント待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			// 次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator.Get(), nullptr);
			assert(SUCCEEDED(hr));
		}
	}
	// ImGuiの終了処理.。
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	/// *****************************************************
	/// 解放処理
	/// *****************************************************
	CloseHandle(fenceEvent);
	CloseWindow(hwnd);

	CoUninitialize();

	return 0;
}