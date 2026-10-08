#include "../../../../pch.h"
#include "../../../../Application.h"
#include "../../../../Manager/Generic/ResourceManager.h"
#include "../../../../Manager/Decoration/UIManager2d.h"
#include "../../../../Renderer/PixelMaterial.h"
#include "../../../../Renderer/PixelRenderer.h"
#include "../Skill/EnemySkillBase.h"
#include "../Types/Boss/BossBase.h"
#include "BossUI.h"

BossUI::BossUI(void)
	:boss_(nullptr)
	,isDraw_(false)
{
}

BossUI::~BossUI(void)
{
}

void BossUI::Load(void)
{
	//HP
	auto& app = Application::GetInstance();
	float screenWidth = static_cast<float>(app.GetWindowWidth());
	float screenHeight = static_cast<float>(app.GetWindowHeight());
	widthRate_ = screenWidth / static_cast<float>(BASE_WIDTH);
	heightRate_ = screenHeight / static_cast<float>(BASE_HEIGHT);

	// 中心座標
	Vector2 healthCenter;
	healthCenter.x = static_cast<int>(screenWidth * HP_INFO.anchorX + HP_INFO.offsetX * widthRate_);
	healthCenter.y = static_cast<int>(screenHeight * HP_INFO.anchorY + HP_INFO.offsetY * heightRate_);

	// サイズ
	float hpWidth = HP_WIDTH * widthRate_;
	float hpHeight = HP_HEIGHT * heightRate_;

	//半分
	const float halfHpWidth = hpWidth * 0.5f;
	const float halfHpHeight = hpHeight * 0.5f;

	// 左上・右下
	healthPos_.x = healthCenter.x - static_cast<int>(halfHpWidth);
	healthPos_.y = healthCenter.y - static_cast<int>(halfHpHeight);

	healthWH_.x = healthPos_.x + static_cast<int>(hpWidth);
	healthWH_.y = healthPos_.y + static_cast<int>(hpHeight);

	//アイコン
	auto& res = ResourceManager::GetInstance();
	auto& ui = UIManager2d::GetInstance();
	atkIcon_[static_cast<int>(AttackDataBase::ATTACK_ELEMENT::NORMAL)] = -1;
	atkIcon_[static_cast<int>(AttackDataBase::ATTACK_ELEMENT::JUMP)] = res.Load(ResourceManager::SRC::JUMP_ATK_ICON_IMG).handleId_;
	atkIcon_[static_cast<int>(AttackDataBase::ATTACK_ELEMENT::TACKLE)] = res.Load(ResourceManager::SRC::TAKCLE_ATK_ICON_IMG).handleId_;

	//設定のみ(画像を差し替えて使う)
	ui.Add(UIManager2d::UI_NAME::ATTACK_ICON, -1, UIManager2d::UI_DIRECTION_2D::ZOOM_INOUT, UIManager2d::UI_DRAW_DIMENSION::DIMENSION_2);
	VECTOR iconPos = Utility::VECTOR_INIT;
	iconPos.x = healthPos_.x + (halfHpWidth) + (ICON_LOCAL_POS_X * widthRate_);
	iconPos.y = healthPos_.y + (halfHpHeight) + (ICON_LOCAL_POS_Y * heightRate_);
	iconPos.z = 0.0f;
	ui.SetUIDirectionPram(UIManager2d::UI_NAME::ATTACK_ICON, UIManager2d::UI_DIRECTION_GROUP::ZOOM, ICON_ZOOM_SPEED, ICON_ZOOM_MAX, ICON_ZOOM_MIN);
	ui.SetPos(UIManager2d::UI_NAME::ATTACK_ICON, iconPos);
	ui.SetScale(UIManager2d::UI_NAME::ATTACK_ICON, ICON_SIZE);

	//シェーダーの設定
	material_ = std::make_unique<PixelMaterial>(L"BossGuardGauge.cso", CONST_BUFF_NUM);
	material_->AddTextureBuf(ResourceManager::GetInstance().Load(ResourceManager::SRC::CRACK).handleId_);
	material_->AddConstBuf({ 0.0f,0.0f,0.0f,0.0f });
	material_->AddConstBuf({ 0.0f,0.0f,0.0f,0.0f });

	//レンダラー
	Vector2 gaugePos;
	gaugePos.x = static_cast<int>(screenWidth * GAUGE_INFO.anchorX + GAUGE_INFO.offsetX * widthRate_ - GAUGE_SIZE * GAUGE_UV_U * widthRate_);
	gaugePos.y = static_cast<int>(screenHeight * GAUGE_INFO.anchorY + GAUGE_INFO.offsetY * heightRate_ - GAUGE_SIZE * GAUGE_UV_V * heightRate_);

	//レンダラーの生成
	renderer_ = std::make_unique<PixelRenderer>();
	renderer_->SetPos(gaugePos);
	Vector2 rendererSize;
	rendererSize.x = static_cast<int>(GAUGE_SIZE * widthRate_);
	rendererSize.y = static_cast<int>(GAUGE_SIZE * heightRate_);
	renderer_->SetSize(rendererSize);
	renderer_->MakeSquereVertex();
}

