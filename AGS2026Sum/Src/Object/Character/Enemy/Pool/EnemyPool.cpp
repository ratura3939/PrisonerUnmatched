#include "../../../../pch.h"
#include "../EnemyManager.h"
#include "../EnemyBase.h"
#include "EnemyPool.h"

EnemyPool::EnemyPool(void)
{
}

EnemyPool::~EnemyPool(void)
{
}

void EnemyPool::Load(void)
{
	factory_.Load();
}

void EnemyPool::Release(void)
{
	//参照ポインタの削除
	activeEnemys_.clear();
	for (auto& inactiveEnemys : inactiveEnemys_)
	{
		inactiveEnemys.clear();
	}

	//敵の削除(持ち主自体が消えるのでそのまま消去)
	allEnemys_.clear();
}

EnemyBase* EnemyPool::Spawn(const ENEMY_TYPE& _type)
{
	//int変換
	int type = static_cast<int>(_type);

	//生成済みの敵でもう死んでいるものがいるならそれを再利用する
	if (!inactiveEnemys_[type].empty())
	{
		//再利用する敵のポインタ
		EnemyBase* enemy = inactiveEnemys_[type].back();

		//再初期化(Loadは済んでいるので行わない)
		enemy->Init();

		//動的ステータスの初期化
		enemy->InitRunTimeParameter(factory_.GetParam(_type));

		//生存リストの末尾に追加するのでその添え字を設定
		enemy->SetActiveIndex(static_cast<int>(activeEnemys_.size()));

		//再利用する敵を生存リストの末尾に追加
		activeEnemys_.push_back(enemy);

		//非稼働中の敵のリストから再利用する敵を削除
		inactiveEnemys_[type].pop_back();

		//再利用した敵のポインタを返す
		return enemy;
	}

	//再利用できる敵がいないなら新たに生成する
	std::unique_ptr<EnemyBase> enemy = factory_.CreateNewEnemy(_type);

	//生成した敵の添え字を設定
	enemy->SetActiveIndex(static_cast<int>(activeEnemys_.size()));

	//生成した敵の参照用生ポインタ
	EnemyBase* enemyPtr = enemy.get();

	//全体の敵リストに追加
	allEnemys_.push_back(std::move(enemy));

	//生存リストの末尾に追加
	activeEnemys_.push_back(enemyPtr);

	//生成した敵のポインタを返す
	return enemyPtr;
}

void EnemyPool::Remove(EnemyBase* _enemy)
{
	//死亡した敵の添え字
	int index = _enemy->GetActiveIndex();

	//int変換
	int type = static_cast<int>(_enemy->GetType());

	//末尾の敵が死亡したならスワップを行わない
	if (index != activeEnemys_.size() - 1)
	{
		//生存リストの末尾の敵
		EnemyBase* backEnemy = activeEnemys_.back();

		//死亡した敵と生存リストの末尾の敵を入れ替える
		std::swap(activeEnemys_[index], activeEnemys_.back());

		//入れ替えた生存中の敵の添え字を更新する
		backEnemy->SetActiveIndex(index);
	}

	//状態関係の終了処理
	_enemy->StateEnd();

	//死亡した敵のコライダを無効化する
	_enemy->DisableHitCollider();

	//死亡した敵を非稼働中の敵のリストに追加
	inactiveEnemys_[type].push_back(_enemy);

	//死亡した敵を稼働中の敵のリストから削除
	activeEnemys_.pop_back();
}