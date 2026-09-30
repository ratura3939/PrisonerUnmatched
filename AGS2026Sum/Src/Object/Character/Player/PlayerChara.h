#pragma once
#include <memory>
#include "../CharacterBase.h"
#include "PlayerOnHit.h"

class ModelMaterial;
class ModelRenderer;

class PlayerChara :
    public CharacterBase
{
    public:
        PlayerChara(void);
        ~PlayerChara(void);

        void Draw(void)override;			//描画
        void Release(void)override;			//解放

		void HitCollider(std::weak_ptr<Collider> _col)override;	//衝突後の処理
        void InputMoveVec(const VECTOR& _inputVec); //移動入力の受付

		void SetIsAttack(const bool _isAttack) { isAttack_ = _isAttack; }	//攻撃状態の設定

        void PlayAnim(const std::wstring& _animName, const float _speed = 1.0f);		//アニメーション再生
		void ForcePlayAnim(const std::wstring& _animName, const float _speed = 1.0f);	//アニメーション強制再生
		void NoBlendPlayAnim(const std::wstring& _animName, const float _speed = 1.0f);	//アニメーション強制再生
		void GetAnimTotalTime(const std::wstring& _animName)const;	//アニメーションの総再生時間を取得

		const float GetCurrentAnimationProgressRate(void)const;	//現在のアニメーションの再生進行度を取得
		const float GetBlendAnimationProgressRate(void)const;	//ブレンド中のアニメーションの再生進行度を取得
		const float GetSpecifiedAnimationProgressRate(const std::wstring& _animName)const;	//指定のアニメーションの再生進行度を取得
		const bool IsFinishAttackAnimation(void)const { return animController_->IsFinishNormalAnim(); }	//通常再生のアニメ（主に攻撃関連）が終了しているか
		const bool IsStartNextAttackAnimation(void)const { return animController_->IsStartNextAnim(); }	//次のアニメ（主に攻撃関連）が開始しているか

    private:
        void DoLoad(void)override;			//読み込み
        void DoInit(void)override;			//初期化
        void DoUpdate(void)override;		//更新
		void InitAnim(void)override;		//アニメーションの初期化

		void Move(void)override;			//移動処理
		void Attack(void)override;			//攻撃処理

		void DrawHP(void);		//HP描画

		std::unique_ptr<ModelMaterial>outlineMaterial_;	//アウトラインマテリアル

        VECTOR inputDir_;       //移動入力方向
        float afterMoveRad_;    //最終的なキャラクター角度
		float moveSpeed_;       //移動速度
		bool isMove_;           //移動しているか
		bool isAttack_;         //攻撃しているか  
		std::wstring useAnim_;  //現在のアニメーション
		PlayerOnHit onHit_;		//当たり判定用
};

