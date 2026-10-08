#include "../../../../pch.h"
#include"../../../../Manager/Generic/SceneManager.h"
#include"../../../../Manager/GameSystem/AttackManager.h"
#include"../../../../Manager/GameSystem/ComboManager.h"
#include"../../../../Manager/Decoration/EffectManager.h"
#include "../../Player/ToJson/PlayerAttackData.h"
#include "../../../Common/Geometry/Model.h"
#include "../../../Common/Geometry/Sphere.h"
#include "../State/EnemyStaggerState.h"
#include "../State/EnemyLaunchState.h"
#include "../State/EnemySlamState.h"
#include "../State/EnemyPushBackState.h"
#include "../State/EnemyBlowAwayState.h"
#include "../EnemyBase.h"
#include "EnemyOnHit.h"

EnemyOnHit::EnemyOnHit(EnemyBase& _parent)
	: parent_(_parent)
	, cnt_(FLT_MAX)
{
	//状態生成関数
	createState_.emplace("Stagger", &EnemyOnHit::CreateStagger);
	createState_.emplace("Launch", &EnemyOnHit::CreateLaunch);
	createState_.emplace("Slam", &EnemyOnHit::CreateSlam);
	createState_.emplace("PushBack", &EnemyOnHit::CreatePushBack);
	createState_.emplace("BlowAway", &EnemyOnHit::CreateBlowAway);
}

EnemyOnHit::~EnemyOnHit(void)
{
}

void EnemyOnHit::CalcDamage(const std::weak_ptr<Collider> _col)
{
	//終了なら無視
	if (parent_.IsFade())return;

	//攻撃マネージャー
	auto& atkMng = AttackManager::GetInstance();

	//ヒット者
	const auto& col = _col.lock();

	//自身のコライダ
	const auto& myCol = parent_.GetColliders()[0];

	//本体にあたってないので無視
	if (!myCol->IsHit())return;

	//当たれるか
	if (!atkMng.IsCanHit(col, myCol)) return;

	//攻撃情報
	const auto& atkData = atkMng.GetAttackData(_col, myCol).lock();
	auto data = dynamic_pointer_cast<AttackData>(atkData);

	//攻撃間隔
	if (data->hitInterval > cnt_ && data->isMultiHit)
	{
		//カウンタ
		auto& scnMng = SceneManager::GetInstance();
		cnt_ += scnMng.GetScaleUpdateSpeedRate(scnMng.GetDeltaTime());
		return;
	}

	//ここからヒット処理

	//リセット
	cnt_ = 0.0f;

	//攻撃終了へ
	parent_.ChangeAction(ENEMY_ACTION::ATTACK_END);

	//回転情報
	const Quaternion quaRot = col->GetGeometry().GetColRot();

	//吹っ飛び(相手の向いている方向)
	VECTOR blowPow = quaRot.GetForward();

	//ダメージ処理
	parent_.Damage(data->power);

	//ダメージ状態
	std::string knockback = data->knockBackType;
	parent_.ChangeState((this->*createState_[knockback])(blowPow));

	//コンボカウント
	ComboManager::GetInstance().AddCombo();
}

void EnemyOnHit::HitPlayer(const std::weak_ptr<Collider> _col)
{
}

void EnemyOnHit::HitPlayerAttack(const std::weak_ptr<Collider> _col)
{
	//ダメージ処理
	CalcDamage(_col);
}

void EnemyOnHit::HitEnemy(const std::weak_ptr<Collider> _col)
{
	//相手のコライダ
	const auto& hitCol = _col.lock();

	//相手の形状
	const auto& geo = hitCol->GetGeometry();

	//相手側の接触情報
	const auto& result = geo.GetHitResult();

	//自分のコライダの方が優先度が低い場合は無視して、多重に押し戻さないようにする
	if (parent_.GetColliders()[0].get() > hitCol.get())return;

	//自分の法線方向なので相手側の逆
	VECTOR myNormal = VScale(result.normal, -1.0f);

	//移動後座標
	VECTOR movedPos = parent_.GetMovedPos();

	//戻す移動量
	VECTOR movePow = VSub(movedPos, parent_.GetPos());

	//接触地点に戻す
	movedPos = VAdd(movedPos, VScale(movePow, result.t));

	//めり込み防止用にもう少し戻す
	movedPos = VAdd(movedPos, VScale(myNormal, 0.01f));

	//移動量分も押し戻す
	movedPos = VAdd(movedPos, VScale(movePow, -1.0f));

	//残りをスライドさせる
	float remain = 1.0f - result.t;
	VECTOR slide = VSub(movePow, VScale(myNormal, VDot(movePow, myNormal)));
	parent_.SetMovedPos(VAdd(movedPos, VScale(slide, remain)));
}

void EnemyOnHit::HitEnemyAttack(const std::weak_ptr<Collider> _col)
{
}

void EnemyOnHit::HitStage(const std::weak_ptr<Collider> _col)
{
	//各コライダ
	auto& mainCol = parent_.GetColliders()[0];

	//相手コライダ
	const auto& hitCol = _col.lock();

	//相手モデル
	const auto& model = hitCol->GetGeometry<Model>();
	const int hitNum = model->GetHitInfo().HitNum;

	//自身の球
	Sphere& mainSphere = dynamic_cast<Sphere&>(mainCol->GetGeometry());
	float radius = mainSphere.GetRadius();

	//移動後座標
	VECTOR pos = parent_.GetMovedPos();
	
	//法線の合計
	VECTOR totalNormal = Utility::VECTOR_ZERO;

	//最大深度
	float maxDepth = 0.0f;

	for (int i = 0; i < hitNum; i++)
	{
		//当たったポリゴン
		const auto& dim = mainSphere.GetHitInfo().Dim[i];

		//当たった座標
		VECTOR hitPos = dim.HitPosition;

		//法線
		VECTOR normal = VNorm(dim.Normal);

		//深度
		float depth = radius - VDot(normal, VSub(pos, hitPos));

		//めり込んでるなら
		if (depth > 0.0f)
		{
			//加算
			totalNormal = VAdd(totalNormal, normal);
			
			//深さ更新
			maxDepth = std::max(maxDepth, depth);

		}
	}

	// 押し戻し
	if (maxDepth > 0.0f)
	{
		VECTOR N = VNorm(totalNormal);
		pos = VAdd(pos, VScale(N, maxDepth));
	}

	//上ベクトルに大きく戻されているなら地面と判断
	if (totalNormal.y > 0.9f && parent_.GetGravityPow().y < 0.0f)
	{
		parent_.SetGrounding(true);
	}

	//位置設定
	parent_.SetMovedPos(pos);
}

std::unique_ptr<EnemyStateBase> EnemyOnHit::CreateStagger(const VECTOR& _vec)
{
	return std::make_unique<EnemyStaggerState>(_vec);
}

std::unique_ptr<EnemyStateBase> EnemyOnHit::CreateLaunch(const VECTOR& _vec)
{
	return std::make_unique<EnemyLaunchState>(_vec);
}

std::unique_ptr<EnemyStateBase> EnemyOnHit::CreateSlam(const VECTOR& _vec)
{
	return std::make_unique<EnemySlamState>(_vec);
}

std::unique_ptr<EnemyStateBase> EnemyOnHit::CreatePushBack(const VECTOR& _vec)
{
	return std::make_unique<EnemyPushBackState>(_vec);
}

std::unique_ptr<EnemyStateBase> EnemyOnHit::CreateBlowAway(const VECTOR& _vec)
{
	return std::make_unique<EnemyBlowAwayState>(_vec);
}
