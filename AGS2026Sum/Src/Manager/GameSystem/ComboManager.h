#pragma once
#include "../../Common/Singleton.h"

class ComboManager : public Singleton<ComboManager>
{
	//シングルトン化のため、Singletonクラスをフレンドクラスに指定
	friend class Singleton<ComboManager>;

public:
	enum class NUMBER_EX_STATE {
		NONE
		,UP
		,DOWN
	};

	//初期化
	void Init(void)override;

	//コンボ加算
	void AddCombo(void);

	//更新
	void Update(void);

	//描画
	void Draw(void);

	//コンボリセット
	void ResetCombo(void);

	//ヒット数リセット
	void ResetHitCount(void) { hitCount_ = 0; }

	//コンボ数の取得
	const int GetComboCount(void)const { return comboCount_; }

	//ヒット数の取得
	const int GetHitCount(void)const { return hitCount_; }

private:

	//コンボUIの描画
	void DrawComboUI(void);

	//状態の変換
	void ChangeState(const NUMBER_EX_STATE& _nextState);

	//増加・減少更新
	void UpdateExRateIncrease(void);
	void UpdateExRateDecrease(void);

	//デバッグ用
	void DrawDebug(void)const;

	//コンボの最大受付時間
	static constexpr float COMBO_MAX_TIME = 5.0f;

	//コンボ数
	int comboCount_;

	//コンボタイマー
	float comboTimer_;

	//ヒット数
	int hitCount_;

	//数字画像
	int* numberImages_;

	//「COMBO」画像
	int comboStringImage_;

	//画像拡大率
	float imageEx_;

	//状態
	NUMBER_EX_STATE numberState_;

	//更新
	using ExRate_f = void(ComboManager::*)(void);
	ExRate_f updateExRate_;

	//コンストラクタ
	ComboManager(void);

	//デストラクタ
	~ComboManager(void)override;
};

