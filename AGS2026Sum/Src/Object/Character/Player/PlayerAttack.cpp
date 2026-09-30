#include "../../../pch.h"
#include"../../../Manager/Generic/ResourceManager.h"
#include"../../../Manager/GameSystem/AttackManager.h"
#include"../../Common/Geometry/Sphere.h"
#include"PlayerManager.h"
#include "PlayerAttack.h"

namespace {
	//攻撃データのキー(PlayerAttackData.jsonより参照)
	const std::string PUNCH_FIRST_KEY = "Punch_First";
	const std::string PUNCH_SECOND_KEY = "Punch_Second";
	const std::string PUNCH_THIRD_KEY = "Punch_Third";
	const std::string KICK_FIRST_KEY = "Kick_First";
	const std::string KICK_SECOND_KEY = "Kick_Second";
	const std::string KICK_THIRD_KEY = "Kick_Third";
	const std::string SPECIAL_PUNCH_KEY = "Punch_Special";
	const std::string SPECIAL_KICK_KEY = "Kick_Special";
	const std::string ULTIMATE_KEY = "Ultimate";

	const int DEFAULT_COLOR = 0xffffff;
	const int KICK_COLOR = 0x0000ff;
	const int PUNCH_COLOR = 0x00ff00;

	const int COMBO_RESET_FRAME = 30;	//コンボリセットまでのフレーム数
	const int CIRCLE_SCALE = 10;	//表記する円の大きさ

	const float COMBO_ELEMENT_DRAW_DIF_X = 50;	//コンボルートの要素の表記差分X座標
	const float COMBO_ELEMENT_DRAW_DIF_Y = 50;	//コンボルートの要素の表記差分Y座標

	const float DRAW_SPECIAL_INFO_X = 2 / 3;

	//矢印関係
	const int DIFF_FOR_ARROW_X = 25;	//矢印描画位置のX差分
	const double DEG_FOR_BOTTOM_RIGHT = 45.0;	//右下用の角度差分
	const int DIFF_FOR_BOTTOM_RIGHT_ARROW = 25;	//右下用のY差分
}

PlayerAttack::PlayerAttack(const VECTOR& _playerPos, const Quaternion& _playerQuaRot)
	:ActorBase()
	,playerPos_(_playerPos)
	,playerQuaRot_(_playerQuaRot)
	,currentData_()
	,data_()
	,animNames_()
	,latestReserveType_(ATTACK_TYPE::MAX)
	,currentAttackName_()
	,nextAttackName_()
	,comboReset_(true)
	,comboResetCounter_(0)
	,isAttacking_(false)
{
	debugColor_ = 0xffffff;
}

PlayerAttack::~PlayerAttack(void)
{
}

void PlayerAttack::TryEnableAttackCollider(const float _animRate)
{
	//アニメーションの進捗が許可タイミング以上ならば
	if (_animRate >= currentData_.colliderEnableTiming) {
		colliders_[0]->SetUseThis(true);	//コライダーの有効化

		//コントローラー振動
		StartJoypadVibration(DX_INPUT_PAD1, currentData_.vibratePower, currentData_.vibrateTime);
	}
}

void PlayerAttack::DrawDebug(void)
{
	if (IsEnableCollier()) {
		const int DivNum = 16;	//分割数

		DrawSphere3D(pos_, currentData_.radius, DivNum, debugColor_, debugColor_, false);	//コライダーの描画
	}
}

void PlayerAttack::DoLoad(void)
{
}

