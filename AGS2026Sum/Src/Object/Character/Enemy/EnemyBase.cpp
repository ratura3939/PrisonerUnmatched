#include "../../../pch.h"
#include "../../../Manager/Generic/SceneManager.h"
#include "../../../Manager/GameSystem/ShadowManager.h"
#include "../../../Manager/GameSystem/CollisionManager.h"
#include "../../../Manager/GameSystem/AttackManager.h"
#include "../../../Manager/GameSystem/GravityManager.h"
#include "../../../Manager/GameSystem/Event/EventManager.h"
#include "../../../Manager/GameSystem/Mission/MissionManager.h"
#include "../../Common/Collider.h"
#include "../../Common/Geometry/Sphere.h"
#include"../../../Renderer/ModelMaterial.h"
#include"../../../Renderer/ModelRenderer.h"
#include "State/EnemyStateBase.h"
#include "State/EnemyNormalState.h"
#include "State/EnemyEndState.h"
#include "Skill/EnemySkillBase.h"
#include "Brain/EnemyBrain.h"
#include "OnHit/EnemyOnHit.h"
#include "EnemyManager.h"
#include "EnemyGroup.h"
#include "EnemyBase.h"

EnemyBase::EnemyBase(const ENEMY_TYPE& _type)
	: group_(nullptr)
	, activeIndex_(-1)
	, type_(_type)
	, attackPos_(Utility::VECTOR_ZERO)
	, action_(ENEMY_ACTION::STAY)
	, state_(nullptr)
	, isElementSkill_(false)
	, movePow_(Utility::VECTOR_ZERO)
	, gravityPow_(Utility::VECTOR_ZERO)
	, eventKey_(EVENT_TYPE::NONE)
	, color_({0.0f,0.0f,0.0f,0.0f})
{	
	//行動ごとの処理の設定
	actionFunc_[static_cast<int>(ENEMY_ACTION::STAY)] = { &EnemyBase::EnterStay, &EnemyBase::UpdateStay, &EnemyBase::ExitStay };
	actionFunc_[static_cast<int>(ENEMY_ACTION::MOVE)] = { &EnemyBase::EnterMove, &EnemyBase::UpdateMove, &EnemyBase::ExitMove };
	actionFunc_[static_cast<int>(ENEMY_ACTION::ALERT)] = { &EnemyBase::EnterAlert, &EnemyBase::UpdateAlert, &EnemyBase::ExitAlert };
	actionFunc_[static_cast<int>(ENEMY_ACTION::ATTACK_READY)] = { &EnemyBase::EnterAttackReady, &EnemyBase::UpdateAttackReady, &EnemyBase::ExitAttackReady };
	actionFunc_[static_cast<int>(ENEMY_ACTION::ATTACK)] = { &EnemyBase::EnterAttack, &EnemyBase::UpdateAttack, &EnemyBase::ExitAttack };
	actionFunc_[static_cast<int>(ENEMY_ACTION::ATTACK_END)] = { &EnemyBase::EnterAttackEnd, &EnemyBase::UpdateAttackEnd, &EnemyBase::ExitAttackEnd };
	actionFunc_[static_cast<int>(ENEMY_ACTION::RETURN_GROUP)] = { &EnemyBase::EnterReturn, &EnemyBase::UpdateReturn, &EnemyBase::ExitReturn };

	//行動ごとの影響情報の設定
	actionInfo_[static_cast<int>(ENEMY_ACTION::STAY)] = { .isLock = false, .canMove = false };
	actionInfo_[static_cast<int>(ENEMY_ACTION::MOVE)] = { .isLock = false, .canMove = true };
	actionInfo_[static_cast<int>(ENEMY_ACTION::ALERT)] = { .isLock = false, .canMove = true };
	actionInfo_[static_cast<int>(ENEMY_ACTION::ATTACK_READY)] = { .isLock = true, .canMove = false };
	actionInfo_[static_cast<int>(ENEMY_ACTION::ATTACK)] = { .isLock = true, .canMove = false };
	actionInfo_[static_cast<int>(ENEMY_ACTION::ATTACK_END)] = { .isLock = true, .canMove = false };
	actionInfo_[static_cast<int>(ENEMY_ACTION::RETURN_GROUP)] = { .isLock = false, .canMove = true };
}

