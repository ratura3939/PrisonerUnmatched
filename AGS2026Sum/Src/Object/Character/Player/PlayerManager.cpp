#include"../../pch.h"
#include"../../../Application.h"
#include"../../../Manager/Generic/InputManager.h"
#include"../../../Manager/Generic/SceneManager.h"
#include"../../../Manager/Generic/Camera.h"
#include"../../../Manager/Generic/ResourceManager.h"
#include"../../../Manager/GameSystem/AttackManager.h"
#include"../../../Manager/Decoration/UIManager2d.h"
#include"../../../Manager/Decoration/SoundManager.h"
#include"../../../Scene/Main/Game.h"
#include"../../../Scene/Sub/PauseScene.h"
#include"PlayerChara.h"
#include "PlayerAttack.h"
#include"AttackCommandInfo.h"
#include "PlayerManager.h"

const std::wstring PlayerManager::ANIM_IDLE = L"Idle";
const std::wstring PlayerManager::ANIM_RUN = L"Run";
const std::wstring PlayerManager::ANIM_DAMAGE = L"Damage";
const std::wstring PlayerManager::ANIM_FIRST_PUNCH = L"FirstPunch";
const std::wstring PlayerManager::ANIM_SECOND_PUNCH = L"SecondPunch";
const std::wstring PlayerManager::ANIM_THIRD_PUNCH = L"ThirdPunch";
const std::wstring PlayerManager::ANIM_MIDDLE_KICK = L"MiddleKick";
const std::wstring PlayerManager::ANIM_HIGH_KICK = L"HighKick";
const std::wstring PlayerManager::ANIM_FINSH_KICK = L"FinishKick";
const std::wstring PlayerManager::ANIM_SPECIAL_PUNCH = L"SpecialPunch";
const std::wstring PlayerManager::ANIM_SPECIAL_KICK = L"SpecialKick";
const std::wstring PlayerManager::ANIM_ULTIMATE = L"Ultimet";
const std::wstring PlayerManager::ANIM_ULTIMATE_TEST = L"UltimetTest";

namespace {
	const VECTOR FOCUS_RELATIVE = { 0.0f,0.0f,300.0f };			//注視点の相対座標
	const VECTOR ULTIMET_RELATIVE = { 100.0f,200.0f,40.0f };	//必殺技時の注視点からカメラの相対座標
	const VECTOR HIGHT_HALF = { 0.0f,100.0f,0.0f };				//キャラクターの腰辺りの相対座標

	const float CAMERA_STY_TIME_ULTIMATE = 60.0f;

	const VECTOR POS_OPERATION_INFO = { 100.0f,800.0f / 2.0f,0.0f };
}

PlayerManager::PlayerManager(Game& _gameScene)
	:scene_(_gameScene)
	,character_(std::make_shared<PlayerChara>())
	,attack_(nullptr)
	,isEnableAttackInput_(true)
	,isForcePlayAnim_(false)
	,isNoBlendPlayAnim_(false)
	,isSpecialAttackReady_(false)
	,isEnableSpecial_(true)
	,isEnableUltimate_(false)
	,isPlaySoundAtCurrentAttack_(false)
	,isPlayEffectAtCurrentAttack_(false)
{
	character_->Load();		//キャラクターの読み込み
}

PlayerManager::~PlayerManager(void)
{
}

