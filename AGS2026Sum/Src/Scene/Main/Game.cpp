#include"../../pch.h"
#include"../../Object/Character/Enemy/EnemyManager.h"
#include"../../Manager/GameSystem/ShadowManager.h"
#include"../../Manager/GameSystem/GravityManager.h"
#include"../../Manager/GameSystem/AttackManager.h"
#include"../../Manager/GameSystem/CollisionManager.h"
#include"../../Manager/GameSystem/ChunkManager.h"
#include"../../Manager/GameSystem/ComboManager.h"
#include"../../Manager/GameSystem/Mission/MissionManager.h"
#include"../../Manager/GameSystem/Mission/MissionKillTarget.h"
#include"../../Manager/GameSystem/Event/EventManager.h"
#include"../../Manager/Generic/Camera.h"
#include"../../Manager/Generic/SceneManager.h"
#include"../../Manager/Generic/InputManager.h"
#include"../../Manager/Generic/ResourceManager.h"
#include"../../Manager/Decoration/SoundManager.h"
#include"../../Manager/Decoration/EffectManager.h"
#include"../../Manager/Decoration/UIManager2d.h"
#include"../../Object/Character/Player/PlayerManager.h"
#include"../../Object/Stage/StageManager.h"
#include "../../Scene/Sub/PauseScene.h"
#include"../../Utility/Utility.h"
#include"../../Renderer/PixelMaterial.h"
#include"../../Renderer/PixelRenderer.h"
#include"../../Application.h"
#include "Title.h"
#include "GameClear.h"
#include "GameOver.h"
#include "Game.h"

//ローカル定数
namespace {
	constexpr VECTOR CAMERA_START_1 = { 600.0f,200.0f,0.0f };	//カメラ演出開始位置
	constexpr VECTOR CAMERA_GOAL_1 = { 600.0f,1000.0f,0.0f };	//カメラ演出目標位置その①
	constexpr VECTOR CAMERA_GOAL_2 = { 0.0f,800.0f,600.0f };	//カメラ演出目標位置その②
	constexpr float ALLOWABLE_DISTANCE = 10.0f;		//カメラの移動完了判定をがば目にするために
	constexpr int BOSS_IDX = 0;		//ボスの配列番号(ボス単体のため必ず0)


	const int LIMIT_SLOW = 120;					//スロー演出時間
	const int BGM_VOL_MAX = 100;				//BGM音量最大値
	const int BGM_VOL_ACC = 1;					//BGM切り換えスピード
	const float NORMAL_SPEED_PERCENT = 100.0f;	//通常の割合
	const float SLOW_SPEED_PERCENT = 10.0f;		//スローの割合
		  
	const int WARNING_DIRECTION_TIME = 150;		//WARNING警告時間
	const int CAMERA_SHAKE_NUM = 3;				//カメラ演出における振動回数
	const int CAMERA_SHAKE_COOL_TIME = 40;		//振動のクールタイム

	const float CAMERA_FOLLOW_DIFF_Y_ABILITY = 200.0f;	//能力使用時の注視点差分

	const float LOCK_DISTANCE_MIN_NOMAL = 500.0f;		//ロックオン時に最低限離れておく距離
	const float LOCK_DISTANCE_MIN_BOSS = 1000.0f;		//ロックオン時に最低限離れておく距離

	const int BGM_VOL = 80;

	const int PS_EDGE_BUFF_NUM = 2;	//エッジ描画用のバッファ数
	const float EDGE_NORMAL_THRESHOLD = 0.7f;	//エッジ描画用の閾値
	const float EDGE_DEPTH_THRESHOLD = 0.7f;	//エッジ描画用の閾値

	const int BGM_VOLUME_PERCENT = 90;	//BGM音量

	//現在の進行度
	std::array<std::string, static_cast<int>(Game::GAME_PROGRESS::MAX)> PROGRESS =
	{
		"Tutorial"
		,"Stage1"
	};
}

Game::Game(void)
{
	isSlowEffect_ = false;
	slowCnt_ = -1;
	nextBgmVol_ = 0;
	switchBgm_ = false;

	prevInputP_ = false;
	isEnemyUpdate_ = true;

	cameraMoveStartPos_ = CAMERA_START_1;
	cameraMoveGoalPos_[0] = CAMERA_GOAL_1;
	cameraMoveGoalPos_[1] = CAMERA_GOAL_2;
	direcState_ = BOSS_DIRECTION::NONE;
	cameraShakeCollTimeCnt_ = 0;
	stayCameraShake_ = false;

	progress_ = GAME_PROGRESS::TUTORIAL;

	isDrawPostEffect_ = false;
	direcCnt_ = 0;

	update_ = &Game::GameUpdate;

	cameraGoalStayCounter_ = 0;
	cameraGoalStayTime_ = 0;

	isDebug_ = false;

	processingAfterCameraAutoMove_ = nullptr;
}