EnemyBase::~EnemyBase(void)
{
	group_ = nullptr;
}

void EnemyBase::Draw(void)
{
	//デバッグ描画
	//DrawDebug();

	//モデル描画
	//MV1DrawModel(modelId_);

	//影マネージャー
	auto& shadow = ShadowManager::GetInstance();

	// マトリックスバッファーの設定
	modelMaterial_->SetConstBufVSMatrix(0, shadow.GetLightViewMatrix());
	modelMaterial_->SetConstBufVSMatrix(1, shadow.GetLightProjectionMatrix());

	// シャドウマップの設定
	modelMaterial_->SetTextureBuf(ModelRenderer::CONSTANT_BUF_SLOT_BEGIN_VS_MATRIX, shadow.GetShadowTexture());

	//モデルの描画
	modelMaterial_->SetConstBufPS(0, color_);
	modelRenderer_->Draw(modelId_,*modelMaterial_);
}

void EnemyBase::Release(void)
{
}

void EnemyBase::HitCollider(std::weak_ptr<Collider> _col)
{
	//当たり判定の処理
	onHit_->HitCollider(_col);
}

void EnemyBase::ChangeState(std::unique_ptr<EnemyStateBase> _nextState)
{
	//状態抜けの処理
	if(state_) state_->Exit(*this);

	//状態の変更
	state_ = std::move(_nextState);

	//状態遷移の処理
	state_->Enter(*this);
}

void EnemyBase::ChangeAction(const ENEMY_ACTION _nextAction)
{
	//状態が同じなら処理しない
	if (action_ == _nextAction || _nextAction == ENEMY_ACTION::MAX)return;

	//状態抜けの処理
	if (action_ != ENEMY_ACTION::MAX)(this->*actionFunc_[static_cast<int>(action_)].exit)();

	//状態の変更
	action_ = _nextAction;
	
	//状態遷移の処理
	(this->*actionFunc_[static_cast<int>(action_)].enter)();
}

void EnemyBase::SetSkills(std::vector<std::unique_ptr<EnemySkillBase>> _skills)
{
	skills_ = std::move(_skills);
}

void EnemyBase::SetAttackCollider(const AttackManager::ATTACK_TYPE& _name)
{
	//本来空で来ることはないが例外があった時のため
	if (colliders_.empty())CreateCollider();

	//攻撃マネージャー
	auto& atkMng = AttackManager::GetInstance();

	//攻撃マネージャーに自身の名前とスキルから持ってきたデータを伝える
	atkMng.AddAttackCollider(_name, colliders_[1]);

	//攻撃のヒット情報をリセット
	atkMng.ResetTargetColList(colliders_[1]);
}

void EnemyBase::RemoveAttackCollider(void)
{
	//本来空で来ることはないが例外があった時のため
	if (colliders_.empty())CreateCollider();

	//攻撃マネージャーから攻撃コライダを破棄
	AttackManager::GetInstance().DeleteAttackCollider(colliders_[1]);
}

void EnemyBase::SetCurrentSkill(EnemySkillBase* _skill)
{
	//既にスキルを使っている
	if (currentSkill_)return;

	//保存
	currentSkill_ = _skill;

	//攻撃準備
	ChangeAction(ENEMY_ACTION::ATTACK_READY);
}

void EnemyBase::RemoveCurrentSkill(void)
{
	//ないならスキップ
	if (!currentSkill_)return;

	//強制終了
	currentSkill_->EndEnter(*this);

	//攻撃コライダの登録削除
	RemoveAttackCollider();
}

