#pragma once
#include <DxLib.h>
#include <cmath>
#include <DirectXMath.h>
#include<ranges>
#include "../../Common/Quaternion.h"
#include"CameraCollider.h"

class Transform;

class Camera
{
public:
	//カメラの描画域(Near,Far)関連の定数------------------------------------------------------
	static constexpr float SPEED = 15.0f;			//カメラスピード

	static constexpr float CAMERA_NEAR = 40.0f;		//カメラクリップ：NEAR

	static constexpr float CAMERA_FAR = 19000.0f;	//カメラクリップ：FAR

	//カメラ座標関連の定数---------------------------------------------------------------------
	
	static constexpr VECTOR DEFAULT_CAMERA_POS = { 0.0f, 700.0f, -700.0f };			//カメラの初期座標

	static constexpr VECTOR RELATIVE_C2T_POS = { 0.0f, -300.0f, 500.0f };			//カメラ位置から注視点までの相対座標

	
	static constexpr VECTOR RELATIVE_F2C_POS_FOLLOW = { 0.0f, 300.0f, -500.0f };	//追従対象からカメラ位置までの相対座標(完全追従)

	//static constexpr VECTOR RELATIVE_F2C_POS_SPRING = { 0.0f, 40.0f, 150.0f };	//追従対象からカメラ位置までの相対座標(ばね付き)

	static constexpr float ROCK_DISTANCE_MIN = 500.0f;		//ロックオン時に最低限離れておく距離

	static constexpr float ROCK_MAGNIFICATION_Y = 0.25f;	//ロックオン時のY座標調整用の倍率
	
	//カメラ移動関連の定数---------------------------------------------------------------------
	
	static constexpr float MAX_MOVE_SPEED = 5.0f;		//移動速度の最大値
	static constexpr float MAX_ROT_SPEED_X = 0.025f;	//X軸回転速度の最大値
	static constexpr float MAX_ROT_SPEED_Y = 0.06f;	//Y軸回転速度の最大値

	//カメラ揺らし関連の定数--------------------------------------------------------------------

	static constexpr float TIME_SHAKE = 0.5f;		//時間

	static constexpr float WIDTH_SHAKE = 5.0f;		//幅

	static constexpr float SPEED_SHAKE = 40.0f;		//スピード

	//リセット関係
	static constexpr float RESET_TIME = 1.0f;
	static constexpr float RESET_STEP = 0.05f;

	//距離最低制限
	static constexpr float UNDER_LIMIT_Y = 280.0f;
	static constexpr float HIGHT_LIMIT_Y = 1200.0f;

	// カメラのX回転上限度角
	static constexpr float LIMIT_X_UP_RAD = 45.0f * (DX_PI_F / 180.0f);
	static constexpr float LIMIT_X_DW_RAD = -45.0f * (DX_PI_F / 180.0f);

	//線形補完
	static constexpr float NO_LERP = 1.0f;
	static constexpr float LERP_SPEED = 0.05f;

	//カメラモード
	enum class MODE
	{
		NONE,
		FIXED_POINT,	//定点カメラ
		FREE,			//フリーモード
		FOLLOW,			//追従モード
		SHAKE,			//カメラ揺らし
		LOCKON,			//ロックオン
		RESET,			//カメラ位置リセット用
		AUTO_MOVE,		//目標位置まで自動的に移動
	};

	struct ForFollowInfo
	{
		VECTOR pos;
		Quaternion quaRot;
	};

	//コンストラクタ
	Camera(void);

	//デストラクタ
	~Camera(void);

	//初期化処理
	void Init(void);

	//更新処理
	void Update(void);

	//カメラの描画モード関連------------------
	void SetBeforeDraw(void);

	void SetBeforeDrawFixedPoint(void);		//定点カメラ
	void SetBeforeDrawFree(void);			//フリーカメラ
	void SetBeforeDrawFollow(void);			//追従カメラ
	void SetBeforeDrawLockOn(void);			//ロックオンカメラ
	void SetBeforeDrawShake(void);			//カメラシェイク
	void SetBeforeDrawReset(void);			//カメラリセット
	void SetBeforeDrawAutoMove(void);		//カメラ自動移動

