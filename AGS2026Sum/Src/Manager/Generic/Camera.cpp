#include"../../pch.h"
#include "../../Application.h"
#include "../../Utility/Utility.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Camera.h"

namespace {
	const float LERP_SPEED = 0.1f;			//補完速度
	const float LERP_MAX = 1.0f;			//補完完了
	const float HALF_DISTANCE = 0.5f;		//半分
	const float WALL_LERP_SPEED = 1.0f;		//壁には補完は必要ない
	const float ALLOW_NO_KERP_DIFF = 10.0f;	//補完が必要ないと感じる距離

	const float LERP_STEP_FOUCUS_RESET = 0.8f;	//カメラリセット時の補完スピード
	const float LERP_STEP_AUTOMOVE = 0.1f;		//カメラ自動移動時の補完スピード;
}

Camera::Camera(void)
	:mode_(MODE::NONE)
	,c2fRelative_(Utility::VECTOR_ZERO)
	,idealPos_(Utility::VECTOR_ZERO)
	,adjustedPos_(Utility::VECTOR_ZERO)
{
	currentMode_ = MODE::NONE;
	pos_ = Utility::VECTOR_ZERO;
	focusPos_ = Utility::VECTOR_ZERO;
	goalFocusPos_ = Utility::VECTOR_ZERO;
	lockPos_ = Utility::VECTOR_ZERO;
	rot_ = Quaternion::Identity();
	rotSpeed_ = {MAX_ROT_SPEED_X, MAX_ROT_SPEED_Y, 0.0f};

	stepReset_ = 0.0f;
	isReset_ = true;

	followObject_.pos = Utility::VECTOR_ZERO;
	followObject_.quaRot = Quaternion::Identity();
	start_.pos = Utility::VECTOR_ZERO;
	start_.quaRot = Quaternion::Identity();
	goal_.pos = Utility::VECTOR_ZERO;
	goal_.quaRot = Quaternion::Identity();

	angles_.x = Utility::Deg2RadF(0.0f);
	angles_.y = 0.0f;
	angles_.z = 0.0f;

	lerpStep_ = 0.0f;;
	finishShake_ = false;

	prevGoalPos_ = Utility::VECTOR_ZERO;
	lockOnGoalPos_ = Utility::VECTOR_ZERO;
	lockOnDistanceMin_ = 0.0f;
}

Camera::~Camera(void)
{
}

void Camera::Init(void)
{
	//カメラの初期設定
	SetDefault();
	collider_ = std::make_unique<CameraCollider>(*this);
	collider_->Init();
}

void Camera::Update(void)
{
	//collider_->Update();

	if (mode_ == MODE::FREE || mode_ == MODE::FOLLOW) {
		Rotation();
	}
}

void Camera::SetBeforeDraw(void)
{
	//クリップ距離を設定する(SetDrawScreenでリセットされる)
	SetCameraNearFar(CAMERA_NEAR, CAMERA_FAR);

	lerpStep_ += LERP_SPEED;
	if (lerpStep_ > LERP_MAX)lerpStep_ = LERP_MAX;

	switch (mode_)
	{
	case MODE::NONE:
		SetBeforeDrawFollow();
		break;

	case MODE::FIXED_POINT:
		SetBeforeDrawFixedPoint();
		break;
	case MODE::FREE:
		SetBeforeDrawFree();
		break;
	
	case MODE::FOLLOW:
		SetBeforeDrawFollow();
		break;

	case MODE::LOCKON:
		SetBeforeDrawLockOn();
		break;
	case MODE::SHAKE:
		SetBeforeDrawShake();
		break;

	case MODE::RESET:
		SetBeforeDrawReset();
		break;

	case MODE::AUTO_MOVE:
		SetBeforeDrawAutoMove();
		break;
	}

	// FOLLOW・LOCKON・NONE時にレイキャストによるカメラ位置補正を適用
	if (mode_ == MODE::FOLLOW || mode_ == MODE::LOCKON || mode_ == MODE::NONE) {
		collider_->UpdateRayCast();
		pos_ = Utility::Lerp(pos_, adjustedPos_, WALL_LERP_SPEED);
	}

	//カメラの設定(位置と注視点による制御)
	SetCameraPositionAndTargetAndUpVec(
		pos_, 
		focusPos_,
		cameraUp_
	);

	// DXライブラリのカメラとEffekseerのカメラを同期する。
	Effekseer_Sync3DSetting();

	c2fRelative_ = VSub(followObject_.pos, pos_);
}

