/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#include "PhysicWorld.h"
#include "raknet/BitStream.h"
#include "SpeedController.h"

const int TIMESTEP = 1; // calculations per frame

const float TIMEOUT_MAX = 2.5;

// Blobby Settings
const float BLOBBY_HEIGHT = 89;
const float BLOBBY_WIDTH = 75;
const float BLOBBY_UPPER_SPHERE = 19;
const float BLOBBY_UPPER_RADIUS = 25;
const float BLOBBY_LOWER_SPHERE = 13;
const float BLOBBY_LOWER_RADIUS = 33;

// Volley Ball Net
const float NET_POSITION_X = 400;
const float NET_POSITION_Y = 438;
const float NET_RADIUS = 7;
const float NET_SPHERE = 154;

// Ball Settings
const float BALL_RADIUS = 31.5;

const float GROUND_PLANE_HEIGHT_MAX = 500;
const float GROUND_PLANE_HEIGHT = GROUND_PLANE_HEIGHT_MAX - BLOBBY_HEIGHT / 2.0;

// Boarder Settings
const float LEFT_PLANE = 0;
const float RIGHT_PLANE = 800.0;
// These numbers should include the blobbys width, but in the original game
// the blobbys can go a bit into the walls too.


// Gamefeeling relevant constants:
const float BLOBBY_ANIMATION_SPEED = 0.5;
const float BLOBBY_JUMP_ACCELERATION = 14.5;

// This is exactly the half of the gravitation, i checked it in
// the original code
const float BLOBBY_JUMP_BUFFER = 0.44;
const float GRAVITATION = 0.88;
const float BALL_GRAVITATION = 0.28;
const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

const float BALL_COLLISION_CORRECTION = 3.0;
const float BALL_COLLISION_VELOCITY = 13.125;

// Temp
float temp = 0;



PhysicWorld::PhysicWorld()
{
	reset(0);
	mCurrentBlobbyAnimationSpeed[LEFT_PLAYER] = 0.0;
	mCurrentBlobbyAnimationSpeed[RIGHT_PLAYER] = 0.0;
	mTimeSinceBallout = 0.0;
	mLastSpeed = 1.0;
	mAvgTimeDelta = 1.0;
}

PhysicWorld::~PhysicWorld()
{
}

bool PhysicWorld::resetAreaClear()
{
	if (blobbyHitGround(LEFT_PLAYER) && blobbyHitGround(RIGHT_PLAYER))
		return true;
	return false;
}

void PhysicWorld::reset(int player)
{
	if (player == 0)
		mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == 1)
		mBallPosition = Vector2(600, STANDARD_BALL_HEIGHT);
	else
		mBallPosition = Vector2(400, 450);

	mBallVelocity.clear();

	mBallRotation = 0.0;
	mBallAngularVelocity = STANDARD_BALL_ANGULAR_VELOCITY;
	mBlobState[LEFT_PLAYER] = 0.0;
	mBlobState[RIGHT_PLAYER] = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
}