void EnemyBase::UpdateBrain(void)
{
	//思考の更新
	if (!actionInfo_[static_cast<int>(action_)].isLock)
	{
		//優先度決定
		brain_->DecidePriority();

		//行動選択
		brain_->ChoiceAction();
	}
}

void EnemyBase::UpdateAction(void)
{
	//状態ごとの更新
	(this->*actionFunc_[static_cast<int>(action_)].update)();

	//移動処理
	if (actionInfo_[static_cast<int>(action_)].canMove)Move();
}

void EnemyBase::ResetPos(void)
{
	//グループがないなら処理しない
	if(!group_) return;
	
	//グループ座標の取得
	VECTOR groupPos = group_->GetInitPos();
	VECTOR leavePos = { LEAVE_GROUP_DIST, 0.0f, LEAVE_GROUP_DIST };
	VECTOR randPos = Utility::GetRandomValue(VScale(leavePos, -1.0f), leavePos);

	//座標のリセット
	pos_ = VAdd(groupPos, randPos);
	movedPos_ = pos_;
}

void EnemyBase::StateEnd(void)
{
	//最後の状態の終了
	if (state_) state_->Exit(*this);

	//既に終了しているなら終わり
	if (state_->GetStateId() == ENEMY_STATE::END)return;

	//途中で強制終了しているなら終了処理をする
	state_ = std::make_unique<EnemyEndState>();

	//終了
	state_->Enter(*this);
	state_->Exit(*this);
}

void EnemyBase::DoLoad(void)
{
	//思考の初期化
	brain_ = std::make_unique<EnemyBrain>(*this);

	//接触処理の初期化
	onHit_ = std::make_unique<EnemyOnHit>(*this);

	//シェーダーの読み込み
	LoadShader();
}

void EnemyBase::LoadShader(void)
{
	//モデルマテリアル生成
	modelMaterial_ = std::make_unique<ModelMaterial>(L"SkinVS.cso", VS_SKIN_BUFF_SIZE, L"StdModelPS.cso", PS_C_BUFF_SIZE, BUFFER_MATRIX_SIZE);
	modelMaterial_->AddConstBufPS(color_);

	//影マネージャー
	auto& shadow = ShadowManager::GetInstance();

	// マトリックスバッファーの追加
	modelMaterial_->AddConstBufVSMatrix(shadow.GetLightViewMatrix());
	modelMaterial_->AddConstBufVSMatrix(shadow.GetLightProjectionMatrix());

	// シャドウマップの設定
	modelMaterial_->SetTextureBuf(ModelRenderer::CONSTANT_BUF_SLOT_BEGIN_VS_MATRIX, shadow.GetShadowTexture());

	//モデルレンダラー生成
	modelRenderer_ = std::make_unique<ModelRenderer>();
}

void EnemyBase::DoInit(void)
{
	//ローカル回転
	quaRotLocal_ = Quaternion();

	//状態の初期化
	state_ = std::make_unique<EnemyNormalState>();
	state_->Enter(*this);

	//色の初期化
	color_ = { 0.0f,0.0f,0.0f,0.0f };

	//アニメーション初期化
	InitAnim();
}

void EnemyBase::InitWithGroup(void)
{
	//グループがないなら処理しない
	if (!group_)return;

	//座標
	ResetPos();
}

void EnemyBase::InitRunTimeParameter(const EnemyParameter& _param)
{
	//体力の初期化
	hp_ = static_cast<float>(_param.initHp);
	hpMax_ = static_cast<float>(_param.initHp);
}

void EnemyBase::SetModel(const int _modelId)
{
	modelId_ = _modelId;
}

void EnemyBase::SetAnim(std::unique_ptr<AnimationController> _anim)
{
	animController_ = std::move(_anim);
}