void PlayerManager::Init(void)
{
	character_->Init();		//キャラクターの初期化

	attack_ = std::make_unique<PlayerAttack>(character_->GetPos(), character_->GetQua());	//攻撃クラスの生成
	attack_->Init();		//攻撃クラスの初期化

	commandInfo_ = std::make_unique<AttackCommandInfo>();
	commandInfo_->Init();

	//注視点の更新
	Camera& camera = SceneManager::GetInstance().GetCamera();
	focusPos_ = VAdd(character_->GetPos(), camera.GetRot().PosAxis(FOCUS_RELATIVE));

	//操作方法のUI登録
	UIManager2d& uiM = UIManager2d::GetInstance();
	uiM.Add(UIManager2d::UI_NAME::OPERATION_INFO,
		ResourceManager::GetInstance().Load(ResourceManager::SRC::OPERATION_INFO_IMG).handleId_,
		UIManager2d::UI_DIRECTION_2D::NORMAL, UIManager2d::UI_DRAW_DIMENSION::DIMENSION_2);

	uiM.SetUIInfo(UIManager2d::UI_NAME::OPERATION_INFO, POS_OPERATION_INFO);

	//必殺準備効果音
	SoundManager::GetInstance().Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ULTIMATE_READY, ResourceManager::GetInstance().Load(ResourceManager::SRC::ULTIMATE_REDY_SE).handleId_);
}

void PlayerManager::Update(void)
{
	if (isUltimateReady_) {
		commandInfo_->Appear(AttackCommandInfo::TYPE::ULTIMATE);
	}

	UserInput();			//入力受付
	character_->Update();	//キャラクターの更新
	attack_->Update();		//攻撃クラスの更新
	commandInfo_->Update();	//攻撃説明の更新

	//注視点の更新
	Camera& camera = SceneManager::GetInstance().GetCamera();
	focusPos_ = VAdd(character_->GetPos(), camera.GetRot().PosAxis(FOCUS_RELATIVE));

	//攻撃中だが攻撃のコライダーが未だ有効ではないとき
	if (attack_->IsAttacking() && !attack_->IsEnableCollier()) {
		std::wstring animName = attack_->GetCurrentAttackAnimInfo().name;					//攻撃中の名前を取得
		float animProgressRate = character_->GetSpecifiedAnimationProgressRate(animName);	//進捗率を取得

		attack_->TryEnableAttackCollider(animProgressRate);	//進捗を渡し、発生トライ
	}

	UpdateAnimationEvent();		//アニメーションと連動した処理
}

void PlayerManager::Draw(void)
{
	character_->Draw();		//キャラクターの描画
	attack_->Draw();		//攻撃クラスの描画
	
	
#ifdef _DEBUG
	if (isSpecialAttackReady_) {
		DrawString(30, 300, L"SpecialRedy!!!", 0xffffff);
	}

	attack_->DrawDebug();	//攻撃クラスのデバッグ描画
	DrawFormatString(30, 280, 0xffffff, L"AttackCansel = %d", static_cast<int>(isEnableAttackInput_));	//現在の攻撃アニメーション登録名の先頭文字を表示(デバッグ用)
#endif
}

void PlayerManager::DrawUI(void)
{
	commandInfo_->Draw();	//攻撃説明の描画

	UIManager2d::GetInstance().Draw(UIManager2d::UI_NAME::OPERATION_INFO);	//操作方法の描画
}

void PlayerManager::Release(void)
{
	character_->Release();	//キャラクターの解放
}

const VECTOR& PlayerManager::GetPos(void) const
{
	return character_->GetPos();
}

const VECTOR& PlayerManager::GetFocusPos(void)
{
	return focusPos_;
}

const Quaternion& PlayerManager::GetQua(void)
{
	return character_->GetQua();
}

const bool PlayerManager::IsAlive(void) const
{
	return character_->IsAlive();
}

