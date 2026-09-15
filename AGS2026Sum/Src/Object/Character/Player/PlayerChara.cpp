#include "../../../pch.h"
#include"../../../Application.h"
#include"../../../Manager/Generic/SceneManager.h"
#include"../../../Manager/Generic/Camera.h"
#include"../../../Manager/Generic/ResourceManager.h"	
#include"../../../Manager/GameSystem/ShadowManager.h"	
#include"../../Common/Geometry/Sphere.h"
#include"../../../Renderer/ModelMaterial.h"
#include"../../../Renderer/ModelRenderer.h"
#include"PlayerManager.h"
#include "PlayerChara.h"

#include"../../../Manager/Generic/InputManager.h"	

namespace {
	const VECTOR INIT_POS = { 5000.0f,0.0f,1000.0f };	//初期位置

	const float MOVE_SPEED = 25.0f;	//移動速度
	const std::wstring ROOT_NAME = L"mixamorig8:Hips";
	const int HP_MAX = 50;	//体力最大
	const float RADIUS = 30.0f;
	const float BROUD_RADIUS = RADIUS + 15.0f;

	//シェーダ関連(後でキャラクターに共通化するかも？)
	const int PS_NORMAL_DEPTH_BUFF_SIZE = 1;	//ピクセルシェーダの定数バッファの数

	const int PS_CAMERA_RAY_BUFF_NUM = 1;	//バッファ番号

	const int BUFFER_MATRIX_SIZE = 2;	//マトリックスの大きさ

	const float OUTLINE_DEPTH_RANGE = 500.0f;	//アウトライン描画の深度範囲
}

PlayerChara::PlayerChara(void)
	:inputDir_(Utility::VECTOR_ZERO)
	,isMove_(false)
	,isAttack_(false)
	,moveSpeed_(MOVE_SPEED)
	,afterMoveRad_(0.0f)
	,onHit_(*this)
{
	hp_ = HP_MAX;
}

PlayerChara::~PlayerChara(void)
{
	//影の削除
	ShadowManager::GetInstance().SubShadowModel(ShadowManager::MESH_TYPE::SKINNED, modelId_);
}

void PlayerChara::PlayAnim(const std::wstring& _animName, const float _speed)
{
	animController_->Play(_animName, _speed);	//アニメーションの新規再生
}

void PlayerChara::ForcePlayAnim(const std::wstring& _animName, const float _speed)
{
	animController_->ForcePlay(_animName, _speed);	//アニメーションの強制再生
}

void PlayerChara::NoBlendPlayAnim(const std::wstring& _animName, const float _speed)
{
	animController_->NoBlendPlay(_animName, _speed);	//ブレンドなしの強制再生
}

void PlayerChara::GetAnimTotalTime(const std::wstring& _animName) const
{
	animController_->GetAnimTotalTime(_animName);	//アニメーションの総再生時間を取得
}

const float PlayerChara::GetCurrentAnimationProgressRate(void) const
{
	return animController_->GetCurrentAnimationProgressRate();
}

const float PlayerChara::GetBlendAnimationProgressRate(void) const
{
	return animController_->GetBlendAnimationProgressRate();
}

const float PlayerChara::GetSpecifiedAnimationProgressRate(const std::wstring& _animName) const
{
	return animController_->GetSpecifiedAnimationProgressRate(_animName);
}

void PlayerChara::DoLoad(void)
{
	modelId_ = ResourceManager::GetInstance().LoadModelDuplicate(ResourceManager::SRC::PLAYER_MDL);	//モデル取得

	outlineMaterial_ = std::make_unique<ModelMaterial>(L"SkinOutLineVS.cso", 2, L"SkinOutLinePS.cso", 1);
	outlineMaterial_->AddConstBufVS(FLOAT4{ 2.0f,0.0f,0.0f,0.0f });

	auto& camera = SceneManager::GetInstance().GetCamera();
	const VECTOR& cameraPos = camera.GetPos();
	const VECTOR& forward = camera.GetForward();
	outlineMaterial_->AddConstBufVS(FLOAT4{ cameraPos.x, cameraPos.y, cameraPos.z, 0.0f });
	outlineMaterial_->AddConstBufPS(FLOAT4{ 0.0f,0.0f,0.0f,1.0f });
	modelMaterial_ = std::make_unique<ModelMaterial>(L"SkinVS.cso", VS_SKIN_BUFF_SIZE,L"StdModelPS.cso", PS_SKIN_BUFF_SIZE, BUFFER_MATRIX_SIZE);	//モデルマテリアル生成
	modelMaterial_->AddConstBufPS(FLOAT4{ 1.0f,1.0f,1.0f,1.0f });
	modelMaterial_->AddConstBufPS(FLOAT4{ forward.x,forward.y,forward.z,1.0f });

	modelRenderer_ = std::make_unique<ModelRenderer>();	//モデルレンダラー生成

	//影マネージャー
	auto& shadow = ShadowManager::GetInstance();

	// マトリックスバッファーの追加
	modelMaterial_->AddConstBufVSMatrix(shadow.GetLightViewMatrix());
	modelMaterial_->AddConstBufVSMatrix(shadow.GetLightProjectionMatrix());

	//影を追加
	shadow.AddShadowModel(ShadowManager::MESH_TYPE::SKINNED, modelId_);

	//当たり判定の生成
	std::unique_ptr<Geometry> geo = std::make_unique<Sphere>(pos_, movedPos_, quaRot_, BROUD_RADIUS, RADIUS);
	MakeCollider(std::move(geo), Collider::COL_TAG::PLAYER, { Collider::COL_TAG::ENEMY,Collider::COL_TAG::ENEMY_ATTACK, Collider::COL_TAG::STAGE });
}

