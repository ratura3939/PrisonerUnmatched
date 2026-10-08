#include "../../pch.h"
#include"../../Application.h"
#include "../Generic/SceneManager.h"
#include "../Decoration/UIManager2d.h"
#include"../Generic/ResourceManager.h"
#include "ComboManager.h"

namespace {
	//拡大率
	const float EX_RATE_MIN = 0.7f;		//最低値
	const float EX_RATE_MAX = 1.2f;		//最大値
	const float EX_RATE_ACC = 0.05f;	//加速度
}

void ComboManager::Init(void)
{
	//初期化
	ResetCombo();
	ResetHitCount();

	ResourceManager& resM = ResourceManager::GetInstance();
	using SOURCE = ResourceManager::SRC;

	numberImages_ = resM.Load(SOURCE::COMBO_NUMBER_IMG).handleIds_;
	comboStringImage_ = resM.Load(SOURCE::COMBO_STRING_IMG).handleId_;
}

void ComboManager::AddCombo(void)
{
	//コンボ数を加算
	comboCount_++;

	//ヒット数を加算
	hitCount_++;

	//コンボタイマーをリセット
	comboTimer_ = 0.0f;

	//数字の拡大開始
	ChangeState(NUMBER_EX_STATE::UP);
}

void ComboManager::Update(void)
{
	//シーンマネージャー
	auto& scnMng = SceneManager::GetInstance();

	//コンボタイマーを更新
	comboTimer_ += scnMng.GetScaleUpdateSpeedRate(scnMng.GetDeltaTime());

	//数字に動きがある場合
	if (numberState_ != ComboManager::NUMBER_EX_STATE::NONE) {
		//拡大率更新
		(this->*updateExRate_)();
	}

	//コンボタイマーが最大受付時間を超えた場合
	if (comboTimer_ > COMBO_MAX_TIME)
	{
		//コンボをリセット
		ResetCombo();
	}
}

void ComboManager::Draw(void)
{
#ifdef _DEBUG
	//デバッグ
	DrawDebug();
#endif

	//コンボカウンター描画
	DrawComboUI();
}

void ComboManager::ResetCombo(void)
{
	//コンボ数をリセット
	comboCount_ = 0;

	//コンボタイマーをリセット
	comboTimer_ = 0.0f;

	//拡大率リセット
	imageEx_ = EX_RATE_MIN;
}

void ComboManager::DrawComboUI(void)
{
	//コンボ数が存在しない場合処理しない
	if (comboCount_ <= 0) {
		return;
	}

	const int drawPos_Y = 100;			//描画位置Y(共通)
	const int drawStartPosCombo = 50;	//コンボ数最上位桁描画位置

	const double angle = 0.0;			//回転

	std::vector<int>digitNumbers;	//桁それぞれを保持
	int remainingCombo = comboCount_;

	const int maxDigit = 10;	//最大桁数(10桁まで)

	while (remainingCombo > 0) {
		//一の位を取得
		int useNumber = remainingCombo % maxDigit;

		//下位桁から順に保存
		digitNumbers.push_back(useNumber);

		//一桁分削る
		remainingCombo /= maxDigit;
	}

	const int numberDistance = 40;	//数字の描画間隔

	//描画位置
	int numberDrawPos_X = drawStartPosCombo;
	//数字ごとの描画位置差分
	const int numberDrawOffset_X = numberDistance * static_cast<int>(Utility::CalculateRatio(EX_RATE_MIN, imageEx_));

	//最上位桁(末尾要素)から一の位(先頭要素)へ向かって描画
	for (auto it = digitNumbers.rbegin(); it != digitNumbers.rend(); ++it) {
		int useNumber = *it;

		//数字の描画
		DrawRotaGraph(numberDrawPos_X, drawPos_Y, imageEx_, angle, numberImages_[useNumber], true);

		//位置差分(右方向へ)
		numberDrawPos_X += numberDrawOffset_X;
	}

	//「Combo」画像描画位置
	const int comboStartPos_X = numberDrawPos_X + 30;
	const int comboStartPos_Y = drawPos_Y + 10;
	const double comboExRate = 0.4;		//画像拡大率

	//拡大率が最低値(デフォルト)の時
	if (numberState_ == NUMBER_EX_STATE::NONE) {
		//「COMBO」の表示
		DrawRotaGraph(comboStartPos_X, comboStartPos_Y, comboExRate, angle, comboStringImage_, true);
	}
}

void ComboManager::ChangeState(const NUMBER_EX_STATE& _nextState)
{
	numberState_ = _nextState;

	if (numberState_ == NUMBER_EX_STATE::UP) {
		updateExRate_ = &ComboManager::UpdateExRateIncrease;
	}
	else if (numberState_ == NUMBER_EX_STATE::DOWN) {
		updateExRate_ = &ComboManager::UpdateExRateDecrease;
	}
}

void ComboManager::UpdateExRateIncrease(void)
{
	//増加
	imageEx_ += EX_RATE_ACC;

	//上限値にいった場合
	if (imageEx_ >= EX_RATE_MAX) {
		imageEx_ = EX_RATE_MAX;
		ChangeState(NUMBER_EX_STATE::DOWN);
	}
}

void ComboManager::UpdateExRateDecrease(void)
{
	//減少
	imageEx_ -= EX_RATE_ACC;

	//上限値にいった場合
	if (imageEx_ <= EX_RATE_MIN) {
		imageEx_ = EX_RATE_MIN;
		ChangeState(NUMBER_EX_STATE::NONE);
	}
}

void ComboManager::DrawDebug(void)const
{
#ifdef _DEBUG

	//コンボ数が0以上なら
	if (comboCount_ > 0)
	{
		//コンボ数を描画
		DrawString(10, 120, (L"Combo: " + std::to_wstring(comboCount_)).c_str(), 0xffffff);

	}
	//ヒット数の描画
	DrawString(10, 150, (L"Hit: " + std::to_wstring(hitCount_)).c_str(), 0xffffff);

#endif // _DEBUG
}

ComboManager::ComboManager(void)
	:comboCount_(0)
	,comboTimer_(0.0f)
	,hitCount_(0)
	,imageEx_(EX_RATE_MIN)
{
}

ComboManager::~ComboManager(void)
{
}