void Camera::SetBeforeDrawFixedPoint(void)
{
	//何もしない
}

void Camera::SetBeforeDrawFree(void)
{
}

void Camera::SetBeforeDrawFollow(void)
{

	//追従対象の位置
	VECTOR followPos = followObject_.pos;

	//追従対象の向き
	Quaternion followRot = followObject_.quaRot;

	//auto& ins = InputManager::GetInstance();
	//if (ins.IsTrigerrDown("rock")) {
	//	ChangeMode(MODE::RESET);
	//	return;
	//}

	//追従対象までの距離ベクトルを回転させ相対座標を生成
	VECTOR relativeCPos = rot_.PosAxis(RELATIVE_F2C_POS_FOLLOW);

	//カメラ位置の更新(追従対象位置から相対座標を足す)
	VECTOR gPos = VAdd(followPos, relativeCPos);

	if (fabs(Utility::MagnitudeF(gPos) - Utility::MagnitudeF(pos_)) <= ALLOW_NO_KERP_DIFF) {
		lerpStep_ = NO_LERP;
	}
	idealPos_ = gPos;

	//注視点までの距離ベクトルを回転させ相対座標を生成
	VECTOR relativeTPos = rot_.PosAxis(RELATIVE_C2T_POS);

	//注視点の更新
	focusPos_ = VAdd(followPos, relativeTPos);

	//カメラの上方向
	cameraUp_ = rot_.GetUp();

}

void Camera::SetBeforeDrawLockOn(void)
{
	Rotation();

	//追従対象の位置
	VECTOR followPos = followObject_.pos;
	//追従対象の向き
	Quaternion followRot = followObject_.quaRot;

	//ロックオン対象と追従対象の離れている距離
	VECTOR distance = VSub(lockPos_, followPos);

	//離れる距離を数値化
	float disMag = Utility::MagnitudeF(distance);
	//最低限の値を下回っていたら
	if (disMag <= lockOnDistanceMin_) {
		//最低限の値を入れる
		disMag = lockOnDistanceMin_;
	}

	//カメラ位置調整(カメラは後方位置に。Y方向は距離に応じて高さを変える。)
	VECTOR relative = { 0.0f,disMag * ROCK_MAGNIFICATION_Y,-disMag };
	//カメラの回転情報をもとに相対座標を回転させる
	VECTOR relativeCPos = rot_.PosAxis(relative);

	//初動時のみに発動する
	//カメラの初期ゴールを計算結果で算出した場所にする
	if (!isReset_) {
		ChangeMode(MODE::RESET);
		goal_.pos = VAdd(followObject_.pos, followObject_.quaRot.PosAxis(relative));
		goal_.quaRot = followObject_.quaRot;
		return;
	}

	//注視点の更新
	//ロックオン中の注視点は追従対象とロックオン対象の中間地点にある。
	goalFocusPos_ = VAdd(followPos, VScale(distance, HALF_DISTANCE));
	focusPos_ = Utility::Lerp(focusPos_, goalFocusPos_, lerpStep_);

	//カメラ位置の更新
	prevGoalPos_ = lockOnGoalPos_;
	lockOnGoalPos_ = VAdd(focusPos_, relativeCPos);

	//pos_ = Utility::Lerp(pos_, lockOnGoalPos_, lerpStep_);
	idealPos_ = lockOnGoalPos_;

	//ある程度の高さは保つ
	if (pos_.y < UNDER_LIMIT_Y)pos_.y = UNDER_LIMIT_Y;
	if (pos_.y > HIGHT_LIMIT_Y)pos_.y = HIGHT_LIMIT_Y;

	//カメラの上方向
	cameraUp_ = rot_.GetUp();
}

