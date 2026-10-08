#pragma once
#include<unordered_map>
#include<string>
#include<nlohmann/json.hpp>
#include"../../Attack/AttackDataBase.h"

struct EnemySkillDatas
{
	//stringをElementに変換
	static AttackDataBase::ATTACK_ELEMENT ToElementType(const std::string& _type)
	{
		if ("NormalSkill" == _type) return AttackDataBase::ATTACK_ELEMENT::NORMAL;
		if ("Tackle" == _type) return AttackDataBase::ATTACK_ELEMENT::TACKLE;
		if ("Jump" == _type) return AttackDataBase::ATTACK_ELEMENT::JUMP;
		return AttackDataBase::ATTACK_ELEMENT::NORMAL;
	}

	//スキル達のデータ
	std::unordered_map<std::string, std::shared_ptr<AttackDataBase>> skillsData = {};
};

inline void from_json(const nlohmann::json& _data, EnemySkillDatas& _skills)
{
	for (const auto& [name, data] : _data.items())
	{
		//スキルの攻撃データ
		std::shared_ptr<AttackDataBase> atkData = std::make_shared<AttackDataBase>();

		//威力
		data.at("power").get_to(atkData->power);

		//属性
		std::string element;
		data.at("element").get_to(element);
		atkData->element = EnemySkillDatas::ToElementType(element);

		//格納
		_skills.skillsData.emplace(name, std::move(atkData));
	}
}