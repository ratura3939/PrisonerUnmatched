#include "../../../pch.h"
#include"../../../Manager/Generic/ResourceManager.h"
#include "AttackCommandInfo.h"

namespace {
	const VECTOR START_POSITION = { 800.0f,250.0f,0.0f };	//初期位置
	const VECTOR GOAL_POSITION = { 700.0f,250.0f,0.0f };	//目標位置

	const float MOVE_SPEED = 20.0f;	//スピード

	const double EX_RATE_ACC = 0.1;	//大きくなる速さ
	const double EX_RATE_MAX = 0.3;	//最大値

	const float COMMAND_EX_RATE = 0.8f;

	const float DIFF_GUIDE_FOR_CENTER = 130.0f* COMMAND_EX_RATE;	//ガイド項目の中心からの相対距離
	const VECTOR DIFF_GUIDE_BUTON[static_cast<int>(AttackCommandInfo::BUTTON_TYPE::MAX)] = {
		{DIFF_GUIDE_FOR_CENTER,0.0f,0.0f},	//B
		{0.0f,DIFF_GUIDE_FOR_CENTER,0.0f},	//A
		{-DIFF_GUIDE_FOR_CENTER,0.0f,0.0f},	//X
		{0.0f,-DIFF_GUIDE_FOR_CENTER,0.0f}	//Y
	};
}

AttackCommandInfo::AttackCommandInfo(void)
	:useUpdate_(&AttackCommandInfo::UpdateNone)
	,useType_(TYPE::MAX)
	,commandImages_{-1,-1}
	,commandGuides_{{-1,-1,-1,-1},{-1,-1,-1,-1}}
	,isAppeared_(false)
	,isDrawGuide_(false)
	,guideIconExRate_(0.0)
	,centerPos_(Utility::VECTOR_INIT)
	,goalPos_(Utility::VECTOR_INIT)
{
}

AttackCommandInfo::~AttackCommandInfo(void)
{
}

void AttackCommandInfo::Init(void)
{
	ResourceManager& resM = ResourceManager::GetInstance();
	using SOURCE = ResourceManager::SRC;

	//４ボタンの画像
	commandImages_[static_cast<int>(TYPE::SEPECIAL)] = resM.Load(SOURCE::OPERATION_ATK_IMG).handleId_;
	commandImages_[static_cast<int>(TYPE::ULTIMATE)] = resM.Load(SOURCE::OPERATION_ULT_IMG).handleId_;

	//効果のあるボタンにのみ画像を挿入
	commandGuides_[static_cast<int>(TYPE::SEPECIAL)][static_cast<int>(BUTTON_TYPE::X)] = resM.Load(SOURCE::TAKCLE_ATK_ICON_IMG).handleId_;
	commandGuides_[static_cast<int>(TYPE::SEPECIAL)][static_cast<int>(BUTTON_TYPE::Y)] = resM.Load(SOURCE::JUMP_ATK_ICON_IMG).handleId_;
	commandGuides_[static_cast<int>(TYPE::ULTIMATE)][static_cast<int>(BUTTON_TYPE::B)] = resM.Load(SOURCE::ULTIMATE_ICON_IMG).handleId_;
}

void AttackCommandInfo::Update(void)
{
	(this->*useUpdate_)();
}

void AttackCommandInfo::Draw(void)
{
	if (useType_ == TYPE::MAX || !isAppeared_) {
		return;
	}

	//４ボタン表示
	DrawCommand();

	if (isDrawGuide_) {
		//ガイド表示
		DrawGuide();
	}
}

void AttackCommandInfo::Appear(const TYPE& _type)
{
	//すでに出現している時
	if (isAppeared_) {
		//処理の必要なし
		return;
	}

	useType_ = _type;

	centerPos_ = START_POSITION;
	goalPos_ = GOAL_POSITION;

	useUpdate_ = &AttackCommandInfo::UpdateAppear;

	isAppeared_ = true;
}

void AttackCommandInfo::Disappear(const TYPE& _type)
{
	if (_type != useType_) {
		//処理の必要なし
		return;
	}

	//消去処理実行
	DoDisappear();
}

void AttackCommandInfo::DoDisappear(void)
{
	//そもそも出現していない時
	if (!isAppeared_) {
		//処理の必要なし
		return;
	}

	isDrawGuide_ = false;
	isAppeared_ = false;
	centerPos_ = GOAL_POSITION;
	goalPos_ = START_POSITION;

	useUpdate_ = &AttackCommandInfo::UpdateDisappear;
}

void AttackCommandInfo::UpdateNone(void)
{
}

void AttackCommandInfo::UpdateAppear(void)
{
	//出現（右から左へ）
	centerPos_.x -= MOVE_SPEED;

	//ゴールについたら
	if (centerPos_.x <= goalPos_.x) {
		centerPos_ = goalPos_;

		//ガイド見せるフェーズへ
		useUpdate_ = &AttackCommandInfo::UpdateShowGuide;
		guideIconExRate_ = 0.0;
		isDrawGuide_ = true;
	}
}

void AttackCommandInfo::UpdateShowGuide(void)
{
	//拡大率の加算
	if (guideIconExRate_ < EX_RATE_MAX) {
		guideIconExRate_ += EX_RATE_ACC;
	}
}

void AttackCommandInfo::UpdateDisappear(void)
{
	//消失（左から右へ）
	centerPos_.x += MOVE_SPEED;

	//ゴールについたら
	if (centerPos_.x >= goalPos_.x) {
		centerPos_ = goalPos_;
		//初期化
		useUpdate_ = &AttackCommandInfo::UpdateNone;
		useType_ = TYPE::MAX;
		isAppeared_ = false;
	}
}

void AttackCommandInfo::DrawCommand(void)
{
	const double angle = 0.0;
	DrawRotaGraph(static_cast<int>(centerPos_.x), static_cast<int>(centerPos_.y), COMMAND_EX_RATE, angle, commandImages_[static_cast<int>(useType_)], true);
}

void AttackCommandInfo::DrawGuide(void)
{
	const double angle = 0.0;

	for (int i = 0;i < static_cast<int>(BUTTON_TYPE::MAX);i++) {
		int useImage = commandGuides_[static_cast<int>(useType_)][i];

		//画像がある場合
		if (useImage != -1) {
			VECTOR drawPos = VAdd(centerPos_, DIFF_GUIDE_BUTON[i]);
			const int circleSize = 40;
			const int white = 0xffffff;
			//下地（円）の描画
			DrawCircle(static_cast<int>(drawPos.x), static_cast<int>(drawPos.y), circleSize, white);
			//UI画像の描画
			DrawRotaGraph(static_cast<int>(drawPos.x), static_cast<int>(drawPos.y), guideIconExRate_, angle, useImage, true);
		}
	}
}