void Camera::SetBeforeDrawShake(void)
{
	// 一定時間カメラを揺らす
	//stepShake_ -= SceneManager::GetInstance().GetDeltaTime();

	stepShake_ -= 0.01f;

	if (stepShake_ < 0.0f)
	{
		pos_ = defaultPos_;
		ChangeMode(MODE::FIXED_POINT);
		finishShake_ = true;
		return;
	}

	// -1.0f～1.0f
	float f = sinf(stepShake_ * SPEED_SHAKE);

	// -1000.0f～1000.0f
	f *= 1000.0f;

	// -1000 or 1000
	int d = static_cast<int>(f);

	// 0 or 1
	int shake = d % 2;

	// 0 or 2
	shake *= 2;

	// -1 or 1
	shake -= 1;

	// 移動量
	VECTOR velocity = VScale(shakeDir_, (float)(shake)*WIDTH_SHAKE);

	// 移動先座標
	 pos_ = VAdd(defaultPos_, velocity);
}

void Camera::SetBeforeDrawReset(void)
{
	stepReset_ += RESET_STEP;
	if (stepReset_ >= RESET_TIME) {
		lerpStep_ = NO_LERP;
		idealPos_ = pos_;
		adjustedPos_ = pos_;

		ChangeMode(currentMode_);
		isReset_ = true;
		angles_ = Utility::VECTOR_ZERO;

		VECTOR finishEuler = rot_.ToEuler();
		angles_.x = finishEuler.x;
		angles_.y = finishEuler.y;

		return;
	}

	// 球面補間
	rot_ = Quaternion::Slerp(start_.quaRot, goal_.quaRot, stepReset_);
	pos_ = VAdd(followObject_.pos, rot_.PosAxis(RELATIVE_F2C_POS_FOLLOW));

	// 注視点を追従対象に追従させる（毎フレーム更新）
	goalFocusPos_ = followObject_.pos;
	focusPos_ = Utility::Lerp(focusPos_, goalFocusPos_, LERP_STEP_FOUCUS_RESET);

	// angleの逆算
	VECTOR currentEuler = rot_.ToEuler();
	angles_.x = currentEuler.x;
	angles_.y = currentEuler.y;

	// カメラの上方向
	cameraUp_ = rot_.GetUp();
}

void Camera::SetBeforeDrawAutoMove(void)
{
	//目標位置まで移動する
	//終了の判定は呼び出した側で行う
	pos_ = Utility::Lerp(pos_, goalDirecPos_, LERP_STEP_AUTOMOVE);

	//カメラの上方向
	cameraUp_ = rot_.GetUp();
}

void Camera::Draw(void)
{
}

void Camera::Release(void)
{
}

const VECTOR& Camera::GetPos(void) const
{
	return pos_;
}

const Quaternion& Camera::GetRot(void) const
{
	return rot_;
}

const VECTOR& Camera::GetAngle(void) const
{
	return angles_;
}

const VECTOR& Camera::GetRotSpeed(void) const
{
	return rotSpeed_;
}

void Camera::SetRotSpeed(const VECTOR& _speed)
{
	rotSpeed_ = _speed;
}

const VECTOR& Camera::GetCameraRay(void) const
{
	return VSub(followObject_.pos, pos_);
}

const VECTOR& Camera::GetCameraRayNormalised(void) const
{
	return Utility::VNormalize(VSub(followObject_.pos, pos_));
}