const CharacterBase::KNOCKBACK_TYPE PlayerManager::GetCurrentKnockBackType(void) const
{
	const std::string knockBackTypeString = attack_->GetCurrentAttackKnockBackType();	//攻撃クラスから現在の攻撃の吹っ飛び方の文字列を取得

	using KNOCKBACK_TYPE = CharacterBase::KNOCKBACK_TYPE;

	//文字列と照らし合わせて、ノックバックの種類を返す
	if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::STAGGER)]) {
		return KNOCKBACK_TYPE::STAGGER;
	}
	else if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::PUSH_BACK)]) {
		return KNOCKBACK_TYPE::PUSH_BACK;
	}
	else if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::LAUNCH)]) {
		return KNOCKBACK_TYPE::LAUNCH;
	}
	else if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::FLOAT)]) {
		return KNOCKBACK_TYPE::FLOAT;
	}
	else if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::SLAM)]) {
		return KNOCKBACK_TYPE::SLAM;
	}
	else if (knockBackTypeString == CharacterBase::KNOCKBACK_TYPE_STRING[static_cast<int>(KNOCKBACK_TYPE::BLOW_AWAY)]) {
		return KNOCKBACK_TYPE::BLOW_AWAY;
	}

	//例外
	return KNOCKBACK_TYPE();
}

void PlayerManager::SetAnimSpeedPercent(const float _percent)
{
	character_->SetAnimationSpeedPercent(_percent);
}

void PlayerManager::DisappearAttackCommandInfo(void)
{
	commandInfo_->Disappear(AttackCommandInfo::TYPE::SEPECIAL);
}

void PlayerManager::DisappearUltimateCommandInfo(void)
{
	commandInfo_->Disappear(AttackCommandInfo::TYPE::ULTIMATE);
}

void PlayerManager::UpdateAnimationEvent(void)
{
	//アニメーションが終了していたら(攻撃関連のアニメーションに限り発生するもの)
	if (character_->IsFinishAttackAnimation()) {
		//攻撃予約関係
		isEnableAttackInput_ = true;	//攻撃入力を受け付ける状態にする
		isEnableSpecial_ = true;		//特殊準備を受け入れる
		isEnableUltimate_ = false;

		bool ret = character_->IsStartNextAttackAnimation();

		if (ret) {
			Attack();
		}
		else {
			//コンボの終了
			attack_->FinishAttack();	//攻撃終了処理
			character_->SetIsAttack(false);	//そうでないときは攻撃状態を解除する
			EffectManager::GetInstance().StopOrderEffectName(attack_->GetNextAttackDirectionInfo().efcName);
		}
	}

	//攻撃のSE・エフェクトの再生管理
	if (attack_->IsAttacking()) {
		const PlayerAttack::AttackDirectionInfo& currentAttackDirecInfo = attack_->GetNextAttackDirectionInfo();	//演出情報取得
		std::wstring animName = attack_->GetCurrentAttackAnimInfo().name;					//攻撃中の名前を取得
		float animProgressRate = character_->GetSpecifiedAnimationProgressRate(animName);	//進捗率を取得

		//まだ効果音が再生されいない場合
		if (!isPlaySoundAtCurrentAttack_) {
			//再生を試みる
			TryPlaySoundAtCurrentAttack(currentAttackDirecInfo, animProgressRate);
		}

		//エフェクトの再生
		if (!isPlayEffectAtCurrentAttack_) {
			//再生を試みる
			TryPlayEffectAtCurrentAttack(currentAttackDirecInfo, animProgressRate);
		}

	}
}

