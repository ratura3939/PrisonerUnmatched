#pragma once
#include "../SceneBase.h"
#include<memory>
#include<string>
#include"../../Object/Character/Enemy/Info/StageEnemyData.h"

class StageManager;
class PlayerManager;
class EnemyManager;
class AttackManager;
class CollisionManager;

class PixelMaterial;
class PixelRenderer;

class Game :
    public SceneBase
{
public:
	static constexpr int CAMERA_DIRECTION_NUM = 2;			//カメラ演出における移動回数

	/// <summary>
	/// ボスの演出
	/// </summary>
	enum class BOSS_DIRECTION {
		NONE,
		POST_EFFECT,	//ポストエフェクト
		SHAKE_SCREEN,	//画面揺れ
		CAMERA_MOVE,	//カメラ移動
		END
	};

	/// <summary>
	/// ポストエフェクトの種類
	/// </summary>
	enum class ACTION_DIRECTION {
		NOMAL,
		BLUR,
		JUST_DODGE,
		SCAN_LINE,
		END
	};

	//ゲームの進行度
	enum class GAME_PROGRESS {
		TUTORIAL,	//チュートリアル
		STAGE_1,	//ステージ１
		MAX
	};

	//カメラ自動移動状況
	enum class CAMERA_MOVE_SITUATION {
		NONE
		,ULTIMATE	//必殺技
		,NEXT_STAGE	//次のステージへ
	};

	Game(void);
	~Game(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;
	void DrawUI(void)override;

	void Release(void) override;
	void Reset(void)override;

	//ボス出現最初の処理用
	void StartBossFaze(void);

	//ブラー入れるか入れないか
	void ChangeActionDirec(const ACTION_DIRECTION _direc);	

	//スロー演出開始
	void StartSlow(void);
	//スロー終了
	void EndSlow(void);	

	//カメラのゴール付近滞在時間
	void SetCameraStayTimeAtAutoMove(const float _time) { cameraGoalStayTime_ = _time; }

	/// <summary>
	/// カメラ自動移動後の独自処理設定
	/// </summary>
	/// <param name="_situation">状況</param>
	void SetProcessingAfterCameraAutoMove(const CAMERA_MOVE_SITUATION& _situation);

private:
	//各初期化
	void InitSound(void)override;
	void InitEffect(void)override;
	void InitShader(void);

	//各種更新
	void GameUpdate(void);			//ゲーム通常

	//各種描画処理(ポストエフェクト)
	void DrawEdge(void);	//エッジ描画

	//進行度ごとの更新
	void UpdateTutorial(void);
	void UpdateStage1(void);

	//切り換え終了時の処理
	void FinishSwitchBgm(void);
	
	//２ステージ目の開始
	void StartNextStage(void);

	//デバッグ描画
	void DrawDebug(void);

	//変数
#pragma region インスタンス
	std::unique_ptr<PlayerManager>player_;			//プレイヤー
	std::unique_ptr<EnemyManager>enemy_;			//敵
	std::unique_ptr<StageManager>stage_;			//ステージ
#pragma endregion

#pragma region 関数ポインタ
	//更新関数
	using Update_f = void(Game::*)(void);
	using DirecUpdate_f = bool(Game::*)(void);
	Update_f update_;			//通常・演出の二つを管理

	//進行度の更新
	using UpdateProgress_f = void(Game::*)(void);
	UpdateProgress_f updateProgress_;

	//カメラ演出後処理
	using CameraMoveAfter_f = void(Game::*)(void);
	CameraMoveAfter_f processingAfterCameraAutoMove_;
#pragma endregion

#pragma region shader関連
	std::unique_ptr<PixelMaterial>edgeMaterial_;	//エッジ描画用マテリアル
	std::unique_ptr<PixelRenderer>edgeRender_;	//エッジ描画用レンダラー
	int normalDepthScreen_;	//法線・深度描画用スクリーン

	bool isDrawPostEffect_;	//ポストエフェクトをかけるか
#pragma endregion


#pragma region その他変数
	//スロー演出
	bool isSlowEffect_;	//ON/OFFフラグ
	int slowCnt_;		//カウンタ

	//BGM
	std::string nowBgmStr_;		//現在のBGM
	std::string switchBgmStr_;	//切り替え後のBGM
	int nextBgmVol_;			//音量調整用(BGM切り替え時に使用)
	bool switchBgm_;			//切り換え開始フラグ

	BOSS_DIRECTION direcState_;		//ボス演出管理
	int direcCnt_;					//演出に関わるカウンタ

	//カメラの演出用
	VECTOR cameraMoveStartPos_;							//初期位置
	VECTOR cameraMoveGoalPos_[CAMERA_DIRECTION_NUM];	//目標位置
	int cameraShakeCollTimeCnt_;	//画面揺れクールタイム
	bool stayCameraShake_;			//画面揺れ待機フラグ true=待機
	int cameraGoalStayTime_;		//カメラ自動移動時、ゴール付近でどれほど滞在するか
	int cameraGoalStayCounter_;		//カメラ自動移動時、ゴール付近滞在カウンター

	//敵
	StageEnemyData stageEnemyData_;	//敵のステージごとデータ

#pragma endregion

	//進行度
	GAME_PROGRESS progress_;

	bool prevInputP_;			//デバッグ用トリガ
	bool isEnemyUpdate_;

	bool isDebug_;
};