Game::~Game(void)
{
}

void Game::Init(void)
{
	//リソース準備
	ResourceManager& rsM = ResourceManager::GetInstance();
	rsM.GetInstance().Init(SceneManager::SCENE_ID::GAME);

	update_ = &Game::GameUpdate;

	//進行度
	progress_ = GAME_PROGRESS::TUTORIAL;

	//更新
	updateProgress_ = &Game::UpdateTutorial;

	//生成

	//影マネージャー
	ShadowManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);

	//重力マネージャー
	GravityManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);

	//攻撃マネージャー
	AttackManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);
	
	//チャンク管理
	ChunkManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);

	//コンボ管理
	ComboManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);

	//イベントマネージャ
	EventManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);

	//目的管理
	MissionManager::CreateInstance(SingletonRegistry::DESTROY_TIMING::GAME_END);
	auto& misMng = MissionManager::GetInstance();

	//プレイヤー
	player_ = std::make_unique<PlayerManager>(*this);
	player_->Init();

	//ステージごとの敵情報
	stageEnemyData_ = rsM.Load(ResourceManager::SRC::STAGE_ENEMY_DATA).GetData<StageEnemyData>();

	//敵
	enemy_ = std::make_unique<EnemyManager>(player_->GetPos());
	enemy_->Load();
	enemy_->Init();
	enemy_->CreateStageEnemy(stageEnemyData_.allStageInfo[PROGRESS[static_cast<int>(progress_)]]);

	//ステージ
	stage_ = std::make_unique<StageManager>();
	stage_->Load();
	stage_->Init();

	//目的設定
	std::unique_ptr<MissionKillTarget> mission = std::make_unique<MissionKillTarget>();
	mission->SetExplanText(L"Alexを倒せ！！");
	mission->SetTargetNum(stageEnemyData_.allStageInfo[PROGRESS[static_cast<int>(progress_)]].bossInfos.size());
	mission->SetTargetType(ENEMY_TYPE::MIDDLE_BOSS);
	misMng.SetMission(std::move(mission));

	//カメラの初期設定
	Camera& camera = SceneManager::GetInstance().GetCamera();
	camera.SetDefault();	//初期化
	camera.ChangeMode(Camera::MODE::FOLLOW);					//モード選択
	camera.SetFollow(player_->GetPos(), player_->GetQua());		//追従対象
	camera.SetGoalFocusPos(player_->GetFocusPos());				//注視点
	camera.SetLockOnDistanceMin(LOCK_DISTANCE_MIN_NOMAL);		//ロックオン最低距離

	//音関係初期設定
	InitSound();
	//エフェクト関係初期化
	InitEffect();
	//シェーダー初期化
	InitShader();
}

void Game::SetProcessingAfterCameraAutoMove(const CAMERA_MOVE_SITUATION& _situation)
{
	//状況に応じた処理と紐づけ
	switch (_situation) {
	case CAMERA_MOVE_SITUATION::ULTIMATE:
		processingAfterCameraAutoMove_ = nullptr;
		break;
	case CAMERA_MOVE_SITUATION::NEXT_STAGE:
		processingAfterCameraAutoMove_ = &Game::StartNextStage;
		break;
	}
}

void Game::InitSound(void)
{
	ResourceManager& rsM = ResourceManager::GetInstance();
	SoundManager& sndM = SoundManager::GetInstance();

	using SND_TYPE = SoundManager::TYPE;
	using SND_NAME = SoundManager::SOUND_NAME;

	//BGM
	sndM.Add(SND_TYPE::BGM, SND_NAME::GAME_BGM,
		rsM.Load(ResourceManager::SRC::GAME_BGM).handleId_);

	sndM.AdjustVolume(SND_NAME::GAME_BGM, BGM_VOLUME_PERCENT);

	sndM.Play(SND_NAME::GAME_BGM);
}