void PlayerAttack::DoInit(void)
{
	MakeCollider(std::make_unique<Sphere>(pos_, pos_, quaRot_, currentData_.radius, currentData_.radius), Collider::COL_TAG::PLAYER_ATTACK, { Collider::COL_TAG::ENEMY });	//攻撃用のコライダ生成
	LoadAttackData();

	//各種演出用の空を生成
	directionNames_.emplace(PUNCH_FIRST_KEY, INIT_ANIM_DIRECTION_INFO);	//パンチ初段
	directionNames_.emplace(PUNCH_SECOND_KEY, INIT_ANIM_DIRECTION_INFO);	//パンチ二段
	directionNames_.emplace(PUNCH_THIRD_KEY, INIT_ANIM_DIRECTION_INFO);	//パンチ最終段
	directionNames_.emplace(KICK_FIRST_KEY, INIT_ANIM_DIRECTION_INFO);		//キック初段
	directionNames_.emplace(KICK_SECOND_KEY, INIT_ANIM_DIRECTION_INFO);	//キック二段
	directionNames_.emplace(KICK_THIRD_KEY, INIT_ANIM_DIRECTION_INFO);		//キック最終段
	directionNames_.emplace(SPECIAL_PUNCH_KEY, INIT_ANIM_DIRECTION_INFO);
	directionNames_.emplace(SPECIAL_KICK_KEY, INIT_ANIM_DIRECTION_INFO);
	directionNames_.emplace(ULTIMATE_KEY, INIT_ANIM_DIRECTION_INFO);

	LoadAttackSound();			//SE
	LoadAttackEffect();			//Effect
	LoadAttackDirectionData();	//Timing

	colliders_[0]->SetUseThis(false);	//コライダの無効化
	AttackManager::GetInstance().AddAttackCollider(AttackManager::ATTACK_TYPE::P_ATTACK, colliders_[0]);	//攻撃マネージャーに登録

	ResourceManager& resM = ResourceManager::GetInstance();
	comboElementImages_[static_cast<int>(ATTACK_TYPE::PUNCH)] = resM.Load(ResourceManager::SRC::PUNCH_ATK_ON_OFF_IMG).handleIds_;
	comboElementImages_[static_cast<int>(ATTACK_TYPE::KICK)] = resM.Load(ResourceManager::SRC::KICK_ATK_ON_OFF_IMG).handleIds_;

	//矢印
	arrowImage_ = resM.Load(ResourceManager::SRC::COMBO_ARROW_IMG).handleId_;
}

void PlayerAttack::DoUpdate(void)
{
	if (IsAttacking()) {
		//攻撃中は座標を更新
		pos_ = VAdd(playerPos_, playerQuaRot_.PosAxis(currentData_.localPos));	//プレイヤーの座標にローカル座標を加算して攻撃の座標とする
		quaRot_ = playerQuaRot_;												//敵にベクトルを与えるためにプレイヤー本体の回転をコピー
	}
	//攻撃していないとき
	else {
		comboResetCounter_++;	//コンボリセットカウンタを加算
		if (!comboReset_ && comboResetCounter_ > COMBO_RESET_FRAME) {
			//コンボリセットフラグが立っていない、かつコンボリセットカウンタが規定値を超えたとき
			ResetCombo();	//コンボリセット
		}
	}
	
}

void PlayerAttack::DrawComboRoute(void)
{
	const int DrawStartX = 30;	//表記開始X座標
	const int DrawStartY = 200;	//表記開始Y座標
	

	const int KickCircleDifferenceY = 50;

	int differenceDrawPosX = 0;	//表記差分X座標
	int differenceDrawPosY = 0;	//表記差分Y座標

	DrawComboRouteElement(startAttackAnimName_, { DrawStartX,DrawStartY,0 });	//コンボ始動の表記

	for (auto& info : comboRouteInfos_) {
		info.second.isDrawed = false;	//描画フラグリセット
	}
}

