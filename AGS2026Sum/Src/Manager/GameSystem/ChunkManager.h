#pragma once
#include"../../Common/Singleton.h"

class EnemyGroup;

class ChunkManager : public Singleton<ChunkManager>
{
	//シングルトン化のため、Singletonクラスをフレンドクラスに指定
	friend class Singleton<ChunkManager>;

public:

	//デバッグ描画
	void DebugDraw(void) const;

	//敵の登録
	void AddEnemyGroup(EnemyGroup* _enemyGroup);

	//敵の削除
	void RemoveEnemyGroup(EnemyGroup* _enemyGroup);

	//チャンク移動
	void MoveEnemyGroup(EnemyGroup* _enemyGroup);

	//チャンク内の敵の取得
	const std::vector<EnemyGroup*>& GetEnemyGroupInChunk(const VECTOR& _pos) const;

	/// <summary>
	/// 指定座標から指定セル分の範囲の敵グループの取得
	/// </summary>
	/// <param name="_enemyGroups">格納用敵グループ配列</param>
	/// <param name="_pos">基準となる座標</param>
	/// <param name="_cellRange">取得する範囲のセル数</param>
	void GetEnemyGroupsInRangeChunk(std::vector<EnemyGroup*>& _enemyGroups, const VECTOR& _pos, const int _cellRange) const;

private:

	//セルの大きさ
	static constexpr float CELL_SIZE = 1000.0f;

	//セル数
	static constexpr int CELL_NUM = 100;

	//チャンクの数
	static constexpr int CHUNK_X = static_cast<int>(CELL_SIZE / CELL_NUM);		//チャンクのXの数
	static constexpr int CHUNK_Z = static_cast<int>(CELL_SIZE / CELL_NUM);		//チャンクのZの数

	//コンストラクタ
	ChunkManager();

	//デストラクタ
	~ChunkManager()override;

	//削除処理
	void Destroy(void)override;

	//チャンクのインデックスを計算
	int GetChunkIndex(const VECTOR& _pos) const;

	//チャンクごとの敵の管理
	std::array<std::vector<EnemyGroup*>, CHUNK_X * CHUNK_Z> chunkEnemyMap_;
};