void PlayerChara::DoInit(void)
{
	//pos_ = Utility::VECTOR_ZERO;
	pos_ = INIT_POS;
	movedPos_ = pos_;	
	moveSpeed_ = MOVE_SPEED;
	inputDir_ = Utility::VECTOR_ZERO;

	InitAnim();	//アニメーションの初期化
}

void PlayerChara::DoUpdate(void)
{
	//アニメーション設定
	useAnim_ = PlayerManager::ANIM_IDLE;	//初期値は待機
	if (isMove_) {
		useAnim_ = PlayerManager::ANIM_RUN;	//走りアニメ
	}

	//攻撃中ではないとき
	if (!isAttack_) {
		Move();	//移動処理
		animController_->Play(useAnim_);	//アニメーションの新規再生
	}
	Rotation();
	inputDir_ = Utility::VECTOR_ZERO;
	animController_->Update();
}

void PlayerChara::InitAnim(void)
{
	ResourceManager& resM = ResourceManager::GetInstance();
	using SRC = ResourceManager::SRC;
	using ANIM_PLAY_TYPE = AnimationController::PLAY_TYPE;
	using ANIM_SOURCE = AnimationController::ANIM_SOURCE;

	animController_ = std::make_unique<AnimationController>(modelId_);

	//アニメーションの登録
	animController_->Add(PlayerManager::ANIM_IDLE, resM.Load(SRC::PLAYER_IDLE_ANIM).handleId_, ANIM_PLAY_TYPE::LOOP, ANIM_SOURCE::EXTERNAL);
	animController_->Add(PlayerManager::ANIM_RUN, resM.Load(SRC::PLAYER_RUN_ANIM).handleId_, ANIM_PLAY_TYPE::LOOP, ANIM_SOURCE::EXTERNAL);
	animController_->Add(PlayerManager::ANIM_DAMAGE, resM.Load(SRC::PLAYER_DAMAGE_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL);
	animController_->Add(PlayerManager::ANIM_FIRST_PUNCH, resM.Load(SRC::PLAYER_FIRST_PUNCH_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_SECOND_PUNCH, resM.Load(SRC::PLAYER_SECOND_PUNCH_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_THIRD_PUNCH, resM.Load(SRC::PLAYER_THIRD_PUNCH_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_MIDDLE_KICK, resM.Load(SRC::PLAYER_MIDDLE_KICK_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_HIGH_KICK, resM.Load(SRC::PLAYER_HIGH_KICK_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_FINSH_KICK, resM.Load(SRC::PLAYER_FINISH_KICK_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true, true);
	animController_->SetFixAnimationAxisInfo(PlayerManager::ANIM_FINSH_KICK, true, false, true);	//竜巻旋風脚の移動補正
	animController_->Add(PlayerManager::ANIM_SPECIAL_PUNCH, resM.Load(SRC::PLAYER_SPECIAL_PUNCH_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true,true);
	animController_->SetFixAnimationAxisInfo(PlayerManager::ANIM_SPECIAL_PUNCH, true, false, true);	//ジャンプ攻撃の移動補正
	animController_->Add(PlayerManager::ANIM_SPECIAL_KICK, resM.Load(SRC::PLAYER_SPECIAL_KICK_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_ULTIMATE, resM.Load(SRC::PLAYER_ULTIMET_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);
	animController_->Add(PlayerManager::ANIM_ULTIMATE_TEST, resM.Load(SRC::PLAYER_ULTIMET_TEST_ANIM).handleId_, ANIM_PLAY_TYPE::NORMAL, ANIM_SOURCE::EXTERNAL, true);

	animController_->SetRootFrameIndex(ROOT_NAME);	//親ボーン名

	animController_->SetDefaultAnim(PlayerManager::ANIM_IDLE);	//デフォルトを待機にする
	animController_->Play(PlayerManager::ANIM_IDLE);			//待機アニメ再生
}

void PlayerChara::Move(void)
{
	VECTOR moveVec = inputDir_;
	moveVec.y = 0.0f;	//Y軸方向の移動はなし

	movedPos_ = VAdd(movedPos_, VScale(moveVec, moveSpeed_*SceneManager::GetInstance().GetUpdateSpeedRate()));	//移動
	isMove_ = false;
}

void PlayerChara::Attack(void)
{
}

void PlayerChara::DrawHP(void)
{
	const int boxStart_X = 30;
	const int boxStart_Y = Application::GetInstance().GetWindowHeight() - 50;
	const int boxEnd_Y = boxStart_Y + 15;
	const int boxXDiff = 300;

	const int GLAY = 0x999999;
	const int RED = 0xff0000;
	const int GREEN = 0x00ff00;

	DrawBox(boxStart_X, boxStart_Y, boxStart_X + (boxXDiff * HP_MAX / HP_MAX), boxEnd_Y, GLAY, true);
	DrawBox(boxStart_X, boxStart_Y, boxStart_X + (boxXDiff * hp_ / HP_MAX), boxEnd_Y, GREEN, true);
}

void PlayerChara::Draw(void)
{
	const VECTOR& cameraPos = SceneManager::GetInstance().GetCamera().GetPos();
	//DrawFormatString(10, 30, 0xffffff, L"PlayerPos: %f, %f, %f,\nInputDir: %f, %f, %f\nCameraPos: %f, %f, %f", pos_.x, pos_.y, pos_.z, inputDir_.x, inputDir_.y, inputDir_.z, cameraPos.x, cameraPos.y, cameraPos.z);

	//描画
	outlineMaterial_->SetConstBufVS(1,FLOAT4{ cameraPos.x, cameraPos.y, cameraPos.z, 0.0f });

	//影マネージャー
	auto& shadow = ShadowManager::GetInstance();

	auto& camera = SceneManager::GetInstance().GetCamera();
	const VECTOR& forward = camera.GetForward();
	modelMaterial_->SetConstBufPS(1, FLOAT4{ forward.x,forward.y,forward.z,0.0f });

	// マトリックスバッファーの設定
	modelMaterial_->SetConstBufVSMatrix(0, shadow.GetLightViewMatrix());
	modelMaterial_->SetConstBufVSMatrix(1, shadow.GetLightProjectionMatrix());

	// シャドウマップの設定
	modelMaterial_->SetTextureBuf(ModelRenderer::CONSTANT_BUF_SLOT_BEGIN_VS_MATRIX, shadow.GetShadowTexture());
	
	//アウトライン用描画
	MV1SetWriteZBuffer(modelId_, false);//モデル描画のZBufferを無効にする
	MV1SetMeshBackCulling(modelId_, 0, DX_CULLING_RIGHT);	//裏面描画
	modelRenderer_->Draw(modelId_, *outlineMaterial_);

	//本体描画
	MV1SetWriteZBuffer(modelId_, true);
	MV1SetMeshBackCulling(modelId_, 0, DX_CULLING_LEFT);	//表面描画
	modelRenderer_->Draw(modelId_, *modelMaterial_);

	//HP描画
	DrawHP();
}

void PlayerChara::Release(void)
{
}

void PlayerChara::HitCollider(std::weak_ptr<Collider> _col)
{
	//当たり判定
	onHit_.HitCollider(_col);
}

void PlayerChara::InputMoveVec(const VECTOR& _inputVec)
{
	//攻撃中は移動入力を受け付けない
	if (isAttack_)return;

#pragma region INPUT->MOVE
	// 入力がほぼゼロの場合
	if (abs(_inputVec.x) < 0.01f && abs(_inputVec.y) < 0.01f) {
		return;		//処理の必要なし
	}

	isMove_ = true;	//移動している
	auto& camera = SceneManager::GetInstance().GetCamera();
	
	VECTOR camForward = camera.GetRot().GetForward();	//カメラ前方向（Y軸方向を無視）
	camForward.y = 0.0f;
	camForward = VNorm(camForward);

	VECTOR camRight = camera.GetRot().GetRight();		//カメラ右方向（Y軸方向を無視）
	camRight.y = 0.0f;
	camRight = VNorm(camRight);

	inputDir_ = VAdd(									//入力方向をワールド座標に変換
		VScale(camForward, _inputVec.y),				//y: 前後（前が正）、x: 左右（右が正）
		VScale(camRight, _inputVec.x)
	);

	inputDir_ = VNorm(inputDir_);	//正規化
#pragma endregion

#pragma region モデルの角度設定
	afterMoveRad_ = 0.0f;
	afterMoveRad_ = atan2f(_inputVec.x, _inputVec.y);	//角度設定（モデルの角度は目標として設定する時にカメラを考慮するので、ここでは入力のみをみる）
	SetGoalRot(afterMoveRad_);	//回転目標角度を設定
#pragma endregion
}
