#include"pch.h"
#include<Windows.h>
#include "Manager/Generic/ResourceManager.h"
#include "Manager/Generic/InputManager.h"
#include "Manager/Generic/SceneManager.h"
#include "Application.h"

Application* Application::instance_ = nullptr;

const std::wstring Application::PATH_IMAGE = L"Data/Image/";
const std::wstring Application::PATH_UI = L"Data/Image/UI/";
const std::wstring Application::PATH_MODEL = L"Data/Model/";
const std::wstring Application::PATH_ANIMATION = L"Data/Animation/";
const std::wstring Application::PATH_CHARACTER = L"Data/Model/Character/";
const std::wstring Application::PATH_STAGE = L"Data/Model/Stage/";
const std::wstring Application::PATH_EFFECT = L"Data/Effect/";
const std::wstring Application::PATH_BGM = L"Data/Sound/BGM/";
const std::wstring Application::PATH_SE = L"Data/Sound/SE/";
const std::wstring Application::PATH_SHADER = L"Data/Shader/";
const std::wstring Application::PATH_JSON = L"Data/Json/";

void Application::CreateInstance(void)
{
	if (instance_ == nullptr)
	{
		instance_ = new Application();
	}
	instance_->Init();
}

Application& Application::GetInstance(void)
{
	return *instance_;
}

void Application::Init(void)
{

	// アプリケーションの初期設定
	SetWindowText(L"囚人無双");


	// 実行中ウィンドウがあるモニターを取得
	HMONITOR hMonitor = MonitorFromWindow(GetMainWindowHandle(), MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	// モニターの幅・高さ
	width_ = (mi.rcMonitor.right - mi.rcMonitor.left) / 2;
	height_ = (mi.rcMonitor.bottom - mi.rcMonitor.top) / 2;

	// ウィンドウサイズ
	SetGraphMode(width_, height_, 32);
	ChangeWindowMode(false);

	// DxLibの初期化
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	isInitFail_ = false;
	if (DxLib_Init() == -1)
	{
		isInitFail_ = true;
		return;
	}
	//エフェクシア初期化
	if (Effekseer_Init(8000) == -1)
	{
		DxLib_End();
	}
	SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
	Effekseer_SetGraphicsDeviceLostCallbackFunctions();

	//マウスカーソル非表示
	SetMouseDispFlag(FALSE);

	// キー制御初期化
	SetUseDirectInputFlag(true);
	InputManager::CreateInstance();

	// リソース管理初期化
	ResourceManager::CreateInstance();

	// シーン管理初期化
	SceneManager::CreateInstance();

	//FPS用初期化
	currentFrame_ = 0;
	lastFrame_ = 0;

}

void Application::Run(void)
{

	auto& inputManager = InputManager::GetInstance();
	auto& sceneManager = SceneManager::GetInstance();

	// ゲームループ
	while (isLoop_ && ProcessMessage() == 0)
	{

		Sleep(1);	//システムに処理を返す
		currentFrame_ = GetNowCount();	//現在のフレーム数を獲得

		//現在のフレームと最後の実行フレームの差分が一定値を超えたら更新処理を行う。
		if (currentFrame_ - lastFrame_ >= FRAME_RATE)
		{
			lastFrame_ = currentFrame_;	//フレームの更新
			inputManager.Update();
			sceneManager.Update();	//更新
		}

		sceneManager.Draw();

		ScreenFlip();

	}
}

void Application::Destroy(void)
{

	InputManager::GetInstance().Destroy();
	ResourceManager::GetInstance().Destroy();
	SceneManager::GetInstance().Destroy();
	
	// Effekseerを終了する。
	Effkseer_End();
	// DxLib終了
	if (DxLib_End() == -1)
	{
		isReleaseFail_ = true;
	}

	delete instance_;

}

bool Application::IsInitFail(void) const
{
	return isInitFail_;
}

bool Application::IsReleaseFail(void) const
{
	return isReleaseFail_;
}

void Application::EndGame(void)
{
	isLoop_ = false;
}

Application::Application(void)
{
	isInitFail_ = false;
	isReleaseFail_ = false;
	isLoop_ = true;
}
