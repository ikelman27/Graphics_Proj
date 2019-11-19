#include "SlenderMan.h"

static AgroLevels levels;

SlenderMan::SlenderMan(Mesh* m, Material* mat, float rad, Camera* player)
	:Entity(
		m,
		mat,
		rad
	)
{
	this->player = player;
	// Don't think a reference to Game is needed
	agroLevel = 3;
	staticAlpha = 0;
	distance = 0;
	proximityCheck = false;
	lastPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	standingTimer = 0;

	levels.maxRanges.push_back(40.0f);
	levels.minRanges.push_back(50.0f);
	levels.teleportTimers.push_back(7.0f);

	levels.maxRanges.push_back(30.0f);
	levels.minRanges.push_back(40.0f);
	levels.teleportTimers.push_back(5.0f);

	levels.maxRanges.push_back(20.0f);
	levels.minRanges.push_back(30.0f);
	levels.teleportTimers.push_back(3.0f);

	levels.maxRanges.push_back(10.0f);
	levels.minRanges.push_back(20.0f);
	levels.teleportTimers.push_back(1.5f);

	// Should be done with a parameter, but we can change this later
	// Just set it to the worlds bounds
	boundsMax = XMFLOAT2(50.0f, 50.0f);
	boundsMin = XMFLOAT2(-50.0f, -50.0f);

	isVisible = false;

	timer = 0.0f;
	stopTeleport = false;
}

SlenderMan::~SlenderMan()
{
}

void SlenderMan::Update(float deltaTime)
{
	timer += deltaTime;
	GetDistance();
	CheckForProximity(deltaTime);
	CheckForStatic();
	CheckStandingStill(deltaTime);

	// Increase static alpha here

	if (timer >= levels.teleportTimers[agroLevel] && !proximityCheck && !stopTeleport) {
		if (CheckLineOfSight()) {
			Teleport();
			timer = 0;
		}
	}

	/*if (*staticAlpha >= 1.0f) {
		// Trigger game over
		// Might just want to check this in Game.cpp
		// or see if there are events in C++ like C#
	}*/
}

void SlenderMan::Teleport()
{
	// Get forward vector then make inverse
	XMFLOAT3 behindTemp = player->GetDirection();
	behindTemp.x *= -1;
	//behindTemp.y *= -1;
	behindTemp.z *= -1;

	XMVECTOR behind = XMLoadFloat3(&behindTemp);

	// Finds angle behind player
	float angle = rand() % 180 - 90;

	// Get the angle quaternion
	XMFLOAT4 angleRotTemp;
	XMStoreFloat4(&angleRotTemp, XMQuaternionIdentity());
	XMFLOAT3 axisTemp = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMVECTOR axis = XMLoadFloat3(&axisTemp);
	XMVECTOR angleRot;
	XMStoreFloat4(&angleRotTemp, XMQuaternionMultiply(XMLoadFloat4(&angleRotTemp), XMQuaternionRotationAxis(axis, angle)));
	angleRot = XMLoadFloat4(&angleRotTemp);

	// Get point on unit circle
	XMVECTOR target = XMVector3Rotate(behind, angleRot);

	float val = rand() % (int)levels.maxRanges[agroLevel] + (int)levels.minRanges[agroLevel];
	target = XMVector3Normalize(target);
	target *= val;

	XMVECTOR newPosTemp =  XMLoadFloat3(&player->GetPosition()) + target;

	float distanceFromEdge = 10;
	XMFLOAT3 maxPosTemp = XMFLOAT3(boundsMax.x, 0.0f, boundsMax.y);
	XMFLOAT3 minPosTemp = XMFLOAT3(boundsMin.x, 0.0f, boundsMin.y);

	XMVECTOR maxPos = XMLoadFloat3(&maxPosTemp);
	XMVECTOR minPos = XMLoadFloat3(&minPosTemp);

	// Need to clamp magnitude
	XMFLOAT3 newPos;
	XMStoreFloat3(&newPos, newPosTemp);
	newPos.x = std::clamp(newPos.x, boundsMin.x + distanceFromEdge, boundsMax.x - distanceFromEdge);
	newPos.z = std::clamp(newPos.z, boundsMin.y + distanceFromEdge, boundsMax.y - distanceFromEdge);
	SetTranslation(newPos);
	std::printf("Teleport");
}

bool SlenderMan::CheckLineOfSight()
{
	if (isVisible) {
		/*
		Raycast stuff goes here
		*/
		
	}
	return true;
}

void SlenderMan::CheckForStatic()
{
	if (CheckLineOfSight()) {
		// Change alpha's static
		//staticAlpha = 
		//return
	}

	XMVECTOR temp = XMVector3Dot(XMVector3Normalize(XMLoadFloat3(&player->GetDirection())), XMVector3Normalize(XMLoadFloat3(GetPosition()) - XMLoadFloat3(&player->GetPosition())));
	XMFLOAT3 temp2;
	XMStoreFloat3(&temp2, temp);
	float dotVal = temp2.x;

	float val = (20.0f / distance) / 200.0f * dotVal;

	// Clamp the magnitude between 0 and 1
	//staticAlpha = 
}

void SlenderMan::CheckForProximity(float deltaTime)
{
	if (distance < 8.0f) {
		proximityCheck = true;

		float val = (1.0f / distance);
		val *= deltaTime;
	}
	else {
		proximityCheck = false;
	}
}

void SlenderMan::CheckStandingStill(float deltaTime)
{
	XMVECTOR lastPosTemp = XMLoadFloat3(&lastPos);
	XMVECTOR playerPositionTemp = XMLoadFloat3(&player->GetPosition());
	if (XMVector3Equal(lastPosTemp, playerPositionTemp)) {
		standingTimer += deltaTime;

		// Causes some weird errors with staticAlpha being null so it's commented out
		/*if (standingTimer >= 5.0f) {
			*staticAlpha += 0.5f * deltaTime;
		}
		else {
			standingTimer = 0.0f;
		}*/
	}
	XMStoreFloat3(&lastPos, playerPositionTemp);
}

void SlenderMan::GetDistance()
{
	XMVECTOR posTemp = XMLoadFloat3(GetPosition());
	XMVECTOR playerPositionTemp = XMLoadFloat3(&player->GetPosition());
	XMVECTOR distanceFromPlayer = posTemp - playerPositionTemp;
	distanceFromPlayer = XMVector3Length(distanceFromPlayer);
	XMFLOAT3 distTemp;
	XMStoreFloat3(&distTemp, distanceFromPlayer);
	distance = distTemp.x;
}

void SlenderMan::IncreaseLevel()
{
	agroLevel++;
}