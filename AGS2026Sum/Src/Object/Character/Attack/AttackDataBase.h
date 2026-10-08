#pragma once
#include<string>
#include<memory>

class Collider;

struct AttackDataBase
{
	//攻撃属性
	enum class ATTACK_ELEMENT
	{
		NORMAL		//属性なし
		, TACKLE	//突進
		, JUMP		//ジャンプ
		, MAX
	};

	//威力
	float power = 0.0f;

	//連続ヒットするか
	bool isMultiHit = false;

	//連続ヒット間隔
	float hitInterval = -1.0f;

	//攻撃属性
	ATTACK_ELEMENT element = ATTACK_ELEMENT::NORMAL;

	//継承のキャスト用仮想関数
	virtual ~AttackDataBase(void) = default;

	//文字列をキャスト
	const ATTACK_ELEMENT CastStringToElement(const std::string& _elementStr)
	{
		if (_elementStr == "Normal") return ATTACK_ELEMENT::NORMAL;
		else if (_elementStr == "Tackle") return ATTACK_ELEMENT::TACKLE;
		else if (_elementStr == "Jump") return ATTACK_ELEMENT::JUMP;
		else return ATTACK_ELEMENT::NORMAL;
	}
};