void Camera::ChangeMode(MODE mode)
{

	//カメラの初期設定
	//カメラを揺らす前の位置で揺れるようにしたいため外している
	//SetDefault();
	
	if (mode == MODE::RESET)currentMode_ = mode_;

	//カメラモードの変更
  	mode_ = mode;

	isReset_ = false;
	lerpStep_ = 0.0f;

	//変更時の初期化処理
	switch (mode_)
	{
	case MODE::FIXED_POINT:
		break;

	case MODE::FREE:
		break;

	case MODE::FOLLOW:
		adjustedPos_ = pos_;
		break;

	case MODE::SHAKE:
		finishShake_ = false;
		stepShake_ = TIME_SHAKE;
		shakeDir_ = VNorm({ 0.7f, 0.7f ,0.0f });
		defaultPos_ = pos_;
		break;

	case MODE::RESET:
	{
		stepReset_ = 0.0f;
		start_.pos = pos_;
		start_.quaRot = rot_;
		goalFocusPos_ = followObject_.pos;

		//現在のキャラクターの向きから Y軸回転（ヨー）のみを抽出
		float charaYaw;
		VECTOR axisY = { 0.0f, 1.0f, 0.0f };
		followObject_.quaRot.ToAngleAxis(&charaYaw, &axisY);

		//現在のカメラの高さ(angles_.x)と「キャラの向き(charaYaw)」を合成
		Quaternion goalRotY = Quaternion::AngleAxis(charaYaw, Utility::AXIS_Y);
		Quaternion goalRotX = Quaternion::AngleAxis(angles_.x, Utility::AXIS_X);

		//水平回転を先に適応し、その後に現在の高さを適応
		goal_.quaRot = goalRotY.Mult(goalRotX);

		VECTOR goalRelative = RELATIVE_F2C_POS_FOLLOW;
		goalRelative.y = pos_.y;
		goal_.pos = VAdd(followObject_.pos, goal_.quaRot.PosAxis(goalRelative));

		//回転の同期
		VECTOR currentEuler = rot_.ToEuler();
		angles_.x = currentEuler.x;
		angles_.y = currentEuler.y;
	}
		break;

	case MODE::AUTO_MOVE:
		angles_ = Utility::VECTOR_ZERO;
		rot_ = Quaternion::Identity();
		break;

	case MODE::LOCKON:
		break;
	}

}

void Camera::SetFollow(const VECTOR _pos, const Quaternion _qua)
{
	followObject_.pos = _pos;
	followObject_.quaRot = _qua;
}

void Camera::SetPos(const VECTOR& pos, const VECTOR& target)
{
	pos_ = pos;
	focusPos_ = target;
}

void Camera::SetPos(const VECTOR& pos)
{
	pos_ = pos;
}

void Camera::SetFocusPos(const VECTOR& _focus)
{
	focusPos_ = _focus;
}

void Camera::SetGoalFocusPos(const VECTOR& _focus)
{
	goalFocusPos_ = _focus;
}

void Camera::SetLockPos(const VECTOR& _lock)
{
	lockPos_ = _lock;
}

void Camera::SetGoalPos(const VECTOR& _goal)
{
	goalDirecPos_ = _goal;
}

const VECTOR& Camera::GetLockPos(void) const
{
	return lockPos_;
}

void Camera::ResetCollider(void)
{
	collider_->SetCollider();
}

const Camera::MODE& Camera::GetMode(void) const
{
	return mode_;
}

void Camera::DrawDebug(void)
{
	//DrawFormatString(0, 0, 0xffffff, "cPOS={%.1f,%.1f,%.1f}\ncROT={%.1f,%.1f,%.1f}", pos_.x, pos_.y, pos_.z, rot_.x, rot_.y, rot_.z);
	//DrawFormatString(0, 100, 0xffffff, "FCPOS={%.1f,%.1f,%.1f}", focusPos_.x, focusPos_.y, focusPos_.z);
	DrawSphere3D(focusPos_, 8, 10, 0x00ff00, 0x00ff00, false);
}

const VECTOR Camera::GetForward(void)
{
	return VNorm(VSub(focusPos_, pos_));
}