void Game::InitEffect(void)
{
	ResourceManager& rsM = ResourceManager::GetInstance();
	EffectManager& efcM = EffectManager::GetInstance();

	//剣
	//efcM.Add(EffectManager::EFFECT_NAME, rsM.Load(ResourceManager::SRC::SWORD_EFC).handleId_);

	//エフェクト
	efcM.Add(EffectManager::EFFECT_NAME::ENEMY_HIT, rsM.Load(ResourceManager::SRC::ENEMY_HIT_EFC).handleId_);
	efcM.Add(EffectManager::EFFECT_NAME::ENEMY_DEAD, rsM.Load(ResourceManager::SRC::ENEMY_DEAD_EFC).handleId_);
	efcM.Add(EffectManager::EFFECT_NAME::ENEMY_TACKLE, rsM.Load(ResourceManager::SRC::ENEMY_TACKLE_EFC).handleId_);
	efcM.Add(EffectManager::EFFECT_NAME::ENEMY_LANDING, rsM.Load(ResourceManager::SRC::ENEMY_LANDING_EFC).handleId_);
	efcM.Add(EffectManager::EFFECT_NAME::ENEMY_AURA, rsM.Load(ResourceManager::SRC::ENEMY_AURA_EFC).handleId_);
	efcM.Add(EffectManager::EFFECT_NAME::PLAYER_HIT, rsM.Load(ResourceManager::SRC::PLAYER_HIT_EFC).handleId_);
}

void Game::InitShader(void)
{
	Application& app = Application::GetInstance();
	int screemWidth = app.GetWindowWidth();
	int screemHeight = app.GetWindowHeight();

	normalDepthScreen_ = MakeScreen(screemWidth, screemHeight, true);

	edgeMaterial_ = std::make_unique<PixelMaterial>(L"EdgeDetectPS.cso", PS_EDGE_BUFF_NUM);
	edgeMaterial_->AddConstBuf(FLOAT4{ 0.0f,0.0f,0.0f,1.0f });	//エッジの色
	edgeMaterial_->AddConstBuf(FLOAT4{ 1.0f / static_cast<float>(screemWidth),1.0f / static_cast<float>(screemHeight),EDGE_DEPTH_THRESHOLD,EDGE_NORMAL_THRESHOLD });	//エッジの色
	edgeMaterial_->AddTextureBuf(normalDepthScreen_);	//法線・深度描画用スクリーンをテクスチャとして登録


	edgeRender_ = std::make_unique<PixelRenderer>();
	edgeRender_->MakeSquereVertex({ 0,0 }, { screemWidth, screemHeight });
}

void Game::Update(void)
{
	SceneManager& scM = SceneManager::GetInstance();
	SoundManager& sndM = SoundManager::GetInstance();
	Camera& camera = scM.GetCamera();
	InputManager& inpM = InputManager::GetInstance();

#pragma region シーン遷移(ルール)

	//if(enemy_->GetActiveEnemyNum() <= 0)
	//{
	//	SceneManager::GetInstance().ChangeScene(std::make_shared<GameClear>());
	//}
	if (!player_->IsAlive())
	{
		SceneManager::GetInstance().ChangeScene(std::make_shared<GameOver>());
	}

#pragma endregion

	//更新
	(this->*update_)();

	//カメラ状態
	float cameraPosToGoalDiff = Utility::MagnitudeF(VSub(camera.GetGoalPos(), camera.GetPos()));	//現在位置と目標位置までの距離

	//一定の距離以内だったら
	if (cameraPosToGoalDiff <= ALLOWABLE_DISTANCE) {
		cameraGoalStayCounter_++;

		//一定時間経過していたら
		if (cameraGoalStayCounter_ >= cameraGoalStayTime_) {
			//フォローに変化
			camera.ChangeMode(Camera::MODE::FOLLOW);		//追従に変更(リセット後のモード変更用)
			camera.SetFocusPos(player_->GetFocusPos());		//注視点
			EndSlow();		//スロー演出終了
			cameraGoalStayCounter_ = 0;

			camera.ChangeMode(Camera::MODE::RESET);	//カメラリセット

			//個別演出
			if (processingAfterCameraAutoMove_ != nullptr) {
				(this->*processingAfterCameraAutoMove_)();
			}
		}
	}
}

