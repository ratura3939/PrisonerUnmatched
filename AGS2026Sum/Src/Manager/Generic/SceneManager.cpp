#include"../../pch.h"
#include "../../Common/Fader.h"
#include "../../Application.h"
#include "../../Scene/Main/Title.h"
#include"../GameSystem/CollisionManager.h"
#include"../Decoration/EffectManager.h"
#include"../Decoration/SoundManager.h"
#include"../Decoration/UIManager2d.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "SceneManager.h"

SceneManager* SceneManager::instance_ = nullptr;

void SceneManager::CreateInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new SceneManager();
	}
	instance_->Init();
}

SceneManager& SceneManager::GetInstance(void)
{
	return *instance_;
}

void SceneManager::Init(void)
{
	//エフェクト・サウンドの生成
	SoundManager::CreateInstance();
	EffectManager::CreateInstance();

	//UIマネージャの生成
	UIManager2d::CreateInstance();

	//判定
	CollisionManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::ALL_END);

	fader_ = new Fader();
	fader_->Init();

	// カメラ
	camera_ = std::make_shared<Camera>();
	camera_->Init();

	isSceneChanging_ = false;
	nextScene_ = nullptr;

	// デルタタイム
	preTime_ = std::chrono::system_clock::now();

	// 3D用の設定
	Init3D();

	// 初期シーンの設定
	SetInitScene(std::make_shared<Title>());

	// メインスクリーン
	Application& app = Application::GetInstance();
	mainScreen_ = MakeScreen(
		app.GetWindowWidth(), app.GetWindowHeight(), true);
}

void SceneManager::Init3D(void)
{

	// 背景色設定
	SetBackgroundColor(0, 0, 0);

	// Zバッファを有効にする
	SetUseZBuffer3D(true);

	// Zバッファへの書き込みを有効にする
	SetWriteZBuffer3D(true);

	// バックカリングを有効にする
	SetUseBackCulling(true);

	// ライトの設定
	SetUseLighting(false);

	// 正面から斜め下に向かったライト
	ChangeLightTypeDir(LIGHT_DIR);
}

void SceneManager::Update(void)
{

	if (scenes_.empty())
	{
		return;
	}

	// デルタタイム
	auto nowTime = std::chrono::system_clock::now();
	auto checkDelta = nowTime - preTime_;

	deltaTime_ = static_cast<float>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(nowTime - preTime_).count() / 1000000000.0);



	totalTime_ += deltaTime_;
	preTime_ = nowTime;

	fader_->Update();
	if (isSceneChanging_)
	{
		Fade();
	}
	else
	{
		// カメラ更新
		camera_->Update();
		//最新のシーンだけを更新
  		scenes_.back()->Update();
		SoundManager::GetInstance().Update();
		EffectManager::GetInstance().Update();
	}
}

void SceneManager::Draw(void)
{
	
	// 描画先グラフィック領域の指定
	// (３Ｄ描画で使用するカメラの設定などがリセットされる)
	SetDrawScreen(mainScreen_);

	// 画面を初期化
	ClearDrawScreen();

	//カメラ
	camera_->SetBeforeDraw();

	//エフェクシア更新
	UpdateEffekseer3D();

	//シーンの下層から順に描画
	for (auto& scene : scenes_) {
		scene->Draw();
	}

	//エフェクシア描画
	DrawEffekseer3D();

	// 暗転・明転
	fader_->Draw();

	// 背面スクリーンにメインスクリーンを描画
	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();
	DrawGraph(0, 0, mainScreen_, true);
}

void SceneManager::Destroy(void)
{
	//エフェクト・サウンドの削除
	SoundManager::GetInstance().Destroy();
	EffectManager::GetInstance().Destroy();
	UIManager2d::GetInstance().Destroy();
	//CollisionManager::GetInstance().DestroyInstance();

	scenes_.clear();

	DeleteGraph(mainScreen_);
	//delete scene_;

	delete fader_;

	camera_->Release();

	delete instance_;

}