void PlayerManager::UserInput(void)
{
	InputManager& ins = InputManager::GetInstance();

	//ポーズシーン
	if (ins.IsTriggerDown(InputManager::INPUT_COMMAND::PAUSE)) {
		//シーン追加(一つ次へ)
		SceneManager::GetInstance().PushScene(std::make_shared<PauseScene>());
	}

#pragma region 攻撃
	bool isAttackInput = false;

	//必殺技中は入力を受け付けない
	if (isEnableUltimate_) {
		return;
	}

	if (ins.IsTriggerDown(InputManager::INPUT_COMMAND::ATTACK_NORMAL)) {
		//特殊攻撃準備中の場合
		if (isSpecialAttackReady_) {
			isAttackInput = ReserveAttackSpecial(PlayerAttack::ATTACK_TYPE::PUNCH);	//攻撃クラスに特殊攻撃開始を伝え,結果を得る
		}
		//通常時、攻撃を受け付ける状態の場合
		else if (isEnableAttackInput_) {
			isAttackInput = ReserveAttack(PlayerAttack::ATTACK_TYPE::PUNCH);	//攻撃クラスに攻撃開始を伝え,結果を得る
		}
	}
	else if (ins.IsTriggerDown(InputManager::INPUT_COMMAND::ATTACK_STRONG)) {
		//特殊攻撃準備中の場合
		if (isSpecialAttackReady_&& isEnableSpecial_) {
			isAttackInput = ReserveAttackSpecial(PlayerAttack::ATTACK_TYPE::KICK);	//攻撃クラスに特殊攻撃開始を伝え,結果を得る
		}
		//通常時、攻撃を受け付ける状態の場合
		else if (isEnableAttackInput_) {
			isAttackInput = ReserveAttack(PlayerAttack::ATTACK_TYPE::KICK);	//攻撃クラスに攻撃開始を伝え,結果を得る
		}
	}

	//必殺技
	//デバッグ用のボタンが押されていたら
	if (/*ins.IsPressed(InputManager::INPUT_COMMAND::DEBUG_ULT_REDY) && */ins.IsTriggerDown(InputManager::INPUT_COMMAND::CANCEL) && isUltimateReady_) {
		attack_->ReserveAttackUltimate();	//必殺技予約
		isEnableUltimate_ = true;			//必殺技中
		scene_.StartSlow();					//スロー演出
		isNoBlendPlayAnim_ = true;

		commandInfo_->Disappear(AttackCommandInfo::TYPE::ULTIMATE);

		//カメラが必殺技用の移動を開始
		scene_.SetProcessingAfterCameraAutoMove(Game::CAMERA_MOVE_SITUATION::ULTIMATE);

		//準備音再生
		SoundManager::GetInstance().Play(SoundManager::SOUND_NAME::ULTIMATE_READY);

		SettingUltimateCamera();
		isAttackInput = true;
	}

	//入力があったとき
	if (isAttackInput) {
		SetAttackStateForCharacter();	//攻撃に関する情報をキャラクターに反映
	}

	//特殊攻撃準備
	//開始
	if (ins.IsTriggerDown(InputManager::INPUT_COMMAND::ATTACK_SPECIAL) && isEnableSpecial_) {
		scene_.StartSlow();
		commandInfo_->Appear(AttackCommandInfo::TYPE::SEPECIAL);
	}
	//終了
	else if (ins.IsTrigerrUp(InputManager::INPUT_COMMAND::ATTACK_SPECIAL)) {
		scene_.EndSlow();
		commandInfo_->Disappear(AttackCommandInfo::TYPE::SEPECIAL);
	}
#pragma endregion


#pragma region 移動
	auto moveVec = ins.GetMoveInput();	//LS・WASDの移動入力を取得

	//入力がある場合
	if (moveVec.x != 0.0f || moveVec.y != 0.0f) {
		character_->InputMoveVec(VECTOR(moveVec.x, moveVec.y, 0.0f));	//移動方向をキャラクターに渡す
	}
#pragma endregion

}

void PlayerManager::Attack(void)
{
	attack_->Attack();						//攻撃
	isPlaySoundAtCurrentAttack_ = false;	//攻撃が切り替わるためフラグをfalseに
	isPlayEffectAtCurrentAttack_ = false;
}

void PlayerManager::SetAttackStateForCharacter(void)
{
	auto attackAnimInfo = attack_->GetNextAttackAnimInfo();	//再生するアニメーション名取得

	//例外の場合
	if (attackAnimInfo.name.empty()) {
		return;
	}

	character_->SetIsAttack(true);	//攻撃中は攻撃状態にする

	auto seInfo = attack_->GetNextAttackDirectionInfo();	//SE情報取得

	//アニメーションの再生
	if (isNoBlendPlayAnim_) {
		Attack();
		character_->NoBlendPlayAnim(attackAnimInfo.name, attackAnimInfo.speed);
		return;
	}

	if (isForcePlayAnim_) {
		Attack();		//攻撃開始(コンボ始動のため即時発動)				
		character_->ForcePlayAnim(attackAnimInfo.name, attackAnimInfo.speed);	//攻撃アニメを強制再生する
	}
	else {
		character_->PlayAnim(attackAnimInfo.name, attackAnimInfo.speed);	//攻撃アニメを再生する
	}
}