void EnemyBase::CreateCollider(void)
{
	//コライダの初期化
	DeleteCollider();

	//当たり判定の生成
	std::unique_ptr<Geometry> geo = std::make_unique<Sphere>(pos_, movedPos_, quaRot_, BROUD_RADIUS, RADIUS);
	MakeCollider(
		std::move(geo)
		, Collider::COL_TAG::ENEMY
		, { Collider::COL_TAG::PLAYER, Collider::COL_TAG::PLAYER_ATTACK ,Collider::COL_TAG::ENEMY,Collider::COL_TAG::STAGE 
		});

	//攻撃コライダ
	geo = std::make_unique<Sphere>(attackPos_, attackPos_, quaRot_, ATTACK_BROUD_RADIUS, ATTACK_RADIUS);
	MakeCollider(std::move(geo), Collider::COL_TAG::ENEMY_ATTACK, { Collider::COL_TAG::PLAYER });
	DisableColliderAtTag(Collider::COL_TAG::ENEMY_ATTACK);
}

void EnemyBase::DeleteCollider(void)
{
	//攻撃マネージャーに攻撃コライダが消えることを伝える
	if(!colliders_.empty())AttackManager::GetInstance().DeleteAttackCollider(colliders_[1]);

	//コライダの全削除
	DeleteAllColliders();
}

void EnemyBase::DoUpdate(void)
{
	//移動後座標に更新
	pos_ = movedPos_;

	//アニメーション更新
	animController_->Update();

	//状態の更新
	state_->Update(*this);
}

void EnemyBase::InitAnim(void)
{
	//空なら何もしない
	if (!animController_.get())return;

	//初期アニメーション
	animController_->Play(L"Idle");
}

void EnemyBase::DrawDebug(void)
{
#ifdef _DEBUG

	for (auto& col : colliders_) {
		if(col->IsUseThis())col->DrawDebugCollider();
	}

#endif // DEBUG
}

void EnemyBase::EnterStay(void)
{
	//待機アニメーションの再生
	animController_->Play(L"Idle");

	//速度設定
	speed_ = 0.0f;
}

void EnemyBase::EnterMove(void)
{
	//歩きアニメーションの再生
	animController_->Play(L"Run", RUN_ANIM_SPEED);

	//速度設定
	speed_ = WALK_SPEED;
}

void EnemyBase::EnterAlert(void)
{
	//待機アニメーションの再生
	animController_->Play(L"Walk");

	//速度設定
	speed_ = ALERT_SPEED;
}

void EnemyBase::EnterAttackReady(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//準備入り
	currentSkill_->ReadyEnter(*this);
}

void EnemyBase::EnterAttack(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//攻撃入り
	currentSkill_->Enter(*this);
}

void EnemyBase::EnterAttackEnd(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//攻撃終了
	RemoveCurrentSkill();
}

void EnemyBase::EnterReturn(void)
{
	//歩きアニメーションの再生
	animController_->Play(L"Run", RUN_ANIM_SPEED);

	//速度設定
	speed_ = RETURN_SPEED;
}

void EnemyBase::UpdateStay(void)
{
}

void EnemyBase::UpdateMove(void)
{
	//グループの目標地点に直接向かう
	goalPos_ = group_->GetGoalPos();

	//移動量の更新
	UpdateMovePow();
}

void EnemyBase::UpdateAlert(void)
{
	//グループの目標地点に直接向かう
	goalPos_ = group_->GetGoalPos();

	//移動量の更新
	UpdateMovePow();
}

void EnemyBase::UpdateAttackReady(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//目標地点を更新
	goalPos_ = group_->GetGoalPos();

	//スキルごとの準備行動
	if (currentSkill_->ReadyUpdate(*this))
	{
		//攻撃移行
		ChangeAction(ENEMY_ACTION::ATTACK);
	}
}

void EnemyBase::UpdateAttack(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//スキルごとの行動
	if (currentSkill_->Update(*this))
	{
		//待機
		ChangeAction(ENEMY_ACTION::ATTACK_END);
	}
}