void SceneManager::SetInitScene(std::shared_ptr<SceneBase> _scene)
{
	nextScene_ = _scene;
	DoChangeScene();
}

void SceneManager::ChangeScene(std::shared_ptr<SceneBase> _scene)
{
	nextScene_ = _scene;

	// フェードアウト(暗転)を開始する
	fader_->SetFade(Fader::STATE::FADE_OUT);
	isSceneChanging_ = true;
}

void SceneManager::PushScene(std::shared_ptr<SceneBase> _scene)
{
	_scene->Init();
	scenes_.push_back(_scene);
}

void SceneManager::PopScene(void)
{
	scenes_.back()->Release();
	scenes_.pop_back();
}


float SceneManager::GetDeltaTime(void) const
{
	//return 1.0f / 60.0f;
	return deltaTime_;
}

Camera& SceneManager::GetCamera(void) const
{
	return *camera_;
}

const SceneManager::CNTL& SceneManager::GetController(void) const
{
	return cntl_;
}

void SceneManager::SetController(const CNTL _cntl)
{
	cntl_ = _cntl;
}

void SceneManager::SwitchController(void)
{
	if (cntl_ == CNTL::KEY) {
		SetController(CNTL::PAD);
	}
	else if(cntl_==CNTL::PAD) {
		SetController(CNTL::KEY);
	}
}

void SceneManager::SetUpdateSpeedRate(const float _percent)
{
	updateSpeedRate_ = _percent / 100.0f;
}

const float SceneManager::GetUpdateSpeedRate(void) const
{
	return updateSpeedRate_;
}

const float SceneManager::GetUpdateSpeedRatePercent(void) const
{
	return updateSpeedRate_ * 100.0f;
}

const float SceneManager::GetScaleUpdateSpeedRate(const float _target) const
{
	return _target * updateSpeedRate_;
}

SceneManager::SceneManager(void)
{
	cntl_ = CNTL::NONE;

	mainScreen_ = -1;

	fader_ = nullptr;

	isSceneChanging_ = false;

	// デルタタイム
	deltaTime_ = 1.0f / 60.0f;
	totalTime_ = 0.0f;

	updateSpeedRate_ = 1.0f;

}

void SceneManager::ResetDeltaTime(void)
{
	deltaTime_ = 0.016f;
	preTime_ = std::chrono::system_clock::now();
}

void SceneManager::DoChangeScene(void)
{	
	//解放
	auto& resM = ResourceManager::GetInstance();
	auto& sndM = SoundManager::GetInstance();
	auto& efcM = EffectManager::GetInstance();
	auto& uiM = UIManager2d::GetInstance();
	auto& colM = CollisionManager::GetInstance();

	//解放
	for (auto& scene : scenes_) {
		scene->Release();
	}

	// リソースの全解放
	resM.Release();
	sndM.Release();
	efcM.Release();
	uiM.Relese();

	//リソース解放後,カメラのコライダーをを再度付与
	camera_->ResetCollider();

	//次のシーン初期化
	nextScene_->Init();

	
	scenes_.clear();

	//次のシーンを入れる
	scenes_.push_back(nextScene_);

	ResetDeltaTime();

	nextScene_ = nullptr;
}

void SceneManager::Fade(void)
{

	Fader::STATE fState = fader_->GetState();
	switch (fState)
	{
	case Fader::STATE::FADE_IN:
		// 明転中
		if (fader_->IsEnd())
		{
			// 明転が終了したら、フェード処理終了
			fader_->SetFade(Fader::STATE::NONE);
			isSceneChanging_ = false;
		}
		break;
	case Fader::STATE::FADE_OUT:
		// 暗転中
		if (fader_->IsEnd())
		{
			// 完全に暗転してからシーン遷移
			DoChangeScene();
			// 暗転から明転へ
			fader_->SetFade(Fader::STATE::FADE_IN);
		}
		break;
	}

}