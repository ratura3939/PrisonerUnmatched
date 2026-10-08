#include"../../pch.h"
#include"../../Utility/Utility.h"
#include"../../../Manager/Generic/ResourceManager.h"
#include"../../../Manager/GameSystem/ChunkManager.h"
#include "../../../Manager/Decoration/SoundManager.h"
#include"Info/EnemyDefine.h"
#include"Info/StageEnemyData.h"
#include"Pool/EnemyGroupPool.h"
#include"Pool/EnemyPool.h"
#include"EnemyGroup.h"
#include"EnemyBase.h"
#include "EnemyManager.h"

EnemyManager::EnemyManager(const VECTOR& _pPos)
	:playerPos_(_pPos)
{
	//チャンク内の敵の管理用のリストにとりあえず初期確保数分の容量を確保しておく(確保・削除を減らすため)
	oldChunkGroups_.reserve(INIT_CHUNK_GROUP_NUM);
	chunkGroups_.reserve(INIT_CHUNK_GROUP_NUM);
}

EnemyManager::~EnemyManager(void)
{
}

void EnemyManager::Load(void)
{
	//SE
	auto& res = ResourceManager::GetInstance();
	auto& snd = SoundManager::GetInstance();
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_CHARGE_SE, res.Load(ResourceManager::SRC::ENEMY_CHARGE_SE).handleId_);
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_NORMAL_SKILL_SE, res.Load(ResourceManager::SRC::ENEMY_NORMAL_SKILL_SE).handleId_);
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_TACKLE_SKILL_SE, res.Load(ResourceManager::SRC::ENEMY_TACKLE_SKILL_SE).handleId_);
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_JUMP_SKILL_SE, res.Load(ResourceManager::SRC::ENEMY_JUMP_SKILL_SE).handleId_);
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_SKILL_CANCEL_SE, res.Load(ResourceManager::SRC::ENEMY_SKILL_CANCEL_SE).handleId_);
	snd.Add(SoundManager::TYPE::SE, SoundManager::SOUND_NAME::ENEMY_GUARD_BREAK_SE, res.Load(ResourceManager::SRC::ENEMY_GUARD_BREAK_SE).handleId_);

	//敵グループのプールを生成
	enemyGroupPool_ = std::make_unique<EnemyGroupPool>();
	enemyGroupPool_->Load();

	//敵のプールを生成
	enemyPool_ = std::make_unique<EnemyPool>();
	enemyPool_->Load();

	//ボスUIの読み込み
	bossUi_.Load();
}

void EnemyManager::Init(void)
{
	//const int ENEMY_GROUP_NUM = 1;
	//const int MIDDLE_BOSS_GROUP_NUM = 0;

	////敵の生成(デバッグ)
	//for (int i = 0; i < ENEMY_GROUP_NUM;i++)
	//{
	//	CreateEnemyGroup(1,ENEMY_TYPE::NORMAL);
	//	//CreateEnemyGroup(CREATE_NUM);
	//}
	//for (int i = 0; i < MIDDLE_BOSS_GROUP_NUM;i++)
	//{
	//	CreateEnemyGroup(CREATE_NUM,ENEMY_TYPE::MIDDLE_BOSS);
	//}
}

void EnemyManager::Update(void)
{
	//インスタンス
	ChunkManager& chunkMng = ChunkManager::GetInstance();

	//古いのを保存
	oldChunkGroups_ = chunkGroups_;

	//チャンク管理リストを空にする
	chunkGroups_.clear();

	//チャンク管理用のリストを更新
	chunkMng.GetEnemyGroupsInRangeChunk(chunkGroups_, playerPos_, CHUNK_RANGE);

	//加入・離脱処理
	ChankGroupsEnterAndLeave();

	//距離ごとの命令決め
	DecideOrderByDistance();

	//チャンク内のみ更新
	for (auto& group : chunkGroups_)
	{
		//チャンクの移動確認
		chunkMng.MoveEnemyGroup(group);

		//更新
		group->Update();
	}

	//グループの削除処理
	DeleteEnemyGroup();

	//敵の削除処理
	DeleteEnemy();

	//ボスUIの設定
	SetBossUI();
}

void EnemyManager::Draw(void)
{
	//チャンク内のみ描画
	for (auto& group : chunkGroups_)
	{
		group->Draw();
	}

	//ボスUIの描画
	bossUi_.Draw();
}

void EnemyManager::Release(void)
{
	//解放
	enemyGroupPool_->Release();
	enemyGroupPool_.reset();
	enemyPool_->Release();
	enemyPool_.reset();
}