void PlayerAttack::DrawComboRouteElement(const std::string& _attackKey, const VECTOR& _pos)
{
	//すでに描画している場合は描画しない
	if (comboRouteInfos_[_attackKey].isDrawed) {
		return;		//終了
	}

	//描画
	const double elementExRate = 0.3;	//描画倍率
	const double elementAngle = 0.0;	//描画角度

	//ボタンの描画
	DrawRotaGraph(static_cast<int>(_pos.x), static_cast<int>(_pos.y),
		elementExRate, elementAngle,
		comboElementImages_[static_cast<int>(comboRouteInfos_[_attackKey].type)][comboRouteInfos_[_attackKey].isUsed], true);
	comboRouteInfos_[_attackKey].isDrawed = true;

	std::string nextPunchAttackName = data_[_attackKey].nextAttacks[static_cast<int>(ATTACK_TYPE::PUNCH)];	//次のパンチ攻撃
	std::string nextKickAttackName = data_[_attackKey].nextAttacks[static_cast<int>(ATTACK_TYPE::KICK)];	//次のキック攻撃

	//次の要素の描画位置調整
	VECTOR nextPos = _pos;
	nextPos.x += COMBO_ELEMENT_DRAW_DIF_X;	//次の要素の表記位置をずらす

	int arrowPosX = static_cast<int>(_pos.x) + DIFF_FOR_ARROW_X;
	int arrowPosY = static_cast<int>(_pos.y);
	double arrowAngle = 0.0;
	const double arrowExRate = 0.3;

	//要素がある場合
	if (nextPunchAttackName != "") {
		//パンチ派生時、現在の攻撃がキックの場合(現在はそんな派生はない)
		if (comboRouteInfos_[_attackKey].type == ATTACK_TYPE::KICK) {
			nextPos.y -= COMBO_ELEMENT_DRAW_DIF_Y;	//パンチの要素の表記位置をさらにずらす
		}

		//矢印の描画
		DrawRotaGraph(arrowPosX, arrowPosY, arrowExRate, arrowAngle, arrowImage_, true);

		//パンチ派生の描画
		DrawComboRouteElement(nextPunchAttackName, nextPos);	
	}

	if (nextKickAttackName != "") {
		//キック派生時、現在の攻撃がパンチの場合
		if (comboRouteInfos_[_attackKey].type == ATTACK_TYPE::PUNCH) {
			nextPos.y += COMBO_ELEMENT_DRAW_DIF_Y;	//キックの要素の表記位置をさらにずらす
			arrowAngle = Utility::Deg2RadD(DEG_FOR_BOTTOM_RIGHT);	//矢印を右斜め下に
			arrowPosY += DIFF_FOR_BOTTOM_RIGHT_ARROW;
		}

		//矢印の描画
		DrawRotaGraph(arrowPosX, arrowPosY, arrowExRate, arrowAngle, arrowImage_, true);

		DrawComboRouteElement(nextKickAttackName, nextPos);	//キック派生の描画
	}
}

void PlayerAttack::LoadAttackData(void)
{
	//攻撃の詳細を登録
	RegisterAttackData();	

	//アニメーション名登録
	RegisterAttackAnimData();
	
	//フラグ管理(コンボ履歴用)
	RegisterComboData();

	//コンボ始動は弱パンチから
	currentData_ = data_[PUNCH_FIRST_KEY];
	startAttackAnimName_ = PUNCH_FIRST_KEY;

	currentAttackName_ = KICK_THIRD_KEY;	//予約→発生というロジックの関係上、初段に設定するためには「次の攻撃」が設定されていない最終段を用いる

	ApplyAttackColliderSettings();	//情報適用
}