const bool PlayerManager::ReserveAttack(PlayerAttack::ATTACK_TYPE _type)
{
	bool isSuccess = false;	//処理成功フラグ
	isForcePlayAnim_ = false;

	//攻撃中の場合
	if (attack_->IsAttacking()) {
		//攻撃キャンセル可能な状態であれば
		if (character_->GetCurrentAnimationProgressRate() >= PlayerAttack::ATTACK_CANCEL_RATE) {
			isSuccess = attack_->ReserveAttack(_type);	//次段の攻撃予約

			//キャンセル可能状態中は一度だけ攻撃入力を受け付ける
			isEnableAttackInput_ = false;	//攻撃入力を受け付けない状態にする
		}
	}
	else {
		//攻撃始動
		isSuccess = attack_->ReserveAttack(_type);	//初段の攻撃予約
		isForcePlayAnim_ = true;		//強制再生フラグON
	}

	return isSuccess;
}

const bool PlayerManager::ReserveAttackSpecial(PlayerAttack::ATTACK_TYPE _type)
{
	attack_->ReserveAttackSpecial(_type);
	isForcePlayAnim_ = true;
	isEnableSpecial_ = false;	//発生中なので再度発動できないように

	//特殊攻撃を発生させるときにスロー終了
	scene_.EndSlow();

	return true;
}

void PlayerManager::SettingUltimateCamera(void)
{
	Camera& camera = SceneManager::GetInstance().GetCamera();

	camera.ChangeMode(Camera::MODE::AUTO_MOVE);		//モード変更
	scene_.SetCameraStayTimeAtAutoMove(CAMERA_STY_TIME_ULTIMATE);	//ゲームシーンに溜め時間を渡す

	VECTOR cameraGoalPos = VAdd(character_->GetPos(), character_->GetQua().PosAxis(ULTIMET_RELATIVE));	//目標位置
	camera.SetGoalPos(cameraGoalPos);	//設定

	VECTOR cameraocusPos = VAdd(character_->GetPos(), HIGHT_HALF);	//カメラ注視点(キャラクターの半分ほどの高さ)
	camera.SetFocusPos(cameraocusPos);
}

void PlayerManager::TryPlaySoundAtCurrentAttack(const PlayerAttack::AttackDirectionInfo& _info, const float _animProgressRate)
{
	//進捗が一定以上なら
	if (_animProgressRate >= _info.detail.seTiming) {
		SoundManager::GetInstance().Play(_info.seName);		//効果音の再生
		isPlaySoundAtCurrentAttack_ = true;
	}
}

void PlayerManager::TryPlayEffectAtCurrentAttack(const PlayerAttack::AttackDirectionInfo& _info, const float _animProgressRate)
{
	//進捗が一定以上なら
	if (_animProgressRate >= _info.detail.efcTiming) {
		//再生開始に必要な情報生成
		Quaternion efcLocalQua = Quaternion::Euler(Utility::Deg2RadVec(_info.detail.efcLocalRot));	//回転情報
		VECTOR efcPos = VAdd(character_->GetPos(), character_->GetQua().PosAxis(_info.detail.efcLocalPos));	//発生位置

		//発生
		EffectManager::GetInstance().Play(character_->GetSpeciesName(), _info.efcName,
			efcPos, character_->GetQua().Mult(efcLocalQua),
			_info.detail.efcScale, _info.detail.efcSpeed);

		isPlayEffectAtCurrentAttack_ = true;
	}
}