void PhysicWorld::resetPlayer()
{
	mBlobPosition[LEFT_PLAYER] = Vector2( 200,
		GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
	mBlobPosition[RIGHT_PLAYER] = Vector2(600,
		GROUND_PLANE_HEIGHT + BLOBBY_HEIGHT / 2.0);
}

bool PhysicWorld::ballHitRightGround()
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mBallPosition.x > NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::ballHitLeftGround()
{
	if (mIsBallValid)
		if (mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mBallPosition.x < NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::blobbyHitGround(int player)
{
	if (player == 0)
	{
		if (getBlob(LEFT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else if (player == 1)
	{
		if (getBlob(RIGHT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else
		return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::roundFinished()
{
	if (resetAreaClear())
	{
		if (!mIsBallValid)
			if (mBallVelocity.y < 1.5 &&
				mBallVelocity.y > -1.5 && mBallPosition.y > 430)
				return true;
	}
	if (mTimeSinceBallout > TIMEOUT_MAX)
		return true;
	return false;
}

float PhysicWorld::lastHitIntensity()
{
	float intensity = mLastHitIntensity / 25.0;
	return intensity < 1.0 ? intensity : 1.0;
}

bool PhysicWorld::playerTopBallCollision(PlayerSide player)
{
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y - BLOBBY_UPPER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
		return true;
	return false;
}

inline bool PhysicWorld::playerBottomBallCollision(PlayerSide player)
{
	if (Vector2(mBallPosition,
		Vector2(mBlobPosition[player].x,
			mBlobPosition[player].y + BLOBBY_LOWER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
		return true;
	return false;
}

bool PhysicWorld::ballHitLeftPlayer()
{
	return mBallHitByLeftBlob;
}

bool PhysicWorld::ballHitRightPlayer()
{
	return mBallHitByRightBlob;
}

Vector2 PhysicWorld::getBall()
{
	return mBallPosition;
}

float PhysicWorld::getBallRotation()
{
	return mBallRotation;
}

float PhysicWorld::getBallSpeed()
{
	return mBallVelocity.length();
}

Vector2 PhysicWorld::getBlob(PlayerSide player)
{
	return mBlobPosition[player];
}

float PhysicWorld::getBlobState(PlayerSide player)
{
	return mBlobState[player];
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	mPlayerInput[LEFT_PLAYER] = input;
}

void PhysicWorld::setRightInput(const PlayerInput& input)
{
	mPlayerInput[RIGHT_PLAYER] = input;
}

// Blobby animation methods
void PhysicWorld::blobbyAnimationStep(PlayerSide player)
{
	mBlobState[player] += mCurrentBlobbyAnimationSpeed[player];
	if (mBlobState[player] >= 4.5)
	{
		mCurrentBlobbyAnimationSpeed[player]
			=- BLOBBY_ANIMATION_SPEED*mAvgTimeDelta*mLastSpeed;
	}
	if (mBlobState[player] < 0.0)
	{
		mCurrentBlobbyAnimationSpeed[player] = 0;
		mBlobState[player] = 0;
	}
}

void PhysicWorld::blobbyStartAnimation(PlayerSide player)
{
	if (mCurrentBlobbyAnimationSpeed[player] == 0)
		mCurrentBlobbyAnimationSpeed[player] =
			BLOBBY_ANIMATION_SPEED*mAvgTimeDelta*mLastSpeed;
}

void PhysicWorld::step(float timeDelta, float speed)
{
	float time = timeDelta*speed;
	mAvgTimeDelta = (3*mAvgTimeDelta+timeDelta)/4;
	mLastSpeed = speed;

    // Input Handling
	for (int i = LEFT_PLAYER; i <= RIGHT_PLAYER; ++i)
	{
		if (blobbyHitGround(PlayerSide(i)))
			if (mPlayerInput[i].up)
			{
				mBlobVelocity[i].y =
					-BLOBBY_JUMP_ACCELERATION;
				blobbyStartAnimation(PlayerSide(i));
		}
		if (mPlayerInput[i].up)
			mBlobVelocity[i].y -= BLOBBY_JUMP_BUFFER*time;

		mBlobVelocity[i].x = 0.0;

		if (mPlayerInput[i].left)
		{
			if(blobbyHitGround(PlayerSide(i)))
					blobbyStartAnimation(PlayerSide(i));
				mBlobVelocity[i].x -= BLOBBY_SPEED;
		}
		if (mPlayerInput[i].right)
		{
			if(blobbyHitGround(PlayerSide(i)))
					blobbyStartAnimation(PlayerSide(i));
				mBlobVelocity[i].x += BLOBBY_SPEED;
		}
	}

	Vector2 oldBlobPosition[2] = mBlobPosition;
	Vector2 oldBallPosition = mBallPosition;		//used for collision detections

	// Reset the ball-blobby collision
	mBallHitByRightBlob = false;
	mBallHitByLeftBlob = false;

	// Acceleration Integration
	mBlobPosition[LEFT_PLAYER].x += mBlobVelocity[LEFT_PLAYER].x*time;
	mBlobPosition[RIGHT_PLAYER].x += mBlobVelocity[RIGHT_PLAYER].x*time;

	mBlobPosition[LEFT_PLAYER].y += mBlobVelocity[LEFT_PLAYER].y*time+0.5*GRAVITATION*time*time;
	mBlobPosition[RIGHT_PLAYER].y += mBlobVelocity[RIGHT_PLAYER].y*time+0.5*GRAVITATION*time*time;

	mBlobVelocity[LEFT_PLAYER].y += GRAVITATION*time;
	mBlobVelocity[RIGHT_PLAYER].y += GRAVITATION*time;

	// Collision between blobby and the net
	if (mBlobPosition[LEFT_PLAYER].x+BLOBBY_LOWER_RADIUS>NET_POSITION_X-NET_RADIUS) // Collision with the net
		mBlobPosition[LEFT_PLAYER].x=NET_POSITION_X-NET_RADIUS-BLOBBY_LOWER_RADIUS;

	// Collision between blobby and the net
	if (mBlobPosition[RIGHT_PLAYER].x-BLOBBY_LOWER_RADIUS<NET_POSITION_X+NET_RADIUS) // Collision with the net
		mBlobPosition[RIGHT_PLAYER].x=NET_POSITION_X+NET_RADIUS+BLOBBY_LOWER_RADIUS;

	// Collision between blobby and the border
	if (mBlobPosition[LEFT_PLAYER].x < LEFT_PLANE)
		mBlobPosition[LEFT_PLAYER].x=LEFT_PLANE;

	// Collision between blobby and the border
	if (mBlobPosition[RIGHT_PLAYER].x > RIGHT_PLANE)
		mBlobPosition[RIGHT_PLAYER].x=RIGHT_PLANE;


	if (mBlobPosition[LEFT_PLAYER].y > GROUND_PLANE_HEIGHT)
	{
		if(mBlobVelocity[LEFT_PLAYER].y>0.9*time)
			blobbyStartAnimation(LEFT_PLAYER);
		mBlobPosition[LEFT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mBlobVelocity[LEFT_PLAYER].y = 0.0;
		// We need an error correction here because the y coordinate
		// is computed with a physical simulation of the gravitation.
	}
	if (mBlobPosition[RIGHT_PLAYER].y > GROUND_PLANE_HEIGHT)
	{
		if(mBlobVelocity[RIGHT_PLAYER].y>0.9*time)
			blobbyStartAnimation(RIGHT_PLAYER);
		mBlobPosition[RIGHT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mBlobVelocity[RIGHT_PLAYER].y = 0.0;
	}

	// Collision detection
	if(mIsBallValid)
	{
		if(playerBottomBallCollision(LEFT_PLAYER))
		{
			mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[LEFT_PLAYER]).length();
			mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[LEFT_PLAYER].x,mBlobPosition[LEFT_PLAYER].y+BLOBBY_LOWER_SPHERE));
			mBallVelocity = mBallVelocity.normalise();
			mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION)*time;
			mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
			mBallHitByLeftBlob=true;
		}
		else if(playerBottomBallCollision(RIGHT_PLAYER))
		{
			mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[RIGHT_PLAYER]).length();
			mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[RIGHT_PLAYER].x,mBlobPosition[RIGHT_PLAYER].y+BLOBBY_LOWER_SPHERE));
			mBallVelocity = mBallVelocity.normalise();
			mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION)*time;
			mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
			mBallHitByRightBlob=true;
		}
		else if(playerTopBallCollision(LEFT_PLAYER))
		{
			mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[RIGHT_PLAYER]).length();
			mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[LEFT_PLAYER].x,mBlobPosition[LEFT_PLAYER].y-BLOBBY_UPPER_SPHERE));
			mBallVelocity = mBallVelocity.normalise();
			mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION)*time;
			mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
			mBallHitByLeftBlob = true;
		}
		else if(playerTopBallCollision(RIGHT_PLAYER))
		{
			mLastHitIntensity = Vector2(mBallVelocity, mBlobVelocity[RIGHT_PLAYER]).length();
			mBallVelocity = -Vector2(mBallPosition,Vector2(mBlobPosition[RIGHT_PLAYER].x,mBlobPosition[RIGHT_PLAYER].y-BLOBBY_UPPER_SPHERE));
			mBallVelocity = mBallVelocity.normalise();
			mBallPosition += mBallVelocity.scale(BALL_COLLISION_CORRECTION)*time;
			mBallVelocity = mBallVelocity.scale(BALL_COLLISION_VELOCITY);
			mBallHitByRightBlob=true;
		}
	}
	// Ball to ground Collision
	else
	{
		if (mBallPosition.y + BALL_RADIUS > 500.0)
		{
			mBallVelocity = mBallVelocity.reflectY().scaleY(0.5);
			mBallVelocity = mBallVelocity.scaleX(0.55);
			mBallPosition.y = (500 - BALL_RADIUS);
		}
	}

	// Border Collision
	if (mBallPosition.x-BALL_RADIUS<=LEFT_PLANE && mBallVelocity.x < 0.0)
		mBallVelocity = mBallVelocity.reflectX();
	if (mBallPosition.x+BALL_RADIUS>=RIGHT_PLANE && mBallVelocity.x > 0.0)
		mBallVelocity = mBallVelocity.reflectX();


	// Net Collision
	if ( mBallPosition.y >= NET_POSITION_Y-NET_SPHERE
		&& abs(mBallPosition.x - NET_POSITION_X) <= NET_RADIUS + BALL_RADIUS
		&& abs(mBallPosition.x - NET_POSITION_X) <= NET_RADIUS + BALL_RADIUS)
	{
		mBallPosition -= mBallVelocity * time * 1.1;
		mBallVelocity = mBallVelocity.reflectX();
	}
	// Net Sphere
	else if (Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE)).length() <=
			NET_RADIUS + BALL_RADIUS && mBallPosition.y < NET_POSITION_Y-NET_SPHERE)
	{
		mBallPosition -= mBallVelocity * time * 1.1; // Protection of floating point errors
		mBallVelocity = mBallVelocity.reflect(
		Vector2(mBallPosition,Vector2(NET_POSITION_X,NET_POSITION_Y-NET_SPHERE))
		.normalise()).scale(0.75);
	}

	// Ball Gravitation
	if (mIsGameRunning)
	{
		mBallPosition.x += mBallVelocity.x*time;
		mBallPosition.y += mBallVelocity.y*time+0.5*BALL_GRAVITATION*time*time;
		mBallVelocity.y += BALL_GRAVITATION*time;
	}
	else if (ballHitLeftPlayer() || ballHitRightPlayer())
		mIsGameRunning = true;

	// Velocity Integration
	if (mBallVelocity.x > 0.0)
		mBallRotation += mBallAngularVelocity * (getBallSpeed()/6) * time;
	else if (mBallVelocity.x < 0.0)
		mBallRotation -= mBallAngularVelocity * (getBallSpeed()/6) * time;
	else
		mBallRotation -= mBallAngularVelocity * time;

	// Overflow-Protection
	if (mBallRotation<=0)
		mBallRotation = 6.25 + mBallRotation;
	else if (mBallRotation>=6.25)
		mBallRotation = mBallRotation - 6.25;

	// Blobbyanimationstep
	blobbyAnimationStep(LEFT_PLAYER);
	blobbyAnimationStep(RIGHT_PLAYER);

	mTimeSinceBallout = mIsBallValid ? 0.0 :
		mTimeSinceBallout + timeDelta / 75;
}

void PhysicWorld::dampBall()
{
	mBallVelocity = mBallVelocity.scale(0.6);
}

Vector2 PhysicWorld::getBallVelocity()
{
	return mBallVelocity;
}

bool PhysicWorld::getBlobJump(PlayerSide player)
{
	return !blobbyHitGround(player);
}

float PhysicWorld::estimateBallImpact()
{
	float steps;
	steps = (mBallVelocity.y - sqrt((mBallVelocity.y * mBallVelocity.y)-
	(-2 * BALL_GRAVITATION * (-mBallPosition.y + GROUND_PLANE_HEIGHT_MAX + BALL_RADIUS)))) / (-BALL_GRAVITATION);
	return (mBallVelocity.x * steps / (mAvgTimeDelta * mLastSpeed)) + mBallPosition.x;
}

Vector2 PhysicWorld::estimateBallPosition(int steps)
{
	Vector2 ret;
	float time = mAvgTimeDelta * mLastSpeed * float(steps);
	ret.x = mBallVelocity.x * time;
	ret.y = mBallVelocity.y * time + 0.5 * BALL_GRAVITATION * time * time;
	return mBallPosition + ret;
}

bool PhysicWorld::getBallActive()
{
	return mIsGameRunning;
}

void PhysicWorld::setState(RakNet::BitStream* stream)
{
	stream->Read(mBlobPosition[LEFT_PLAYER].x);
	stream->Read(mBlobPosition[LEFT_PLAYER].y);
	stream->Read(mBlobPosition[RIGHT_PLAYER].x);
	stream->Read(mBlobPosition[RIGHT_PLAYER].y);
	stream->Read(mBallPosition.x);
	stream->Read(mBallPosition.y);

	stream->Read(mBlobVelocity[LEFT_PLAYER].x);
	stream->Read(mBlobVelocity[LEFT_PLAYER].y);
	stream->Read(mBlobVelocity[RIGHT_PLAYER].x);
	stream->Read(mBlobVelocity[RIGHT_PLAYER].y);
	stream->Read(mBallVelocity.x);
	stream->Read(mBallVelocity.y);

	stream->Read(mPlayerInput[LEFT_PLAYER].left);
	stream->Read(mPlayerInput[LEFT_PLAYER].right);
	stream->Read(mPlayerInput[LEFT_PLAYER].up);
	stream->Read(mPlayerInput[RIGHT_PLAYER].left);
	stream->Read(mPlayerInput[RIGHT_PLAYER].right);
	stream->Read(mPlayerInput[RIGHT_PLAYER].up);
}

void PhysicWorld::getState(RakNet::BitStream* stream)
{
	stream->Write(mBlobPosition[LEFT_PLAYER].x);
	stream->Write(mBlobPosition[LEFT_PLAYER].y);
	stream->Write(mBlobPosition[RIGHT_PLAYER].x);
	stream->Write(mBlobPosition[RIGHT_PLAYER].y);
	stream->Write(mBallPosition.x);
	stream->Write(mBallPosition.y);

	stream->Write(mBlobVelocity[LEFT_PLAYER].x);
	stream->Write(mBlobVelocity[LEFT_PLAYER].y);
	stream->Write(mBlobVelocity[RIGHT_PLAYER].x);
	stream->Write(mBlobVelocity[RIGHT_PLAYER].y);
	stream->Write(mBallVelocity.x);
	stream->Write(mBallVelocity.y);

	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);

}

void PhysicWorld::getSwappedState(RakNet::BitStream* stream)
{
	stream->Write(800 - mBlobPosition[RIGHT_PLAYER].x);
	stream->Write(mBlobPosition[RIGHT_PLAYER].y);
	stream->Write(800 - mBlobPosition[LEFT_PLAYER].x);
	stream->Write(mBlobPosition[LEFT_PLAYER].y);
	stream->Write(800 - mBallPosition.x);
	stream->Write(mBallPosition.y);

	stream->Write(-mBlobVelocity[RIGHT_PLAYER].x);
	stream->Write(mBlobVelocity[RIGHT_PLAYER].y);
	stream->Write(-mBlobVelocity[LEFT_PLAYER].x);
	stream->Write(mBlobVelocity[LEFT_PLAYER].y);
	stream->Write(-mBallVelocity.x);
	stream->Write(mBallVelocity.y);

	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
}

PlayerInput* PhysicWorld::getPlayersInput()
{
	PlayerInput *input = new PlayerInput[2];
	input[LEFT_PLAYER] = mPlayerInput[LEFT_PLAYER];
	input[RIGHT_PLAYER] = mPlayerInput[RIGHT_PLAYER];
	return input;
}
