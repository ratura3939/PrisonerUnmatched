#pragma once
#include<vector>
#include<functional>

class SingletonRegistry
{
public:

	//削除タイミング
	enum class DESTROY_TIMING
	{
		GAME_END,	//ゲーム終了時
		ALL_END,	//ループ終了時
	};

	//コピー禁止
	SingletonRegistry(const SingletonRegistry& _copy) = delete;
	SingletonRegistry& operator=(const SingletonRegistry& _copy) = delete;

	//静的インスタンスの取得
	static SingletonRegistry& GetInstance(void)
	{
		static SingletonRegistry instance;
		return instance;
	}

	/// @brief 破棄関数格納
	/// @param _func 破棄関数
	void RegistryDestroyer(const DESTROY_TIMING _timing, const std::function<void(void)>& _func)
	{
		destroyer_[_timing].push_back(_func);
	}

	//シングルトンの破棄
	void Delete(const DESTROY_TIMING _timing)
	{
		//破棄関数の取得
		auto it = destroyer_.find(_timing);

		//破棄関数がない場合は何もしない
		if (it == destroyer_.end())
			return;

		//破棄関数の実行
		for (auto func = it->second.rbegin();
			func != it->second.rend();
			++func)
		{
			//関数が空なら何もしない
			if (*func == nullptr)
				continue;

			//関数の実行
			(*func)();
		}

		//破棄関数の削除
		destroyer_.erase(it);
	}

private:

	//シングルトンの破棄関数格納
	std::unordered_map<DESTROY_TIMING, std::vector<std::function<void(void)>>> destroyer_;

	//コンストラクタ
	SingletonRegistry(void) = default;
	
	//デストラクタ
	~SingletonRegistry(void) = default;
};