	//----------------------------------------
	// 描画処理
	void Draw(void);

	//解放処理
	void Release(void);

	//座標取得
	const VECTOR& GetPos(void) const;
	const VECTOR& GetLockPos(void)const;
	const VECTOR& GetGoalPos(void)const { return goalDirecPos_; }
	const VECTOR& GetC2FRelativeVec(void)const { return c2fRelative_; }
	const VECTOR& GetFocusPos(void)const { return focusPos_; }
	const VECTOR& GetFollowPos(void)const { return followObject_.pos; }
	const VECTOR& GetIdealPos(void)const { return idealPos_; }
	void SetAdjustedPos(const VECTOR& _pos) { adjustedPos_ = _pos; }

	//回転取得
	const Quaternion& GetRot(void)const;
	const VECTOR& GetAngle(void)const;
	//回転スピード
	const VECTOR& GetRotSpeed(void)const;
	void SetRotSpeed(const VECTOR& _speed);

	//視線取得
	const VECTOR GetCameraRay(void)const;
	const VECTOR GetCameraRayNormalised(void)const;

	//カメラモードの変更
	void ChangeMode(MODE mode);

	//追従対象の設定
	void SetFollow(const VECTOR _pos,const Quaternion _qua);

	//ロックオン最低限距離設定
	void SetLockOnDistanceMin(const float _distance) { lockOnDistanceMin_ = _distance; }

	//座標設定
	void SetPos(const VECTOR& pos,const VECTOR& focus);
	void SetPos(const VECTOR& pos);
	void SetFocusPos(const VECTOR& _focus);
	void SetGoalFocusPos(const VECTOR& _focus);
	void SetLockPos(const VECTOR& _lock);	//_isRote=ロックオン中回転を有効にするか(能力使用中は回転しないためそれ用)
	void SetGoalPos(const VECTOR& _goal);

	//カメラを初期位置に戻す
	void SetDefault(void);

	//再度カメラコライダーをセットする
	void ResetCollider(void);

	const MODE& GetMode(void)const;
	const bool IsFinishShake(void) { return finishShake_; }	//画面揺れ終了判別

	void DrawDebug(void);

	//前方の取得
	const VECTOR GetForward(void)const;

	//影用の設定
	void CameraSettingShadow(void)const;

private:
	//コライダー
	std::unique_ptr<CameraCollider> collider_;

	//追従対象
	ForFollowInfo followObject_;
	//開始・目標位置(リセット時などに使用)
	ForFollowInfo start_;
	ForFollowInfo goal_;
	float stepReset_;
	bool isReset_;

	//カメラモード
	MODE mode_;			
	MODE currentMode_;	//Shake・Resetに遷移する際に現在の物を保存する

	//カメラの位置
	VECTOR pos_;

	//ロックオン対象の位置
	VECTOR lockPos_;		//ロックオン対象の位置
	VECTOR prevGoalPos_;	//前回の目標位置
	VECTOR lockOnGoalPos_;	//目標位置(ロックオン)
	float lockOnDistanceMin_;	//ロックオン時の最低距離

	//カメラの注視点
	VECTOR focusPos_;
	VECTOR goalFocusPos_;
	
	//移動目標位置
	VECTOR goalDirecPos_;

	// カメラの理想位置（追従計算で確定）
	VECTOR idealPos_;
	// CameraColliderから受け取った補正後の目標位置
	VECTOR adjustedPos_;

	//カメラの上方向
	VECTOR cameraUp_;

	//カメラの回転
	Quaternion rot_;
	

	//回転スピード
	VECTOR rotSpeed_;

	// カメラ角度(rad)
	VECTOR angles_;
	// X軸回転が無い角度
	Quaternion rotOutX_;

	//画面揺らし用
	float stepShake_;

	//画面揺れが終わったか
	bool finishShake_;
	//演出前位置
	VECTOR defaultPos_;
	//揺れ方向
	VECTOR shakeDir_;
	//補完スピード
	float lerpStep_;

	//追従対象との距離
	VECTOR c2fRelative_;

	//回転
	void Rotation(void);
};