void EnemyManager::CreateEnemyGroup(const int _createNum, const VECTOR& _pos, const ENEMY_TYPE& _leaderType, const EVENT_TYPE& _event)
{
	//生成数の制限
	if (_createNum <= 0)return;

	//グループ
	EnemyGroup* group = enemyGroupPool_->Spawn(_pos);

	//敵の参照用ポインタ
	EnemyBase* enemy = nullptr;

	//リーダー生成
	enemy = enemyPool_->Spawn(_leaderType);

	//イベントの所持
	enemy->SetEventKey(_event);
	enemy->AddEventCount();

	//グループに設定
	Grouping(group, enemy);

	//指定分、敵を生成する
	for (int i = 1; i < _createNum; i++)
	{
		//生成
		enemy = enemyPool_->Spawn(ENEMY_TYPE::NORMAL);

		//イベントの所持
		enemy->SetEventKey(EVENT_TYPE::NONE);
		enemy->AddEventCount();

		//グループに設定
		Grouping(group, enemy);
		enemy->InitWithGroup();
	}

	//座標リセット
	group->ResetPos();

	//グループのチャンク管理用の添え字を設定
	ChunkManager::GetInstance().AddEnemyGroup(group);
}

const int EnemyManager::GetActiveEnemyNum(void) const
{
	//プールがないならそもそも0
	if (!enemyPool_) return 0;

	return static_cast<int>(enemyPool_->GetActiveEnemys().size());
}

void EnemyManager::SetAnimSpeedPercent(const float _percent)
{
	for (auto& enemy : enemyPool_->GetActiveEnemys())
	{
		enemy->SetAnimationSpeedPercent(_percent);
	}
}

void EnemyManager::CreateStageEnemy(const StageEnemyData::AllEnemysInfo& _enemyInfo)
{
	//スポーンエリア
	const auto& spawnArea = _enemyInfo.spawnAreas;

	//エリアの個数
	int areaNum = static_cast<int>(spawnArea.size());

	//ボスグループの生成
	const auto& bossInfos = _enemyInfo.bossInfos;
	for (const auto& bossInfo : bossInfos)
	{
		CreateEnemyGroup(_enemyInfo.enemyNum, bossInfo.pos, bossInfo.bossType, bossInfo.eventType);
	}

	//雑魚グループの生成
	VECTOR randPos;
	for (int i = 0; i < _enemyInfo.groupNum;i++)
	{
		//ランダム
		int randAreaNum = Utility::GetRandomValue(0, areaNum - 1);

		//ランダムのエリア
		const auto& randArea = spawnArea[randAreaNum];

		//ランダムのエリアに設定
		randPos = randArea.center;

		//そこから少しずらす
		randPos.x += Utility::GetRandomValue(static_cast<int>(-randArea.size), static_cast<int>(randArea.size));
		randPos.z += Utility::GetRandomValue(static_cast<int>(-randArea.size), static_cast<int>(randArea.size));

		//生成
		CreateEnemyGroup(_enemyInfo.enemyNum, randPos, ENEMY_TYPE::NORMAL, EVENT_TYPE::NONE);
	}
}

const bool EnemyManager::IsGuardBreak(void)const
{
	//チャンク内のグループ
	for (const auto& group : chunkGroups_)
	{
		//敵
		for (const auto& enemy : group->GetEnemys())
		{
			//ガードブレイクしているか
			if (enemy->IsGuardBreak())
				return true;
		}
	}

	//ガードブレイクしている敵がいなかった
	return false;
}

void EnemyManager::Grouping(EnemyGroup* _group, EnemyBase* _enemy)
{
	//グループに所属させる
	_group->AddEnemy(_enemy);
	_enemy->SetGroup(_group);

	//グループが加入済みなら
	if (_group->IsInChank())
	{
		//チャンクに入った時の処理
		_enemy->OnEnterActiveChank();
	}
}

void EnemyManager::DeleteEnemy(void)
{
	//そもそも敵がいないなら何もしない
	if (!enemyPool_) return;

	//削除する敵のリスト
	std::vector<EnemyBase*> removeEnemys;

	//死亡した敵　または　グループに所属していない敵の削除
	for (auto& activeEnemy : enemyPool_->GetActiveEnemys())
	{
		if (activeEnemy->IsEndState() || !activeEnemy->IsInGroup())
		{
			//削除する敵のリストに追加
			removeEnemys.push_back(activeEnemy);
		}
	}

	//削除する敵のリストにいる敵を削除
	for (auto& removeEnemy : removeEnemys)
	{
		enemyPool_->Remove(removeEnemy);
	}
}