void Game::GameUpdate(void)
{
	SceneManager& scM = SceneManager::GetInstance();
	SoundManager& sndM = SoundManager::GetInstance();
	InputManager& inpM = InputManager::GetInstance();
	Camera& camera = scM.GetCamera();

#pragma region 基礎アプデ
	stage_->Update();

	//プレイヤー
	player_->Update();
	//敵はスローの効果を受ける
	if (isSlowEffect_) {
		//スロー時の更新(このカウンタはスローの影響を受けない)
		slowCnt_++;
		if (slowCnt_ >= LIMIT_SLOW) {
			EndSlow();

			//特殊攻撃選択中の可能性を考えて
			player_->DisappearAttackCommandInfo();
		}
	}

	//敵
	if (isEnemyUpdate_) {
		enemy_->Update();
	}

	//必殺技の設定
	player_->SetIsUltimateReady(enemy_->IsGuardBreak());

	//ガードブレイクしていなければ
	if (enemy_->IsGuardBreak() == false) {
		//必殺技ボタン説明を消去
		player_->DisappearUltimateCommandInfo();
	}

	//当たり判定更新
	CollisionManager::GetInstance().UpdateColliders();

	//コンボの更新
	ComboManager::GetInstance().Update();

	//目的更新
	MissionManager::GetInstance().Update();

	//進行度の更新
	(this->*updateProgress_)();

#pragma endregion

#pragma region BGM

	//BGM切り換え実行中
	if (switchBgm_) {
		//音量調整に加算
		nextBgmVol_ += BGM_VOL_ACC;
		//sndM.AdjustVolume(switchBgmStr_(SOUND_NAME), nextBgmVol_);			//次のBGMは音量をあげる
		//sndM.AdjustVolume(nowBgmStr_(SOUND_NAME), (BGM_VOL_MAX - nextBgmVol_));	//現在のBGMは音量を下げる

		//もしボリュームが最大値以上なら
		if (nextBgmVol_ >= BGM_VOL_MAX) {
			//音量を最大値に
			nextBgmVol_ = BGM_VOL_MAX;	
			//終了処理
			FinishSwitchBgm();
		}
	}
#pragma endregion

#pragma region カメラ

	camera.SetFollow(player_->GetFocusPos(), player_->GetQua());		//追従対象の更新
	
#pragma endregion
}


void Game::DrawEdge(void)
{
	int mainScreen = SceneManager::GetInstance().GetMainScreen();

	//法線・深度描画用スクリーンに描画
	SetDrawScreen(normalDepthScreen_);

	ClearDrawScreen();

	SceneManager::GetInstance().GetCamera().SetBeforeDraw();
	
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	player_->DrawNormalDepth();

	// メインに戻す
	SetDrawScreen(mainScreen);

	edgeMaterial_->SetTextureBuf(0, normalDepthScreen_);
	edgeRender_->Draw(*edgeMaterial_);
}

void Game::UpdateTutorial(void)
{
	//条件キーを持っている敵がいなくなったら
	auto& evMng = EventManager::GetInstance();
	if (evMng.IsPlayEvent(EVENT_TYPE::OPEN_TUTORIAL_DOOR))
	{
		//ドアを開ける
		stage_->OpenDoor();

		//カメラの調整
		Camera& camera = SceneManager::GetInstance().GetCamera();
		camera.ChangeMode(Camera::MODE::AUTO_MOVE);
		SetCameraStayTimeAtAutoMove(120.0f);
		camera.SetGoalPos(stage_->GetGoalPosAtDoorOpen());
		camera.SetFocusPos(stage_->GetDoorPos());

		//個別処理の設定
		SetProcessingAfterCameraAutoMove(CAMERA_MOVE_SITUATION::NEXT_STAGE);
	}
}

void Game::UpdateStage1(void)
{
	//条件キーを持っている敵がいなくなったら
	auto& evMng = EventManager::GetInstance();
	if (evMng.IsPlayEvent(EVENT_TYPE::GAME_CLEAR))
	{
		//ゲームクリア
		SceneManager::GetInstance().ChangeScene(std::make_shared<GameClear>());
	}
}

void Game::Draw(void)
{
	DrawString(10, 10, L"GameScene", 0xffffff);

	ShadowManager::GetInstance().Draw();
	stage_->Draw();
	ChunkManager::GetInstance().DebugDraw();
	enemy_->Draw();
	player_->Draw();
	ComboManager::GetInstance().Draw();
	MissionManager::GetInstance().Draw();

	//デバッグ
	DrawDebug();

	//DrawEdge();
}

void Game::DrawUI(void)
{
	player_->DrawUI();
}

void Game::Release(void)
{
	player_->Release();
	enemy_->Release();
	SoundManager& sndM = SoundManager::GetInstance();
	sndM.StopAll(SoundManager::SOUND_NAME::GAME_BGM);	//今まで流していたものを停止

	SingletonRegistry::GetInstance().Delete(SingletonRegistry::DESTROY_TIMING::GAME_END);	//シングルトンの削除
	CollisionManager::GetInstance().DeleteAllCollider();
}

void Game::Reset(void)
{
	SoundManager& sndM = SoundManager::GetInstance();
	sndM.AdjustVolume(SoundManager::TYPE::BGM, BGM_VOL);	//前シーンに戻るのでBGMの音量を復活
	//とりあえずメニューからの復帰時は追従に
	//メニュー開く直前に変える可能性大
	SceneManager::GetInstance().GetCamera().ChangeMode(Camera::MODE::FOLLOW);
}