void EnemyBase::UpdateAttackEnd(void)
{
	//スキルが入ってないなら強制的に待機に移行
	if (!currentSkill_)
	{
		ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//スキルごとの行動
	if (currentSkill_->EndUpdate(*this))
	{
		//完全終了
		currentSkill_->EndExit(*this);

		//破棄
		currentSkill_ = nullptr;

		//待機
		ChangeAction(ENEMY_ACTION::STAY);
	}
}

void EnemyBase::UpdateReturn(void)
{
	//グループ座標を目標地点に設定
	goalPos_ = group_->GetGroupPos();

	//移動量の更新
	UpdateMovePow();
}

void EnemyBase::ExitStay(void)
{
}

void EnemyBase::ExitMove(void)
{
}

void EnemyBase::ExitAlert(void)
{
}

void EnemyBase::ExitAttackReady(void)
{
	//準備終了
	if(currentSkill_)currentSkill_->ReadyExit(*this);
}

void EnemyBase::ExitAttack(void)
{
	//攻撃終了
	if (currentSkill_)currentSkill_->Exit(*this);
}

void EnemyBase::ExitAttackEnd(void)
{
}

void EnemyBase::ExitReturn(void)
{
}

void EnemyBase::Attack(void)
{
}

void EnemyBase::Move(void)
{
	//重力
	GravityManager::GetInstance().CalcGravity(Utility::DIR_D, gravityPow_);
	movePow_ = VAdd(movePow_, gravityPow_);

	//移動量をレートに合わせる
	VECTOR movePow = VScale(movePow_, SceneManager::GetInstance().GetUpdateSpeedRate());

	//移動後座標の更新
	VECTOR movedPos = VAdd(movedPos_, movePow);

	//回転の更新
	Rotation();

	//移動後座標の更新
	movedPos_ = movedPos;

	//下に落ちすぎないようにする
	if (movedPos.y < -100.0f)
	{
		isGrounding_ = true;
	}

	//地面にめり込まないようにする
	if(isGrounding_)
	{
		movedPos_.y = 0.0f;
		gravityPow_ = Utility::VECTOR_ZERO;
	}
}

void EnemyBase::Rotation(void)
{
	//移動後座標の更新
	VECTOR movedPos = VAdd(movedPos_, movePow_);

	//回転の更新
	VECTOR moveDir = Utility::GetMoveVec(movedPos, movedPos_);
	quaRot_ = quaRot_.LookRotation(VGet(moveDir.x, 0.0f, moveDir.z));
}

void EnemyBase::UpdateMovePow(void)
{
	//目標地点に向かう移動量の設定
	movePow_ = Utility::GetMoveVec(pos_, goalPos_, speed_);
}

void EnemyBase::BackMove(void)
{
	//移動
	Move();

	//回転の更新
	BackRotation();
}

void EnemyBase::BackRotation(void)
{
	//移動後座標の更新
	VECTOR movedPos = VAdd(movedPos_, movePow_);

	//回転の更新
	VECTOR moveDir = Utility::GetMoveVec(movedPos_, movedPos);
	quaRot_ = quaRot_.LookRotation(VGet(moveDir.x, 0.0f, moveDir.z));
}

void EnemyBase::Knockback(void)
{
	//重力
	GravityManager::GetInstance().CalcGravity(Utility::DIR_D, gravityPow_);
	GravityManager::GetInstance().CalcGravity(Utility::DIR_F, movePow_);
	movePow_ = VAdd(movePow_, gravityPow_);

	//移動量をレートに合わせる
	VECTOR movePow = VScale(movePow_, SceneManager::GetInstance().GetUpdateSpeedRate());

	//移動後座標の更新
	VECTOR movedPos = VAdd(movedPos_, movePow);

	//移動後座標の更新
	movedPos_ = movedPos;

	//下に落ちすぎないようにする
	if (movedPos.y < -100.0f)
	{
		isGrounding_ = true;
	}

	//地面にめり込まないようにする
	if (isGrounding_)
	{
		movedPos_.y = 0.0f;
		gravityPow_ = Utility::VECTOR_ZERO;
	}
}

void EnemyBase::EnableHitCollider(void)
{
	//当たり判定の有効化
	EnableColliderAtTag(Collider::COL_TAG::ENEMY);
}

void EnemyBase::DisableHitCollider(void)
{
	//当たり判定の無効化
	DisableColliderAtTag(Collider::COL_TAG::ENEMY);
}

const float EnemyBase::GetHitRadius(void)
{
	return colliders_[0]->GetGeometry().GetBroudRadius();
}

void EnemyBase::SetAttackPos(const VECTOR& _localPos)
{
	//攻撃目標座標の設定
	attackPos_ = VAdd(pos_, quaRot_.PosAxis(_localPos));
}

void EnemyBase::SetAttackRadius(const float _radius)
{
	//本来空で来ることはないが例外があった時のため
	if (colliders_.empty())CreateCollider();

	//半径変更
	auto sphere = colliders_[1]->GetGeometry<Sphere>();
	sphere->SetRadius(_radius);
	sphere->SetBroudRadius(_radius);
}

void EnemyBase::EnableAttack(void)
{
	//攻撃コライダの有効化
	EnableColliderAtTag(Collider::COL_TAG::ENEMY_ATTACK);
}

void EnemyBase::DisableAttack(void)
{
	//攻撃コライダの無効化
	DisableColliderAtTag(Collider::COL_TAG::ENEMY_ATTACK);
}

void EnemyBase::PlayAnim(const std::wstring& _animName, const float _speed)
{
	animController_->Play(_animName, _speed);
}

void EnemyBase::PlayAnimIsFinish(const std::wstring& _animName, const float _speed)
{
	if(animController_->IsFinishNormalAnim())animController_->Play(_animName, _speed);
}

void EnemyBase::PlayNoBlendAnim(const std::wstring& _animName, const float _speed)
{
	animController_->NoBlendPlay(_animName, _speed);
}

void EnemyBase::PlayNoBlendAnimIsFinish(const std::wstring& _animName, const float _speed)
{
	if (animController_->IsFinishNormalAnim())animController_->NoBlendPlay(_animName, _speed);
}

const bool EnemyBase::IsFade(void) const
{
	return state_->GetStateId() == ENEMY_STATE::FADE;
}

const bool EnemyBase::IsEndState(void) const
{
	return state_->GetStateId() == ENEMY_STATE::END;
}

void EnemyBase::SetEventKey(const EVENT_TYPE& _event)
{
	eventKey_ = _event;
}

void EnemyBase::AddEventCount(void)const
{
	//マネージャーに伝える
	EventManager::GetInstance().AddFlagCount(eventKey_);
}

void EnemyBase::SubEventCount(void)const
{
	//マネージャーに伝える
	EventManager::GetInstance().SubFlagCount(eventKey_);
	MissionManager::GetInstance().AddProgress(type_);
}

void EnemyBase::OnEnterActiveChank(void)
{
	//コライダ生成
	CreateCollider();

	//影を追加
	auto& shadow = ShadowManager::GetInstance();
	shadow.AddShadowModel(ShadowManager::MESH_TYPE::SKINNED, modelId_);
}

void EnemyBase::OnLeaveActiveChank(void)
{
	//コライダの削除
	DeleteCollider();

	//影を削除
	auto& shadow = ShadowManager::GetInstance();
	shadow.SubShadowModel(ShadowManager::MESH_TYPE::SKINNED, modelId_);
}

void EnemyBase::Death(void)
{
	//死亡アニメーションの再生
	animController_->Play(L"BlowEnd");
}