#pragma once

//定義
//シーン遷移::メインシーンの切り換え
//シーン移動::サブシーンとの移動のやり取り

class SceneBase
{

public:
#pragma region サブシーン共通定数
	//一枚絵
	static constexpr float IMG_EX_SB_SCENE = 0.8f;			//画像拡大率

	//ボタン
	//拡大縮小
	static constexpr float BTN_EX_MAX_SB_SCENE = 0.3f;		//上限
	static constexpr float BTN_EX_MIN_SB_SCENE = 0.2f;		//下限
	static constexpr float BTN_EX_ACC_SB_SCENE = 0.01f;	//スピード
	//位置調整
	static constexpr float BTN_DIFF_X_SB_SCENE = 250.0f;		//X
	static constexpr float BTN_DIFF_Y_SB_SCENE = 100.0f;		//Y
#pragma endregion

	// コンストラクタ
	SceneBase(void);

	// デストラクタ
	virtual ~SceneBase(void) = 0;

	// 初期化処理
	virtual void Init(void) = 0;
	virtual void InitSound(void) = 0;
	virtual void InitEffect(void) = 0;

	// 更新ステップ
	virtual void Update(void) = 0;

	// 描画処理
	virtual void Draw(void) = 0;

	//UI描画(ゲーム大祭用)
	virtual void DrawUI(void);

	// 解放処理
	virtual void Release(void) = 0;

	//状況回帰(シーン移動後の状況用)
	virtual void Reset(void) = 0;
};
