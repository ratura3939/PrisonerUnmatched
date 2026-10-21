#pragma once
#include<DxLib.h>
#include<memory>
#include"../../Common/Quaternion.h"
#include"Collider.h"

class ActorBase
{
public:

	//初期化用
	static constexpr float INIT_MODEL_ROT = 180.0f;	//Unity形式のモデルの形を合わせる用

	//重力定数
	static constexpr float GRAVITY_POW = -0.98f;

	ActorBase(void);
	virtual ~ActorBase(void);

	void Load(void);
	void Init(void);
	void Update(void);
	virtual void Draw(void) = 0;
	virtual void Release(void) = 0;

	const int GetModelID(void)const;

	//位置設定
	void SetPos(const VECTOR& _pos);
	const VECTOR& GetPos(void)const;

	//移動後の位置設定
	void SetMovedPos(const VECTOR& _movedPos);
	const VECTOR& GetMovedPos(void)const;

	//移動後の位置設定
	void SetLocalPos(const VECTOR& _localPos);
	const VECTOR& GetLocalPos(void)const;

	// 各種方向を取得
	const VECTOR& GetForward(void) const;
	const VECTOR& GetBack(void) const;
	const VECTOR& GetRight(void) const;
	const VECTOR& GetLeft(void) const;
	const VECTOR& GetUp(void) const;
	const VECTOR& GetDown(void) const;

	// 対象方向を取得
	const VECTOR GetDir(const VECTOR& _vec) const;

	//所持コライダ
	const std::vector<std::shared_ptr<Collider>>& GetColliders(void);
	//衝突後の処理
	virtual void HitCollider(std::weak_ptr<Collider> _col) = 0;
	//攻撃力の取得
	const float GetPower(void)const;
	//個体名取得
	const std::wstring& GetSpeciesName(void)const;

protected:
	//派生クラス用
	virtual void DoLoad(void) = 0;		//読み込み
	virtual void DoInit(void) = 0;		//初期化
	virtual void DoUpdate(void) = 0;	//更新

	void MakeCollider(std::unique_ptr<Geometry> _geo, const Collider::COL_TAG _tag, const std::set<Collider::COL_TAG> _hitTags = {});	//コライダの生成

	void EnableColliderAtTag(const Collider::COL_TAG& _tag);		//タグによるコライダの有効化
	void DisableColliderAtTag(const Collider::COL_TAG& _tag);		//タグによるコライダの無効化
	void DeleteAllColliders(void);									//コライダの全削除
	void DeleteColliderAtTag(const Collider::COL_TAG& _tag);		//タグによるコライダの削除

#pragma region モデル基礎
	int modelId_;		//モデルID

	VECTOR pos_;		//座標
	VECTOR movedPos_;	//移動後座標
	VECTOR localPos_;	//ローカル座標
	VECTOR scl_;		//モデル大きさ
	VECTOR rot_;		//回転情報(XYZ)

	//各情報の行列か
	MATRIX matScl_;		//大きさ
	MATRIX matRot_;		//回転
	MATRIX matPos_;		//位置

	// 回転
	Quaternion quaRot_;
	Quaternion quaRotOrigin_;
	// ローカル回転
	Quaternion quaRotLocal_;
#pragma endregion

	//個体名
	std::wstring speciesName_;

	std::vector<std::shared_ptr<Collider>> colliders_;	//コライダー
	float power_;			//攻撃力
	VECTOR gravity_;		//重力ベクトル

private:
	void UpdateRotQuat(void);	//基礎情報の更新
	void UpdateGravity(void);	//重力処理
	void SweepColliders(void);	//必要なくなったコライダの削除
};