void BossUI::Draw(void)
{
	//表示するものがいないなら
	if (!boss_ || !isDraw_)return;

	//ボスUI
	float hp = boss_->GetHp();
	float hpMax = boss_->GetHpMax();

	// 枠
	DrawBox(
		healthPos_.x - static_cast<int>(HP_WINDOW * widthRate_),
		healthPos_.y - static_cast<int>(HP_WINDOW * heightRate_),
		healthWH_.x + static_cast<int>(HP_WINDOW * widthRate_),
		healthWH_.y + static_cast<int>(HP_WINDOW * heightRate_),
		0x000000,
		true);

	// HP
	if (hp > 0.0f)
	{
		DrawBox(
			healthPos_.x,
			healthPos_.y,
			healthPos_.x + static_cast<int>((HP_WIDTH * widthRate_) * (hp / hpMax)),
			healthPos_.y + static_cast<int>(HP_HEIGHT * heightRate_),
			0xff8888,
			true);
	}

	//攻撃アイコン
	auto& ui = UIManager2d::GetInstance();
	const auto& skill = boss_->GetCurrentSkill();
	if (skill == nullptr)
	{

	}
	else if (skill->GetAttackElement() != AttackDataBase::ATTACK_ELEMENT::NORMAL && !skill->IsEnd())
	{
		//UI見やすさ用の背景（■）
		const int circleSize = 35;
		const int white = 0xffffff;
		const int bocWidthHalf = 60;
		const int bocHeightHalf = 25;

		VECTOR iconDrawPos = ui.GetDrawPos(UIManager2d::UI_NAME::ATTACK_ICON);

		DrawBox(static_cast<int>(iconDrawPos.x) - bocWidthHalf, static_cast<int>(iconDrawPos.y) - bocHeightHalf,
			static_cast<int>(iconDrawPos.x) + bocWidthHalf, static_cast<int>(iconDrawPos.y) + bocHeightHalf,
			white, true);

		ui.SetImage(UIManager2d::UI_NAME::ATTACK_ICON, atkIcon_[static_cast<int>(skill->GetAttackElement())]);
		ui.Update(UIManager2d::UI_NAME::ATTACK_ICON);
		ui.Draw(UIManager2d::UI_NAME::ATTACK_ICON);
	}

	//ガード耐久値
	float guard = boss_->GetGuardDurability();
	float GuardMax = BossBattleComponent::GUARD_DURABILITY_MAX;
	float progress = guard / GuardMax;
	progress = std::max(progress, 0.0f);

	//定数バッファの更新と描画
	material_->SetConstBuf(0, { GAUGE_UV_U,GAUGE_UV_V,progress,0.0f });
	material_->SetConstBuf(1, { GAUGE_RADIUS,0.0f,0.0f,0.0f });
	renderer_->Draw(*material_);
}

void BossUI::SetDrawBoss(const EnemyBase* _boss)
{
	//いないならボス表示を無効化
	if (!_boss || !_boss->IsBoss())
	{
		isDraw_ = false;
		boss_ = nullptr;
		return;
	}

	//既に同じなら表示判定のみ変更
	if (boss_ == _boss)
	{
		isDraw_ = true;
		return;
	}

	//表示ボスを変える
	boss_ = dynamic_cast<const BossBase*>(_boss);
	isDraw_ = true;
}