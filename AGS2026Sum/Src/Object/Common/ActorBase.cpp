#include"../../pch.h"
#include"../../Utility/Utility.h"
#include"../../Manager/GameSystem/CollisionManager.h"
#include "ActorBase.h"

ActorBase::ActorBase(void)
	: modelId_(-1)
	, movedPos_(Utility::VECTOR_ZERO)
	, pos_(Utility::VECTOR_ZERO)
	, localPos_(Utility::VECTOR_ZERO)
	, scl_(Utility::VECTOR_ONE)
	, rot_(Utility::VECTOR_ZERO)
	, matScl_(MGetIdent())
	, matRot_(MGetIdent())
	, matPos_(MGetIdent())
	, quaRot_(Quaternion::Euler(Utility::VECTOR_ZERO))
	, quaRotOrigin_(Quaternion::Euler(Utility::VECTOR_ZERO))
	, quaRotLocal_(Quaternion::Euler(Utility::VECTOR_ZERO))
	, speciesName_(L"")
	, power_(0.0f)
	, gravity_(Utility::VECTOR_ZERO)
{
}

ActorBase::~ActorBase(void)
{
	//持ち主自体が消えるのでそのまま消去
	colliders_.clear();
}

void ActorBase::Load(void)
{
	quaRotLocal_ = Quaternion::Euler(0.0f, Utility::Deg2RadF(INIT_MODEL_ROT), 0.0f);
	DoLoad();
}

void ActorBase::Init(void)
{
	DoInit();
	movedPos_ = pos_;

	//モデルがある時のみ行う
	if (modelId_ != -1) {
		UpdateRotQuat();	//モデルの初期位置を反映
	}
}

void ActorBase::Update(void)
{
	//移動後の座標を影響
	pos_ = movedPos_;
	//派生クラスの更新処理
	DoUpdate();
	//共通処理
	//UpdateGravity();
	if (modelId_ != -1) {
		UpdateRotQuat();
	}
	SweepColliders();
}

const int ActorBase::GetModelID(void) const
{
	return modelId_;
}

void ActorBase::SetPos(const VECTOR& _pos)
{
	pos_ = _pos;
}

const VECTOR& ActorBase::GetPos(void) const
{
	return pos_;
}

void ActorBase::SetMovedPos(const VECTOR& _movedPos)
{
	movedPos_ = _movedPos;
}

const VECTOR& ActorBase::GetMovedPos(void)const
{
	return movedPos_;
}

void ActorBase::SetLocalPos(const VECTOR& _localPos)
{
	localPos_ = _localPos;
}

const VECTOR& ActorBase::GetLocalPos(void) const
{
	return localPos_;
}

const VECTOR& ActorBase::GetForward(void) const
{
	return GetDir(Utility::DIR_F);
}

const VECTOR& ActorBase::GetBack(void) const
{
	return GetDir(Utility::DIR_B);
}

const VECTOR& ActorBase::GetRight(void) const
{
	return GetDir(Utility::DIR_R);
}

const VECTOR& ActorBase::GetLeft(void) const
{
	return GetDir(Utility::DIR_L);
}

const VECTOR& ActorBase::GetUp(void) const
{
	return GetDir(Utility::DIR_U);
}

const VECTOR& ActorBase::GetDown(void) const
{
	return GetDir(Utility::DIR_D);
}

const VECTOR ActorBase::GetDir(const VECTOR& _vec) const
{
	return quaRot_.PosAxis(_vec);
}

const std::vector<std::shared_ptr<Collider>>& ActorBase::GetColliders(void)
{
	return colliders_;
}

const float ActorBase::GetPower(void) const
{
	return power_;
}

const std::wstring& ActorBase::GetSpeciesName(void) const
{
	return speciesName_;
}

void ActorBase::MakeCollider(std::unique_ptr<Geometry> _geo, const Collider::COL_TAG _tag, const std::set<Collider::COL_TAG> _hitTags)
{
	//コライダの生成
	std::shared_ptr<Collider> col = std::make_shared<Collider>(*this, _tag, std::move(_geo), _hitTags);

	//生成したコライダを保存
	colliders_.push_back(col);

	//コライダマネージャに登録
	CollisionManager::GetInstance().AddCollider(col);
}

void ActorBase::EnableColliderAtTag(const Collider::COL_TAG& _tag)
{
	//タグによるコライダの有効化
	for (auto& col : colliders_) {
		if (col->IsContainsTag(_tag)) {
			col->SetUseThis(true);
		}
	}
}

void ActorBase::DisableColliderAtTag(const Collider::COL_TAG& _tag)
{
	//タグによるコライダの無効化
	for (auto& col : colliders_) {
		if (col->IsContainsTag(_tag)) {
			col->SetUseThis(false);
		}
	}
}

void ActorBase::DeleteAllColliders(void)
{
	//コライダの全削除
	for(auto& col : colliders_) {
		col->Kill();
	}
	colliders_.clear();
}

void ActorBase::DeleteColliderAtTag(const Collider::COL_TAG& _tag)
{
	//タグによるコライダの削除
	for (auto& col : colliders_) {
		if (col->IsContainsTag(_tag)) {
			col->Kill();
		}
	}

	//必要なくなったコライダの削除
	SweepColliders();
}

void ActorBase::UpdateRotQuat(void)
{
	// 大きさ
	matScl_ = MGetScale(scl_);

	// 回転
	rot_ = quaRot_.ToEuler();
	matRot_ = quaRot_.ToMatrix();

	// 位置
	matPos_ = MGetTranslate(pos_);

	// 行列の合成
	MATRIX mat = MGetIdent();
	mat = MMult(mat, matScl_);
	Quaternion q = quaRot_.Mult(quaRotLocal_);
	mat = MMult(mat, q.ToMatrix());
	mat = MMult(mat, matPos_);

	// 行列をモデルに判定
	if (modelId_ != -1) {
		MV1SetMatrix(modelId_, mat);
	}
}

void ActorBase::UpdateGravity(void)
{
	//重力処理
	gravity_.y += GRAVITY_POW;
	pos_ = VAdd(pos_, gravity_);
}

void ActorBase::SweepColliders(void)
{
	//死亡したコライダの削除
	std::erase_if(colliders_, [](const std::shared_ptr<Collider>& col) {return col->IsDead();});
}
