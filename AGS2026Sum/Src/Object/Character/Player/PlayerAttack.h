#pragma once
#include "../../Common/ActorBase.h"
#include"../../../Manager/Decoration/SoundManager.h"
#include"../../../Manager/Decoration/EffectManager.h"
#include "ToJson/PlayerAttackData.h"
#include "ToJson/PlayerAttackDirectionData.h"

class PlayerAttack :
    public ActorBase
{
public:
    //攻撃種別
    enum class ATTACK_TYPE {
		PUNCH   //パンチ
		,KICK   //キック
		,ULTIMATE //必殺技
        ,MAX
    };

	struct AttackAnimationInfo {
		std::wstring name;	//アニメーション登録名
		float speed;		//アニメーションの再生速度
	};

	struct ComboRouteInfo {
		ATTACK_TYPE type;	//攻撃種別
		bool isUsed;		//攻撃を行ったか
		bool isDrawed;		//描画したか
	};

	struct AttackDirectionInfo {
		SoundManager::SOUND_NAME seName;
		EffectManager::EFFECT_NAME efcName;
		AttackDirectionData detail;
	};

	static const int ATTACK_LEVEL_MAX = 3;				//攻撃レベルの最大値
	static constexpr float ATTACK_CANCEL_RATE = 0.5f;	//攻撃キャンセル可能割合(アニメーションの進行度)
	static constexpr int COMBO_ELEMENT_TYPE_NUM = 2;	//コンボ表示必要な種類数(パンチとキック)

	static constexpr AttackDirectionInfo INIT_ANIM_DIRECTION_INFO = { SoundManager::SOUND_NAME::MAX, EffectManager::EFFECT_NAME::MAX,{} };	//アニメーション演出初期化用

	PlayerAttack(const VECTOR& _playerPos, const Quaternion& _playerQuaRot);
	~PlayerAttack(void)override;

	void Draw(void)override;
	void DrawSpecialAttack(void);
	void Release(void)override;

	void HitCollider(std::weak_ptr<Collider> _col)override;

	
	/// <summary>
	/// 通常攻撃の予約
	/// </summary>
	/// <param name="_type">攻撃種別</param>
	/// <returns></returns>
	const bool ReserveAttack(const ATTACK_TYPE& _type);

	/// <summary>
	/// 特殊攻撃の予約
	/// </summary>
	/// <param name="_type">攻撃種別</param>
	/// <returns></returns>
	const bool ReserveAttackSpecial(const ATTACK_TYPE& _type);	

	/// <summary>
	/// 必殺技の予約
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	const bool ReserveAttackUltimate(void);

	void Attack(void);			//攻撃開始
	void FinishAttack(void);	//攻撃終了処理

	const AttackAnimationInfo GetCurrentAttackAnimInfo(void)const;	//現在の攻撃アニメーション情報の取得
	const AttackAnimationInfo GetNextAttackAnimInfo(void)const;		//次の攻撃アニメーション情報の取得
	const AttackDirectionInfo& GetNextAttackDirectionInfo(void)const;//次の攻撃のSE/エフェクトに関する情報の取得

	const std::string& GetCurrentAttackKnockBackType(void)const;	//現在の攻撃のノックバックの種類の取得
	
	const bool IsAttacking(void)const;		//攻撃中か
	const bool IsEnableCollier(void)const;	//攻撃判定が有効であるか

	/// <summary>
	/// 攻撃の許可(アニメーションの再生進捗による)
	/// </summary>
	/// <param name="_animRate">アニメーションの再生進捗</param>
	void TryEnableAttackCollider(const float _animRate);

	//デバッグ用
	void DrawDebug(void);

private:
	void DoLoad(void)override;
	void DoInit(void)override;
	void DoUpdate(void)override;

	void DrawComboRoute(void);	//コンボルートの描画
	void DrawComboRouteElement(const std::string& _attackKey, const VECTOR& _pos);	//コンボルートの要素の描画

	void LoadAttackData(void);				//攻撃データの読み込み
	void LoadAttackDirectionData(void);		//演出関連情報の読み込み
	void LoadAttackSound(void);				//効果音などの読み込み
	void LoadAttackEffect(void);			//エフェクトの読み込み

	void RegisterAttackData(void);			//攻撃情報の登録
	void RegisterAttackAnimData(void);		//攻撃アニメーション情報の登録
	void RegisterComboData(void);			//コンボ情報の登録

	void ApplyAttackColliderSettings(void);	//コライダの設定
	void ResetCombo(void);					//コンボのリセット
	void ResetComboRoute(void);				//コンボルートのリセット

	const VECTOR& playerPos_;			//プレイヤーの座標参照
	const Quaternion& playerQuaRot_;	//プレイヤーの回転参照

	std::unordered_map<std::string, AttackData> data_;						//攻撃データ
	std::unordered_map<std::string, std::wstring> animNames_;				//攻撃アニメーション登録名
	std::unordered_map<std::string, ComboRouteInfo> comboRouteInfos_;		//コンボルート情報
	std::unordered_map<std::string, AttackDirectionInfo> directionNames_;		//攻撃効果音登録名

	AttackData currentData_;		//現在の攻撃データ

	std::string currentAttackName_;	//現在の攻撃アニメーション登録名
	std::string nextAttackName_;	//次の攻撃アニメーション登録名

	ATTACK_TYPE latestReserveType_;		//最新に予約された攻撃種別

	bool comboReset_;		//コンボリセットフラグ
	int comboResetCounter_;	//コンボリセットカウンタ
	int* comboElementImages_[COMBO_ELEMENT_TYPE_NUM];	//コンボルート用画像
	int arrowImage_;		//矢印

	std::string startAttackAnimName_;	//攻撃開始アニメーション登録名

	bool isAttacking_;		//攻撃状態であるかの判定

	int debugColor_;	//デバッグ用の色
};