void EnemyManager::DeleteEnemyGroup(void)
{
	//グループが空なら処理しない
	if (!enemyGroupPool_)return;

	//削除するグループのリスト
	std::vector<EnemyGroup*> removeGroups;

	//活動中のグループの数
	int activeGroupNum = static_cast<int>(enemyGroupPool_->GetActiveEnemyGroups().size());

	//グループの削除処理
	for (auto& group : enemyGroupPool_->GetActiveEnemyGroups())
	{
		//非稼働　または　グループが空なら削除
		if (group->IsEmpty() || !group->IsActive() && activeGroupNum > 1)
		{
			//削除するグループのリストに追加
			removeGroups.push_back(group);
		}
	}

	//削除するグループのリストにいるグループを削除
	for (auto& removeGroup : removeGroups)
	{
		//チャンク管理から削除
		ChunkManager::GetInstance().RemoveEnemyGroup(removeGroup);

		//グループが加入済みなら
		if (removeGroup->IsInChank())
		{
			//チャンクから出る処理
			removeGroup->OnLeaveActiveChank();
		}

		//グループに所属している敵をグループから抜けさせる
		enemyGroupPool_->Remove(removeGroup);

		//グループに所属している敵を別グループに再所属させる
		auto& enemys = removeGroup->GetEnemys();
		for (auto& enemy : enemys)
		{
			//グループに所属している敵をグループから抜けさせる
			ReJoinGroups(enemy);
		}
	}
}

void EnemyManager::ReJoinGroups(EnemyBase* _enemy)
{
	//グループ　または　敵が空なら処理しない
	if (!enemyGroupPool_ || !enemyPool_)return;

	//敵グループの末尾
	EnemyGroup* enemyGroupBack = enemyGroupPool_->GetActiveEnemyGroupBack();

	//グループの末尾が空なら処理しない
	if (!enemyGroupBack)return;

	//グループに所属していない敵を別グループに再所属させる
	Grouping(enemyGroupBack, _enemy);
}

void EnemyManager::DecideOrderByDistance(void)
{
	//グループが空なら処理しない
	if (!enemyGroupPool_)return;

	for (auto& group : enemyGroupPool_->GetActiveEnemyGroups())
	{
		//グループが空なら処理しない
		if (group->IsEmpty())continue;

		//プレイヤーからの距離を取得
		float sqrDist = static_cast<float>(Utility::SqrMagnitude(group->GetGroupPos(), playerPos_));

		//プレイヤーから一定距離以上離れているグループは無視する
		if (sqrDist < PLAYER_ATTACK_RADIUS * PLAYER_ATTACK_RADIUS)
		{
			//グループを攻撃準備状態にする
			group->ChangeOrder(GROUP_ORDER::ALERT);
			
			//グループの目標座標をプレイヤー座標に設定
			group->SetGoalPos(playerPos_);
		}
		else if (sqrDist < PLAYER_AIM_RADIUS * PLAYER_AIM_RADIUS)
		{
			//グループを移動状態にする
			group->ChangeOrder(GROUP_ORDER::MOVE);

			//グループの目標座標をプレイヤー座標に設定
			group->SetGoalPos(playerPos_);
		}
		else
		{
			//グループを待機状態にする
			group->ChangeOrder(GROUP_ORDER::STAY);
		}
	}
}

void EnemyManager::ChankGroupsEnterAndLeave(void)
{
	for (auto* group : chunkGroups_)
	{
		//前回いなかったので加入処理
		if (!group->IsInChank())
		{
			group->OnEnterActiveChank();
		}
	}

	for (auto* group : oldChunkGroups_)
	{
		//今回いないので離脱処理
		if (std::find(chunkGroups_.begin(),
			chunkGroups_.end(),
			group) == chunkGroups_.end())
		{
			//そもそもグループが存在していないなら無視
			if (!group->IsActive())continue;
			group->OnLeaveActiveChank();
		}
	}
}

void EnemyManager::SetBossUI(void)
{
	//一番近い距離
	float closestDist = FLT_MAX;
	float dist = 0.0f;
	const EnemyBase* boss = nullptr;

	//チャンク内の比較
	for (const auto& group : chunkGroups_)
	{
		for (const auto& enemy : group->GetEnemys())
		{
			//ボス敵のみ
			if (!enemy->IsBoss())continue;

			//距離
			dist = static_cast<float>(Utility::Distance(enemy->GetPos(), playerPos_));

			//距離比較
			if (dist < closestDist)
			{
				closestDist = dist;
				boss = enemy;
			}
		}
	}

	//比較終了
	if (boss)
	{
		//近いボスを描画
		bossUi_.SetDrawBoss(boss);
	}
	else
	{
		//誰もいない
		bossUi_.SetDrawBoss(nullptr);
	}
}