void PlayerAttack::LoadAttackSound(void)
{
	SoundManager& sndM = SoundManager::GetInstance();
	ResourceManager& resM = ResourceManager::GetInstance();
	using SOUND_TYPE = SoundManager::TYPE;
	using SOUND_NAME = SoundManager::SOUND_NAME;
	using SOURCE = ResourceManager::SRC;

	//SE読み込み
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::PUNCH_FIRST_PLAYER_SE,resM.Load(SOURCE::PUNCH_FIRST_PLAYER_SE).handleId_);		//パンチ初段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::PUNCH_SECOND_PLAYER_SE, resM.Load(SOURCE::PUNCH_SECOND_PLAYER_SE).handleId_);	//パンチ二段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::PUNCH_THIRD_PLAYER_SE, resM.Load(SOURCE::PUNCH_THIRD_PLAYER_SE).handleId_);	//パンチ最終段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::KICK_FIRST_PLAYER_SE, resM.Load(SOURCE::KICK_FIRST_PLAYER_SE).handleId_);		//キック初段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::KICK_SECOND_PLAYER_SE, resM.Load(SOURCE::KICK_SECOND_PLAYER_SE).handleId_);	//キック二段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::KICK_THIRD_PLAYER_SE, resM.Load(SOURCE::KICK_THIRD_PLAYER_SE).handleId_);		//キック最終段
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::PUNCH_SPECIAL_PLAYER_SE, resM.Load(SOURCE::PUNCH_SPECIAL_PLAYER_SE).handleId_);		//パンチ特殊
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::KICK_SPECIAL_PLAYER_SE, resM.Load(SOURCE::KICK_SPECIAL_PLAYER_SE).handleId_);		//キック特殊
	sndM.Add(SOUND_TYPE::SE, SOUND_NAME::ULTIMATE, resM.Load(SOURCE::ULTIMATE_SE).handleId_);		//必殺

	//SE連携
	directionNames_.at(PUNCH_FIRST_KEY).seName = SOUND_NAME::PUNCH_FIRST_PLAYER_SE;	//パンチ初段
	directionNames_.at(PUNCH_SECOND_KEY).seName = SOUND_NAME::PUNCH_SECOND_PLAYER_SE;//パンチ二段
	directionNames_.at(PUNCH_THIRD_KEY).seName = SOUND_NAME::PUNCH_THIRD_PLAYER_SE;	//パンチ最終段
	directionNames_.at(KICK_FIRST_KEY).seName = SOUND_NAME::KICK_FIRST_PLAYER_SE;	//キック初段
	directionNames_.at(KICK_SECOND_KEY).seName = SOUND_NAME::KICK_SECOND_PLAYER_SE;	//キック二段
	directionNames_.at(KICK_THIRD_KEY).seName = SOUND_NAME::KICK_THIRD_PLAYER_SE;	//キック最終段
	directionNames_.at(SPECIAL_PUNCH_KEY).seName = SOUND_NAME::PUNCH_SPECIAL_PLAYER_SE;	//パンチ特殊
	directionNames_.at(SPECIAL_KICK_KEY).seName = SOUND_NAME::KICK_SPECIAL_PLAYER_SE;	//キック特殊
	directionNames_.at(ULTIMATE_KEY).seName = SOUND_NAME::ULTIMATE;		//必殺技
}