void Camera::CameraSettingShadow(void)
{
	constexpr float SIZE = 13050.0f;
	constexpr float S_NEAR = 10.0f;
	constexpr float S_FAR = 10050.0f;
	constexpr float TARGET_LIMIT_Y = 0.0f;

	//カメラタイプを正射影に
	SetupCamera_Ortho(SIZE);

	//描画する奥行き範囲を設定
	SetCameraNearFar(S_NEAR, S_FAR);

	//注視点の制限
	VECTOR targetPos = focusPos_;
	if (targetPos.y > TARGET_LIMIT_Y) targetPos.y = TARGET_LIMIT_Y;

	//static VECTOR targetPos = Utility::VECTOR_ZERO;

	//InputManager& ins = InputManager::GetInstance();
	//using COMMAND = InputManager::INPUT_COMMAND;
	//if (ins.IsPressed(COMMAND::DEBUG_RIGHT))
	//{
	//	targetPos.x -= 1000.0f;
	//}
	//if (ins.IsPressed(COMMAND::DEBUG_LEFT))
	//{
	//	targetPos.x += 1000.0f;
	//}
	//if (ins.IsPressed(COMMAND::DEBUG_UP))
	//{
	//	targetPos.y -= 1000.0f;
	//}
	//if (ins.IsPressed(COMMAND::DEBUG_DOWN))
	//{
	//	targetPos.y += 1000.0f;
	//}
	//if (ins.IsPressed(COMMAND::DEBUG_FLONT))
	//{
	//	targetPos.z += 1000.0f;
	//}
	//if (ins.IsPressed(COMMAND::DEBUG_BACK))
	//{
	//	targetPos.z -= 1000.0f;
	//}

	//カメラの視点、注視点の設定
	SetCameraPositionAndTarget_UpVecY(pos_, targetPos);
}

void Camera::SetDefault(void)
{
	//カメラの初期設定
	pos_ = DEFAULT_CAMERA_POS;

	//注視点
	focusPos_ = VAdd(pos_, RELATIVE_C2T_POS);

	//カメラの上方向
	cameraUp_ = { 0.0f, 1.0f, 0.0f };

	//カメラはX軸に傾いているが、
	//この傾いた状態を角度ゼロ、傾き無しとする
	rot_ = Quaternion::Identity();

	angles_.x = Utility::Deg2RadF(0.0f);
	angles_.y = 0.0f;
	angles_.z = 0.0f;
}

void Camera::Rotation(void)
{
	InputManager& ins = InputManager::GetInstance();

	using COMMAND = InputManager::INPUT_COMMAND;

	if (ins.IsPressed(COMMAND::UP_SUB))
	{
		angles_.x -= rotSpeed_.x;
		if (angles_.x <= LIMIT_X_DW_RAD)
			angles_.x = LIMIT_X_DW_RAD;
	}
	if (ins.IsPressed(COMMAND::DOWN_SUB))
	{
		angles_.x += rotSpeed_.x;
		if (angles_.x >= LIMIT_X_UP_RAD)
			angles_.x = LIMIT_X_UP_RAD;
	}
	if (ins.IsPressed(COMMAND::LEFT_SUB))
	{
		angles_.y -= rotSpeed_.y;
	}
	if (ins.IsPressed(COMMAND::RIGHT_SUB))
	{
		angles_.y += rotSpeed_.y;
	}

	//カメラ座標を中心として、注視点を回転させる
	if (!Utility::EqualsVZero(angles_))
	{
		// 正面から設定されたY軸分、回転させる
		rotOutX_ = Quaternion::AngleAxis(angles_.y, Utility::AXIS_Y);

		// 正面から設定されたX軸分、回転させる
		rot_ = rotOutX_.Mult(Quaternion::AngleAxis(angles_.x, Utility::AXIS_X));
		// カメラの上方向
		cameraUp_ = rot_.GetUp();
	}
}

