#pragma once
#include<string>
#include<memory>
#include<DxLib.h>
#include"../../../Common/Quaternion.h"
#include"PlayerAttack.h"
#include"../CharacterBase.h"
#include"../../../Manager/Decoration/EffectManager.h"
#include"../../../Manager/Decoration/SoundManager.h"

class Game;
class EnemyManager;
class PlayerChara;
class AttackCommandInfo;

class PlayerManager
{
public:
	struct AnimationEvent {
		SoundManager::SOUND_NAME seName;	//効果音
		EffectManager::EFFECT_NAME efcName;	//エフェクト
		float timing;						//タイミング
	};

#pragma region アニメーション登録名
	static const std::wstring ANIM_IDLE;		//待機
	static const std::wstring ANIM_RUN;			//移動
	static const std::wstring ANIM_DAMAGE;		//ダメージ
	static const std::wstring ANIM_FIRST_PUNCH;	//初回パンチ
	static const std::wstring ANIM_SECOND_PUNCH;//二回目パンチ
	static const std::wstring ANIM_THIRD_PUNCH;	//三回目パンチ
	static const std::wstring ANIM_MIDDLE_KICK;	//中段キック(二段目を強派生)
	static const std::wstring ANIM_HIGH_KICK;	//上段キック(三段目を強派生)
	static const std::wstring ANIM_FINSH_KICK;	//最終段キック（パンチ三回目後、強派生）
	static const std::wstring ANIM_SPECIAL_PUNCH;	//特殊攻撃（パンチ派生）
	static const std::wstring ANIM_SPECIAL_KICK;	//特殊攻撃（キック派生）
	static const std::wstring ANIM_ULTIMATE;			//必殺技
	static const std::wstring ANIM_ULTIMATE_TEST;	//必殺技（試し用）
#pragma endregion

	PlayerManager(Game& _gameScene);
	~PlayerManager(void);

	void Init(void);
	void Update(void);
	void Draw(void);
	void DrawUI(void);
	void DrawNormalDepth(void);
	void Release(void);

	//位置・回転取得
	const VECTOR& GetPos(void)const;		//座標
	const VECTOR& GetFocusPos(void);		//注視点
	const Quaternion& GetQua(void);			//回転
	//const VECTOR& GetFocusPoint(void);	//注視点

	const bool IsAlive(void)const;

	//現在の攻撃の吹っ飛び方取得
	const CharacterBase::KNOCKBACK_TYPE GetCurrentKnockBackType(void)const;

	//アニメーションの更新速度設定
	void SetAnimSpeedPercent(const float _percent);

	//特殊攻撃準備フラグ設定
	void SetIsSpecialReady(const bool _flag) { isSpecialAttackReady_ = _flag; }

	//必殺技の準備フラグ設定
	void SetIsUltimateReady(const bool _flag) { isUltimateReady_ = _flag; }

	//攻撃ボタン説明の消去
	void DisappearAttackCommandInfo(void);

	//必殺技ボタン説明の消去
	void DisappearUltimateCommandInfo(void);

private:
	void UpdateAnimationEvent(void);		//アニメーション経過によるイベント
	void UserInput(void);					//入力受付

	void Attack(void);		//攻撃処理
	void SetAttackStateForCharacter(void);	//攻撃に関する状態をキャラクターに反映
	const bool ReserveAttack(PlayerAttack::ATTACK_TYPE _type);			//攻撃判定の設定処理(返り値　true=成功/false=失敗)
	const bool ReserveAttackSpecial(PlayerAttack::ATTACK_TYPE _type);	//攻撃判定の設定処理(返り値　true=成功/false=失敗)
	void SettingUltimateCamera(void);							//必殺技のカメラ設定

	void TryPlaySoundAtCurrentAttack(const PlayerAttack::AttackDirectionInfo& _info,const float _animProgressRate);		//効果音再生トライ
	void TryPlayEffectAtCurrentAttack(const PlayerAttack::AttackDirectionInfo& _info, const float _animProgressRate);	//エフェクト再生トライ

	Game& scene_;	//ゲームクラス参照
	VECTOR focusPos_;	//注視点

	std::shared_ptr<PlayerChara> character_;	//キャラクター
	std::unique_ptr<PlayerAttack> attack_;		//攻撃
	std::unique_ptr<AttackCommandInfo> commandInfo_;	//攻撃方法説明(特殊・必殺)

	bool isEnableAttackInput_;	//攻撃入力を受け付ける状態か
	bool isForcePlayAnim_;		//強制再生させるか(攻撃の初段のみ強制再生)
	bool isNoBlendPlayAnim_;	//ブレンドなしでアニメーションを再生させるか

	bool isSpecialAttackReady_;	//特殊攻撃の準備ができているか
	bool isEnableSpecial_;		//特殊攻撃の準備への状態遷移を許可するか
	bool isUltimateReady_;		//必殺技の受付を許可するか
	bool isEnableUltimate_;		//必殺技の発動を許可するか

	bool isPlaySoundAtCurrentAttack_;	//現在の攻撃の演出が完了したか
	bool isPlayEffectAtCurrentAttack_;	//現在の攻撃のエフェクトの再生が完了したか
};