void Game::StartBossFaze(void)
{
	SoundManager& sndM = SoundManager::GetInstance();
	ChangeActionDirec(ACTION_DIRECTION::SCAN_LINE);

	//sndM.Stop("NomalBgm");	//今まで流していたものを停止
	//sndM.Stop("BattleBgm");	//今まで流していたものを停止
	//sndM.Play("WarningBgm");//警告音流す

	//演出初期設定
	direcState_ = BOSS_DIRECTION::POST_EFFECT;
	SceneManager::GetInstance().GetCamera().ChangeMode(Camera::MODE::FIXED_POINT);	//演出中はカメラ操作を受け付けない
}

void Game::ChangeActionDirec(const ACTION_DIRECTION _direc)
{
	//とりあえずポストエフェクトを描画するように
	isDrawPostEffect_ = true;

}

void Game::FinishSwitchBgm(void)
{
	SoundManager& sndM = SoundManager::GetInstance();
	//切り換え終了
	switchBgm_ = false;
	//sndM.AdjustVolume(switchBgmStr_, nextBgmVol_);
	////今まで流していたものを停止
	//sndM.Stop(nowBgmStr_);	
	//現在のBGM名と切り替え後のBGM名の切り換え
	auto ret = nowBgmStr_;
	nowBgmStr_ = switchBgmStr_;
	switchBgmStr_ = ret;
	//初期化
	nextBgmVol_ = 0;
}

void Game::StartNextStage(void)
{
	//終了判定をステージ１に移行
	updateProgress_ = &Game::UpdateStage1;
	progress_ = GAME_PROGRESS::STAGE_1;

	//敵生成
	enemy_->CreateStageEnemy(stageEnemyData_.allStageInfo[PROGRESS[static_cast<int>(progress_)]]);

	//目的設定
	auto& misMng = MissionManager::GetInstance();
	std::unique_ptr<MissionKillTarget> mission = std::make_unique<MissionKillTarget>();
	mission->SetExplanText(L"Alex兄弟を倒せ！！");
	mission->SetTargetNum(stageEnemyData_.allStageInfo[PROGRESS[static_cast<int>(progress_)]].bossInfos.size());
	mission->SetTargetType(ENEMY_TYPE::MIDDLE_BOSS);
	misMng.SetMission(std::move(mission));
}

void Game::StartSlow(void)
{
	if (isSlowEffect_)return;

	auto& scM = SceneManager::GetInstance();
	//スロー演出準備
	slowCnt_ = 0;
	
	//更新スピードを50％に設定
	scM.SetUpdateSpeedRate(SLOW_SPEED_PERCENT);
	//敵もそれに対応
	enemy_->SetAnimSpeedPercent(scM.GetUpdateSpeedRatePercent());

	//プレイヤーのアニメーションも調整
	player_->SetAnimSpeedPercent(scM.GetUpdateSpeedRatePercent());
	player_->SetIsSpecialReady(true);

	isSlowEffect_ = true;
}

void Game::EndSlow(void)
{
	if (!isSlowEffect_)return;

	auto& scM = SceneManager::GetInstance();
	isSlowEffect_ = false;
	//ChangeActionDirec(ACTION_DIRECTION::NOMAL);
	//更新処理を100％にもどす
	scM.SetUpdateSpeedRate(NORMAL_SPEED_PERCENT);
	enemy_->SetAnimSpeedPercent(scM.GetUpdateSpeedRatePercent());
	player_->SetAnimSpeedPercent(scM.GetUpdateSpeedRatePercent());
	player_->SetIsSpecialReady(false);
}


void Game::DrawDebug(void)
{
#ifdef _DEBUG

	//SceneManager::GetInstance().GetCamera().DrawDebug();F
	//if (isSlowEffect_) {
	//	DrawString(0, 140, "NOW_SLOW", 0xffffff);
	//}
	//enemy_->DrawDebug();
	//atkMng_->DrawDebug();
	//stage_->DrawDebug();
	//player_->DrawDebug();

	// シャドウマップを描画
	constexpr int MAPSIZE = 256;
	DrawExtendGraph(
		Application::SCREEN_SIZE_X - MAPSIZE,
		Application::SCREEN_SIZE_Y - MAPSIZE,
		Application::SCREEN_SIZE_X,
		Application::SCREEN_SIZE_Y,
		ShadowManager::GetInstance().GetShadowTexture(),
		false);

#endif // _DEBUG
}