void PlayerAttack::LoadAttackEffect(void)
{
	EffectManager& efcM = EffectManager::GetInstance();
	ResourceManager& resM = ResourceManager::GetInstance();
	using EFC_NAME = EffectManager::EFFECT_NAME;
	using SOURCE = ResourceManager::SRC;

	efcM.Add(EFC_NAME::PLAYER_PUNCH_ONE_TWO, resM.Load(SOURCE::PLAYER_PUNCH_ONE_TWO_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_PUNCH_THIRD, resM.Load(SOURCE::PLAYER_PUNCH_THIRD_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_KICK_FIRST, resM.Load(SOURCE::PLAYER_KICK_FIRST_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_KICK_SECOND, resM.Load(SOURCE::PLAYER_KICK_SECOND_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_KICK_THIRD, resM.Load(SOURCE::PLAYER_KICK_THIRD_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_PUNCH_SPECIAL, resM.Load(SOURCE::PLAYER_PUNCH_SPECIAL_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_KICK_SPECIAL, resM.Load(SOURCE::PLAYER_KICK_SPECIAL_EFC).handleId_);
	efcM.Add(EFC_NAME::PLAYER_ULTIMATE, resM.Load(SOURCE::PLAYER_ULTIMATE_EFC).handleId_);

	directionNames_.at(PUNCH_FIRST_KEY).efcName = EFC_NAME::PLAYER_PUNCH_ONE_TWO;	//パンチ初段
	directionNames_.at(PUNCH_SECOND_KEY).efcName = EFC_NAME::PLAYER_PUNCH_ONE_TWO;	//パンチ二段
	directionNames_.at(PUNCH_THIRD_KEY).efcName = EFC_NAME::PLAYER_PUNCH_THIRD;		//パンチ最終段
	directionNames_.at(KICK_FIRST_KEY).efcName = EFC_NAME::PLAYER_KICK_FIRST;		//キック初段
	directionNames_.at(KICK_SECOND_KEY).efcName = EFC_NAME::PLAYER_KICK_SECOND;		//キック二段
	directionNames_.at(KICK_THIRD_KEY).efcName = EFC_NAME::PLAYER_KICK_THIRD;		//キック最終段
	directionNames_.at(SPECIAL_PUNCH_KEY).efcName = EFC_NAME::PLAYER_PUNCH_SPECIAL;	//パンチ特殊
	directionNames_.at(SPECIAL_KICK_KEY).efcName = EFC_NAME::PLAYER_KICK_SPECIAL;	//キック特殊
	directionNames_.at(ULTIMATE_KEY).efcName = EFC_NAME::PLAYER_ULTIMATE;			//必殺技
}

void PlayerAttack::RegisterAttackData(void)
{
	Resource res = ResourceManager::GetInstance().Load(ResourceManager::SRC::PLAYER_ATTACK_DATA_JSON);

	//登録
	data_.emplace(PUNCH_FIRST_KEY, res.GetData<AttackData>(PUNCH_FIRST_KEY));	//パンチ初段
	data_.emplace(PUNCH_SECOND_KEY, res.GetData<AttackData>(PUNCH_SECOND_KEY));	//パンチ二段
	data_.emplace(PUNCH_THIRD_KEY, res.GetData<AttackData>(PUNCH_THIRD_KEY));	//パンチ最終段
	data_.emplace(KICK_FIRST_KEY, res.GetData<AttackData>(KICK_FIRST_KEY));		//キック初段
	data_.emplace(KICK_SECOND_KEY, res.GetData<AttackData>(KICK_SECOND_KEY));	//キック二段
	data_.emplace(KICK_THIRD_KEY, res.GetData<AttackData>(KICK_THIRD_KEY));		//キック最終段
	data_.emplace(SPECIAL_PUNCH_KEY, res.GetData<AttackData>(SPECIAL_PUNCH_KEY));//特殊攻撃(対横方向)
	data_.emplace(SPECIAL_KICK_KEY, res.GetData<AttackData>(SPECIAL_KICK_KEY));	//特殊攻撃(対上方向)
	data_.emplace(ULTIMATE_KEY, res.GetData<AttackData>(ULTIMATE_KEY));			//必殺技
}

void PlayerAttack::RegisterAttackAnimData(void)
{
	animNames_.emplace(PUNCH_FIRST_KEY, PlayerManager::ANIM_FIRST_PUNCH);		//パンチ初段
	animNames_.emplace(PUNCH_SECOND_KEY, PlayerManager::ANIM_SECOND_PUNCH);		//パンチ二段
	animNames_.emplace(PUNCH_THIRD_KEY, PlayerManager::ANIM_THIRD_PUNCH);		//パンチ最終段
	animNames_.emplace(KICK_FIRST_KEY, PlayerManager::ANIM_MIDDLE_KICK);		//キック初段
	animNames_.emplace(KICK_SECOND_KEY, PlayerManager::ANIM_HIGH_KICK);			//キック二段
	animNames_.emplace(KICK_THIRD_KEY, PlayerManager::ANIM_FINSH_KICK);			//キック最終段
	animNames_.emplace(SPECIAL_PUNCH_KEY, PlayerManager::ANIM_SPECIAL_PUNCH);	//特殊攻撃(対横方向)
	animNames_.emplace(SPECIAL_KICK_KEY, PlayerManager::ANIM_SPECIAL_KICK);		//特殊攻撃(対上方向)
	animNames_.emplace(ULTIMATE_KEY, PlayerManager::ANIM_ULTIMATE);				//必殺技
}

void PlayerAttack::RegisterComboData(void)
{
	comboRouteInfos_.emplace(PUNCH_FIRST_KEY, ComboRouteInfo{ ATTACK_TYPE::PUNCH,false,false });	//パンチ初段
	comboRouteInfos_.emplace(PUNCH_SECOND_KEY, ComboRouteInfo{ ATTACK_TYPE::PUNCH,false,false });	//パンチ二段
	comboRouteInfos_.emplace(PUNCH_THIRD_KEY, ComboRouteInfo{ ATTACK_TYPE::PUNCH,false,false });	//パンチ最終段
	comboRouteInfos_.emplace(KICK_FIRST_KEY, ComboRouteInfo{ ATTACK_TYPE::KICK,false,false });		//キック初段
	comboRouteInfos_.emplace(KICK_SECOND_KEY, ComboRouteInfo{ ATTACK_TYPE::KICK,false,false });		//キック二段
	comboRouteInfos_.emplace(KICK_THIRD_KEY, ComboRouteInfo{ ATTACK_TYPE::KICK,false,false });		//キック最終段
	comboRouteInfos_.emplace(SPECIAL_PUNCH_KEY, ComboRouteInfo{ ATTACK_TYPE::PUNCH,false,false });	//特殊攻撃(対横方向)
	comboRouteInfos_.emplace(SPECIAL_KICK_KEY, ComboRouteInfo{ ATTACK_TYPE::KICK,false,false });	//特殊攻撃(対上方向)
	comboRouteInfos_.emplace(ULTIMATE_KEY, ComboRouteInfo{ ATTACK_TYPE::ULTIMATE,false,false });		//必殺技
}

void PlayerAttack::LoadAttackDirectionData(void)
{
	Resource res = ResourceManager::GetInstance().Load(ResourceManager::SRC::PLAYER_ATTACK_DIRECTION_DATA_JSON);
	directionNames_.at(PUNCH_FIRST_KEY).detail = res.GetData<AttackDirectionData>(PUNCH_FIRST_KEY);	//パンチ初段
	directionNames_.at(PUNCH_SECOND_KEY).detail = res.GetData<AttackDirectionData>(PUNCH_SECOND_KEY);	//パンチ二段
	directionNames_.at(PUNCH_THIRD_KEY).detail = res.GetData<AttackDirectionData>(PUNCH_THIRD_KEY);	//パンチ最終段
	directionNames_.at(KICK_FIRST_KEY).detail = res.GetData<AttackDirectionData>(KICK_FIRST_KEY);		//キック初段
	directionNames_.at(KICK_SECOND_KEY).detail = res.GetData<AttackDirectionData>(KICK_SECOND_KEY);	//キック二段
	directionNames_.at(KICK_THIRD_KEY).detail = res.GetData<AttackDirectionData>(KICK_THIRD_KEY);		//キック最終段
	directionNames_.at(SPECIAL_PUNCH_KEY).detail = res.GetData<AttackDirectionData>(SPECIAL_PUNCH_KEY);//パンチ特殊
	directionNames_.at(SPECIAL_KICK_KEY).detail = res.GetData<AttackDirectionData>(SPECIAL_KICK_KEY);	//キック特殊
	directionNames_.at(ULTIMATE_KEY).detail = res.GetData<AttackDirectionData>(ULTIMATE_KEY);			//必殺技
}

void PlayerAttack::ApplyAttackColliderSettings(void)
{
	power_ = currentData_.power;	//攻撃力変更

	Sphere* attackSphere = colliders_[0]->GetGeometry<Sphere>();
	
	if (attackSphere != nullptr) {
		attackSphere->SetRadius(currentData_.radius);		//半径変更
		attackSphere->SetBroudRadius(currentData_.radius);	//前判定用半径変更
	}

	//攻撃マネージャー
	auto& atkMng = AttackManager::GetInstance();

	std::shared_ptr<AttackData> data = std::make_shared<AttackData>(currentData_);	//データをポインタ化
	atkMng.SetAttackData(AttackManager::ATTACK_TYPE::P_ATTACK, data);				//データを渡す
	atkMng.ResetTargetColList(colliders_[0]);
}

void PlayerAttack::ResetCombo(void)
{
	currentAttackName_ = KICK_THIRD_KEY;	//現在の攻撃アニメーション登録名をリセット
	nextAttackName_ = "";	//次の攻撃アニメーション登録名をリセット
	comboReset_ = true;		//コンボリセットフラグを立てる

	ResetComboRoute();	//コンボルートのリセット
}

void PlayerAttack::ResetComboRoute(void)
{
	for (auto& info : comboRouteInfos_) {
		info.second.isUsed = false;	//使用フラグリセット
	}
}

void PlayerAttack::Draw(void)
{
#ifdef _DEBUG
	//デバッグ表記
	DrawDebug();
#endif
	//コンボ分岐の表記
	DrawComboRoute();
}

void PlayerAttack::DrawSpecialAttack(void)
{

}

void PlayerAttack::Release(void)
{
}

void PlayerAttack::HitCollider(std::weak_ptr<Collider> _col)
{
}

const bool PlayerAttack::ReserveAttack(const ATTACK_TYPE& _type)
{
	//次の攻撃の予約
	//例外
	if (_type == ATTACK_TYPE::MAX) {
		return false;	//攻撃予約なし
	}

	std::string nextAttackKey = data_[currentAttackName_].nextAttacks[static_cast<int>(_type)];

	//次の攻撃が設定されていない場合(コンボの最終段、または引数タイプに派生がない)
	if (nextAttackKey == "") {
		//初段はパンチのみのため、入力がパンチだった場合,それに設定
		if (_type == ATTACK_TYPE::PUNCH) {
			nextAttackName_ = PUNCH_FIRST_KEY;	//初段のパンチ設定
		}
		//キックの場合
		else {
			nextAttackName_ = "";	//空を設定(初段にキックを許さない)
		}
	}
	//次の攻撃が設定されている場合
	else {
		nextAttackName_ = nextAttackKey;	//次の攻撃アニメーション登録名の設定
	}
	
	return true;
}

const bool PlayerAttack::ReserveAttackSpecial(const ATTACK_TYPE& _type)
{
	//特殊攻撃の予約
	//例外
	if (_type == ATTACK_TYPE::MAX) {
		return false;	//攻撃予約なし
	}

	if (_type == ATTACK_TYPE::PUNCH) {
		nextAttackName_ = SPECIAL_PUNCH_KEY;
	}
	else if (_type == ATTACK_TYPE::KICK) {
		nextAttackName_ = SPECIAL_KICK_KEY;
	}

	return true;
}

const bool PlayerAttack::ReserveAttackUltimate(void)
{
	nextAttackName_ = ULTIMATE_KEY;
	return true;
}

void PlayerAttack::Attack(void)
{
	debugColor_ = PUNCH_COLOR;
	if (nextAttackName_ == "") {
		return;	//次の攻撃が設定されていない
	}

	if (nextAttackName_ == startAttackAnimName_) {
		ResetComboRoute();	//コンボルートリセット
	}

	colliders_[0]->SetUseThis(false);		//コライダーの無効化
	currentData_ = data_[nextAttackName_];
	
	currentAttackName_ = nextAttackName_;	//現在の攻撃アニメーション登録名の設定
	ApplyAttackColliderSettings();			//情報適用;
	isAttacking_ = true;					//攻撃状態の有効化
	comboRouteInfos_[currentAttackName_].isUsed = true;	//コンボルートの使用フラグを立てる
}

const PlayerAttack::AttackAnimationInfo PlayerAttack::GetCurrentAttackAnimInfo(void) const
{
	if (nextAttackName_ == "") {
		return AttackAnimationInfo();	//攻撃なし
	}

	AttackAnimationInfo ret;

	//使用するアニメーションと再生速度を格納
	ret.name = animNames_.at(currentAttackName_);
	ret.speed = data_.at(currentAttackName_).animationSpeed;

	return	ret;
}

const PlayerAttack::AttackAnimationInfo PlayerAttack::GetNextAttackAnimInfo(void) const
{
	if(nextAttackName_ == "") {
		return AttackAnimationInfo();	//攻撃なし
	}

	AttackAnimationInfo ret;

	//使用するアニメーションと再生速度を格納
	ret.name = animNames_.at(nextAttackName_);
	ret.speed = data_.at(nextAttackName_).animationSpeed;

	return	ret;
}

const PlayerAttack::AttackDirectionInfo& PlayerAttack::GetNextAttackDirectionInfo(void) const
{
	if (nextAttackName_ == "") {
		return AttackDirectionInfo();	//攻撃なし
	}
	return directionNames_.at(currentAttackName_);
}

const std::string& PlayerAttack::GetCurrentAttackKnockBackType(void) const
{
	return currentData_.knockBackType;
}

const bool PlayerAttack::IsAttacking(void) const
{
	return isAttacking_;
}

const bool PlayerAttack::IsEnableCollier(void) const
{
	return colliders_[0]->IsUseThis();
}

void PlayerAttack::FinishAttack(void)
{
	currentData_ = {};					//攻撃データ初期化
	colliders_[0]->SetUseThis(false);	//コライダの無効化
	isAttacking_ = false;				//攻撃状態の解除
	comboReset_ = false;				//コンボリセット
	comboResetCounter_ = 0;				//カウンターリセット
}
