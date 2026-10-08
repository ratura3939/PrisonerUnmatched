#include "../../../../pch.h"
#include "../Skill/EnemySkillBase.h"
#include "../EnemyBase.h"
#include "../EnemyGroup.h"
#include "MiddleBossBrain.h"

MiddleBossBrain::MiddleBossBrain(EnemyBase& _parent)
	: BrainBase(_parent)
{
	//グループの命令ごとの行動優先度の設定
	orderPriority_[static_cast<int>(GROUP_ORDER::STAY)] = &MiddleBossBrain::OrderStayPriority;
	orderPriority_[static_cast<int>(GROUP_ORDER::MOVE)] = &MiddleBossBrain::OrderMovePriority;
	orderPriority_[static_cast<int>(GROUP_ORDER::ALERT)] = &MiddleBossBrain::OrderAlertPriority;
}

MiddleBossBrain::~MiddleBossBrain(void)
{
}

void MiddleBossBrain::DecidePriority(void)
{
	//グループに所属していない　または　グループが死んでいるなら何もしない
	if (!parent_.IsInGroup() || !parent_.IsAlive())return;

	//行動優先度の初期化
	actionPriority_.fill(0);

	//グループの命令
	const EnemyGroup* group = parent_.GetGroup();
	int order = static_cast<int>(group->GetOrder());

	//グループから一定距離以上離れているならグループに戻る行動を優先する
	if (Utility::SqrMagnitude(parent_.GetPos(), group->GetGroupPos()) > EnemyBase::LEAVE_GROUP_DIST * EnemyBase::LEAVE_GROUP_DIST)
	{
		//グループに戻る行動を優先する
		actionPriority_[static_cast<int>(ENEMY_ACTION::RETURN_GROUP)] = PRIORITY;
		return;
	}

	//グループの命令ごとの行動優先度の設定
	(this->*orderPriority_[order])();
}

void MiddleBossBrain::ChoiceAttackSkill(void)
{
	//所持スキル
	auto& skills = parent_.GetSkills();

	//スキルがないなら何もしない
	if (skills.empty())
	{
		parent_.ChangeAction(ENEMY_ACTION::STAY);
		return;
	}

	//候補からランダム
	int rand = Utility::GetRandomValue(0, static_cast<int>(skills.size()) - 1);

	//選ばれたスキル
	EnemySkillBase* choiceSkill = skills[rand].get();

	//スキル設定
	parent_.SetCurrentSkill(choiceSkill);
}

void MiddleBossBrain::OrderStayPriority(void)
{
	//グループに所属していない　または　グループが死んでいるなら何もしない
	if (!parent_.IsInGroup() || !parent_.IsAlive())return;

	//グループの命令が待機なら待機行動を優先する
	actionPriority_[static_cast<int>(ENEMY_ACTION::STAY)] = PRIORITY;
}

void MiddleBossBrain::OrderMovePriority(void)
{
	//グループに所属していない　または　グループが死んでいるなら何もしない
	if (!parent_.IsInGroup() || !parent_.IsAlive())return;

	//グループの命令が移動なら移動行動を優先する
	actionPriority_[static_cast<int>(ENEMY_ACTION::MOVE)] = PRIORITY;
}

void MiddleBossBrain::OrderAlertPriority(void)
{
	//グループに所属していない　または　グループが死んでいるなら何もしない
	if (!parent_.IsInGroup() || !parent_.IsAlive())return;

	//目標地点
	const VECTOR& goalPos = parent_.GetGroup()->GetGoalPos();

	//プレイヤー(目標地点)との距離に応じて攻撃か警戒行動を優先する
	if (Utility::SqrMagnitude(parent_.GetPos(), goalPos) < EnemyBase::ATTACK_RADIUS * EnemyBase::ATTACK_RADIUS)
	{
		//攻撃行動を優先する
		actionPriority_[static_cast<int>(ENEMY_ACTION::ATTACK_READY)] = PRIORITY;
	}
	else
	{
		//警戒行動を優先する
		actionPriority_[static_cast<int>(ENEMY_ACTION::ALERT)] = PRIORITY;
	}
}
