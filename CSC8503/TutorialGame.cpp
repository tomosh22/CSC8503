#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"

#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")
#pragma comment(lib, "Xinput9_1_0.lib")


#include <fstream>

using namespace NCL;
using namespace CSC8503;

//#define LOCKED_CAMERA

#pragma region PathfindingObject
PathfindingObject::PathfindingObject(NavigationGrid* grid, Vector3 startPos,
	GameObject* player,GameWorld* world, std::vector<Target*>* mazeTargets) {
	
	stateMachine = new StateMachine();
	
	time = 0;
	this->player = player;
	this->world = world;
	this->mazeTargets = mazeTargets;
	this->grid = grid;
	std::cout << grid << '\n';
	dest = GetNearestMazeTarget(startPos)->GetTransform().GetPosition();
	bool found = this->grid->FindPath(startPos, dest, path);
	path.PopWaypoint(destWaypoint);
	*testInt = 98765;

	State* movingForward = new State(
		[&](float dt)->void {
			//std::cout << dest << ' ' << destWaypoint << '\n';
			Vector3 position = GetTransform().GetPosition();
			//std::cout << position << '\n';
			if ((position - destWaypoint).Length() < 5) {
				finished = path.PopWaypoint(destWaypoint);
			}
			if ((position - dest).Length() < 5) {
				if (this->mazeTargets->size() == 0)return;
				dest = GetNearestMazeTarget(GetTransform().GetPosition())->GetTransform().GetPosition();
				this->grid->FindPath(GetTransform().GetPosition(), dest, path);
				path.PopWaypoint(destWaypoint);
			}
			
			GetPhysicsObject()->AddForce((this->destWaypoint - position).Normalised() * 5);
			time += dt;
			Debug::DrawLine(dest, dest + Vector3(0, 1, 0),Vector4(0,1,0,1));
			Debug::DrawLine(destWaypoint, destWaypoint + Vector3(0, 1, 0),Vector4(1,0,0,1));
			//DisplayPathfinding();
		}
	);
	State* chasingPlayer = new State(
		[&](float dt)->void {
			Vector3 position = GetTransform().GetPosition();
			GetPhysicsObject()->AddForce(this->player->GetTransform().GetPosition() - position);

		}
	);
	stateMachine->AddState(movingForward);
	stateMachine->AddState(chasingPlayer);

	//stateMachine->AddTransition(new StateTransition(movingForward, chasingPlayer, [&]()->bool {return CanSeePlayer(); }));
	//stateMachine->AddTransition(new StateTransition(chasingPlayer, movingForward, [&]()->bool {return !CanSeePlayer(); }));

}

void PathfindingObject::Update(float dt) {
	stateMachine->Update(dt);
	//std::cout << CanSeePlayer();
}

bool PathfindingObject::CanSeePlayer() {
	Vector3 dirToPlayer = player->GetTransform().GetPosition() - GetTransform().GetPosition();
	dirToPlayer.Normalise();
	Ray ray(GetTransform().GetPosition(),dirToPlayer);
	RayCollision closestCollision;
	if (world->Raycast(ray, closestCollision, true,this)) {
		if (closestCollision.node == player) {
			return true;
		}
		else return false;
	}


}

Target* NCL::CSC8503::PathfindingObject::GetNearestMazeTarget(Vector3 from)
{
	if (mazeTargets->size() == 0)return nullptr;
	Target* target = mazeTargets->at(0);
	float lowestDist = (from - target->GetTransform().GetPosition()).LengthSquared();
	for (int i = 1; i < mazeTargets->size(); i++)
	{
		float dist = (from - mazeTargets->at(i)->GetTransform().GetPosition()).LengthSquared();
		if (dist < lowestDist) {
			lowestDist = dist;
			target = mazeTargets->at(i);
		}
	}
	target->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
	return target;
}

void PathfindingObject::DisplayPathfinding() {
	if (mazeTargets->size() < 2)return;
	for (int i = 0; i < mazeTargets->size()-1; ++i) {
		Vector3 a = mazeTargets->at(i)->GetTransform().GetPosition();
		Vector3 b = mazeTargets->at(i+1)->GetTransform().GetPosition();
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}


//Target* PathfindingObject::GetNearestMazeTarget(Vector3 from) {
//	Target* target = mazeTargets[0];
//	float lowestDist = (from - target->GetTransform().GetPosition()).Length();
//	for (int i = 1; i < 100; i++)
//	{
//		if (mazeTargets[i] == NULL)continue;
//		float dist = (from - mazeTargets[i]->GetTransform().GetPosition()).Length();
//		if (dist < lowestDist) {
//			dist = lowestDist;
//			target = mazeTargets[i];
//		}
//	}
//	return target;
//}

#pragma endregion


#pragma region TutorialGame

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	InitialiseAssets();
	srand(time(0));
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	charMesh	= renderer->LoadMesh("goat.msh");
	enemyMesh	= renderer->LoadMesh("Keeper.msh");
	bonusMesh	= renderer->LoadMesh("apple.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");

	InitCamera();
	InitWorld();
	
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateRLCam() {
	Vector3 objPos = lockedObject->GetTransform().GetPosition();

	lookAt = GetNearestTarget()->GetTransform().GetPosition();



	Vector3 lookDir = lookAt - objPos;
	lookDir.Normalise();


	Vector3 camPos = objPos - lookDir * 20
		//+ lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1) * -20
		+ Vector3(0, 1, 0) * 20;



	//lookAt = objPos + lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, -1) + lockedObject->GetTransform().GetOrientation() * Vector3(0, 1, 0);

	Matrix4 temp = Matrix4::BuildViewMatrix(camPos, lookAt, Vector3(0, 1, 0));

	Matrix4 modelMat = temp.Inverse();

	Quaternion q(modelMat);
	Vector3 angles = q.ToEuler(); //nearly there now!

	world->GetMainCamera()->SetPosition(camPos);
	world->GetMainCamera()->SetPitch(angles.x);
	world->GetMainCamera()->SetYaw(angles.y);
}

void TutorialGame::UpdateMaze(float dt) {
	Debug::Print("Score "+std::to_string(score), Vector2(5,5));
	if (mazeTargets.size() == 0) {
		if (score == 0)std::cout << "draw";
		else std::cout << ((score > 0) ? "win" : "lose");
	}
	//player->UpdatePowerUps(dt);
	if (player->frictionTime > 0) {
		player->UpdateFrictionTime(dt);
	}
	//std::cout << player->affectedByFriction << '\n';

}

void TutorialGame::UpdateGame(float dt) {
	if (mode == Gamemode::maze)UpdateMaze(dt);
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		if (mode == Gamemode::rl)UpdateRLCam();


		
	}

	UpdateKeys(dt);

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
	}

	RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world->Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		}
	}

	Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));

	SelectObject();
	MoveSelectedObject();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	Debug::UpdateRenderables(dt);


	if (testStateObject)testStateObject->Update(dt);
	if (pathfinder != NULL && pathfinder != nullptr) {
		pathfinder->Update(dt);
	}
}

void TutorialGame::UpdateKeys(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		selectionObject = nullptr;
		InitWorld(); //We can reset the simulation at any time with F1
		
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::M)) {
		selectionObject = nullptr;
		InitMaze();
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::N)) {
		selectionObject = nullptr;
		InitRL();
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement(dt);
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::RLMovement(float dt) {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();


	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis * 10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -1, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 1, 0));
	}

	Quaternion localRotation = selectionObject->GetTransform().GetOrientation();
	Vector3 localUp = localRotation * Vector3(0, 1, 0);
	Vector3 localForward = localRotation * Vector3(0, 0, -1);
	Vector3 localRight = localRotation * Vector3(1, 0, 0);

	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));
	XInputGetState(0, &state);

	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)selectionObject->GetPhysicsObject()->ApplyLinearImpulse(localUp);
	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)selectionObject->GetPhysicsObject()->AddForce(localForward * 50);
	float leftTrigger = state.Gamepad.bLeftTrigger;
	float leftRight = state.Gamepad.bRightTrigger;

	float leftStickX = state.Gamepad.sThumbLX;
	if (leftStickX > 32767)leftStickX = 32767;
	if (leftStickX < -32767)leftStickX = -32767;

	float leftStickY = state.Gamepad.sThumbLY;
	if (leftStickY > 32767)leftStickY = 32767;
	if (leftStickY < -32767)leftStickY = -32767;





	//std::cout << selectionObject->GetPhysicsObject()->GetAngularVelocity() << '\n';


	PhysicsObject* phys = selectionObject->GetPhysicsObject();
	Vector3 angVel = selectionObject->GetTransform().GetOrientation().Conjugate() * phys->GetAngularVelocity();

#pragma region pitch
	//pitch
	leftStickY *= -1; //i got the axes the wrong way round... oops
	if (leftStickY > 10000) {
		angVel.x += std::lerp((float)0, (float)12.46, leftStickY / 32767) * dt;
		if (angVel.x > 3.14 * 2)angVel.x = 3.14 * 2;
	}
	else {
		if (leftStickY < -10000) {
			angVel.x -= std::lerp((float)0, (float)12.46, -leftStickY / 32767) * dt;
			if (angVel.x < -3.14 * 2)angVel.x = -3.14 * 2;
		}
		else {
			if (std::fabs(angVel.x) > 0) {
				if (angVel.x > 0) {
					angVel.x -= 12.46 * dt;
					if (angVel.x < 0)angVel.x = 0;
				}
				else {
					angVel.x += 12.46 * dt;
					if (angVel.x > 0)angVel.x = 0;
				}
			}
		}

	}
#pragma endregion

#pragma region yaw
	//yaw
	leftStickX *= -1; //i got the axes the wrong way round... oops
	if (leftStickX > 10000) {
		angVel.y += std::lerp((float)0, (float)9.11, leftStickX / 32767) * dt;
		if (angVel.y > 3.14 * 2)angVel.y = 3.14 * 2;
	}
	else {
		if (leftStickX < -10000) {
			angVel.y -= std::lerp((float)0, (float)9.11, -leftStickX / 32767) * dt;
			if (angVel.y < -3.14 * 2)angVel.y = -3.14 * 2;
		}
		else {
			if (std::fabs(angVel.y) > 0) {
				if (angVel.y > 0) {
					angVel.y -= 9.11 * dt;
					if (angVel.y < 0)angVel.y = 0;
				}
				else {
					angVel.y += 9.11 * dt;
					if (angVel.y > 0)angVel.y = 0;
				}
			}
		}

	}
#pragma endregion

#pragma region roll
	//roll

	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
		angVel.z += 38.34 * dt;
		if (angVel.z > 3.14 * 2)angVel.z = 3.14 * 2;
	}
	else {
		if (angVel.z > 0) {
			angVel.z -= 38.34;
			if (angVel.z < 0)angVel.z = 0;
		}
	}
#pragma endregion

	phys->SetAngularVelocity(selectionObject->GetTransform().GetOrientation() * angVel);

	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localForward, Vector4(0, 0, 1, 1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localUp, Vector4(0, 1, 0, 1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localRight, Vector4(1, 0, 0, 1));

}

void TutorialGame::MazeMovement(float dt) {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Quaternion localRotation = selectionObject->GetTransform().GetOrientation();
	Vector3 localUp = localRotation * Vector3(0, 1, 0);
	Vector3 localForward = localRotation * Vector3(0, 0, -1);
	Vector3 localRight = localRotation * Vector3(1, 0, 0);

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(localForward * 100);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-localForward * 100);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddTorque(-localUp * 100);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddTorque(localUp * 100);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RETURN)) {
		selectionObject->GetPhysicsObject()->AddForce(localUp * 50);
	}
	

	

	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localForward, Vector4(0, 0, 1, 1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localUp, Vector4(0, 1, 0, 1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localRight, Vector4(1, 0, 0, 1));

}

void TutorialGame::LockedObjectMovement(float dt) {
	if (mode == Gamemode::rl) { RLMovement(dt); return; }
	if (mode == Gamemode::maze) { MazeMovement(dt); return; }
	return;
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();


	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis*10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddTorque(Vector3(0,-1,0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 1, 0));
	}

	Quaternion localRotation = selectionObject->GetTransform().GetOrientation();
	Vector3 localUp = localRotation * Vector3(0, 1, 0);
	Vector3 localForward = localRotation * Vector3(0, 0, -1);
	Vector3 localRight = localRotation * Vector3(1, 0, 0);

	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));
	XInputGetState(0, &state);

	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)selectionObject->GetPhysicsObject()->ApplyLinearImpulse(localUp);
	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)selectionObject->GetPhysicsObject()->AddForce(localForward * 50);
	float leftTrigger = state.Gamepad.bLeftTrigger;
	float leftRight = state.Gamepad.bRightTrigger;

	float leftStickX = state.Gamepad.sThumbLX;
	if (leftStickX > 32767)leftStickX = 32767;
	if (leftStickX < -32767)leftStickX = -32767;

	float leftStickY = state.Gamepad.sThumbLY;
	if (leftStickY > 32767)leftStickY = 32767;
	if (leftStickY < -32767)leftStickY = -32767;

	

	

	//std::cout << selectionObject->GetPhysicsObject()->GetAngularVelocity() << '\n';

	
	PhysicsObject* phys = selectionObject->GetPhysicsObject();
	Vector3 angVel = selectionObject->GetTransform().GetOrientation().Conjugate() * phys->GetAngularVelocity();

#pragma region pitch
	//pitch
	leftStickY *= -1; //i got the axes the wrong way round... oops
	if (leftStickY > 10000) {
		angVel.x += std::lerp((float)0, (float)12.46, leftStickY / 32767) * dt;
		if (angVel.x > 3.14*2)angVel.x = 3.14*2;
	}
	else {
		if (leftStickY < -10000) {
			angVel.x -= std::lerp((float)0, (float)12.46, -leftStickY / 32767) * dt;
			if (angVel.x < -3.14*2)angVel.x = -3.14*2;
		}
		else {
			if (std::fabs(angVel.x) > 0) {
				if (angVel.x > 0) {
					angVel.x -= 12.46 * dt;
					if (angVel.x < 0)angVel.x = 0;
				}
				else {
					angVel.x += 12.46 * dt;
					if (angVel.x > 0)angVel.x = 0;
				}
			}
		}
		
	}
#pragma endregion

#pragma region yaw
	//yaw
	leftStickX *= -1; //i got the axes the wrong way round... oops
	if (leftStickX > 10000) {
		angVel.y += std::lerp((float)0, (float)9.11, leftStickX / 32767) * dt;
		if (angVel.y > 3.14 * 2)angVel.y = 3.14 * 2;
	}
	else {
		if (leftStickX < -10000) {
			angVel.y -= std::lerp((float)0, (float)9.11, -leftStickX / 32767) * dt;
			if (angVel.y < -3.14 * 2)angVel.y = -3.14 * 2;
		}
		else {
			if (std::fabs(angVel.y) > 0) {
				if (angVel.y > 0) {
					angVel.y -= 9.11* dt;
					if (angVel.y < 0)angVel.y = 0;
				}
				else {
					angVel.y += 9.11 * dt;
					if (angVel.y > 0)angVel.y = 0;
				}
			}
		}

	}
#pragma endregion

#pragma region roll
	//roll
	
	if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
		angVel.z += 38.34 * dt;
		if (angVel.z > 3.14 * 2)angVel.z = 3.14 * 2;
	}
	else {
		if (angVel.z > 0) {
			angVel.z -= 38.34;
			if (angVel.z < 0)angVel.z = 0;
		}
	}
#pragma endregion
	
	phys->SetAngularVelocity(selectionObject->GetTransform().GetOrientation() * angVel);

	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localForward,Vector4(0,0,1,1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localUp,Vector4(0,1,0,1));
	Debug::DrawLine(selectionObject->GetTransform().GetPosition(), selectionObject->GetTransform().GetPosition() + localRight,Vector4(1,0,0,1));

}



void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::Reset() {
	world->ClearAndErase();
	physics->Clear();
	pathfinder = nullptr;
	score = 0;
	mazeTargets.clear();
}

void TutorialGame::GenerateTargets() {
	for (int i = 0; i < NUM_TARGETS; i++)
	{
		Vector3 pos;
		pos.x = rand() % 50;
		pos.y = rand() % 50;
		pos.z = rand() % 50;
		targets[i] = AddTargetToWorld(pos);
	}
}

Target* TutorialGame::GetNearestTarget() {
	Target* target = targets[0];
	float lowestDist = (player->GetTransform().GetPosition() - target->GetTransform().GetPosition()).Length();
	for (int i = 1; i < NUM_TARGETS; i++)
	{
		float dist = (player->GetTransform().GetPosition() - targets[i]->GetTransform().GetPosition()).Length();
		if (dist < lowestDist) {
			dist = lowestDist;
			target = targets[i];
		}
	}
	return target;
}


void TutorialGame::InitRL() {
	Reset();
	lockedObject = AddCarToWorld(Vector3(0,10,0));
	selectionObject = lockedObject;
	InitDefaultFloor();
	GenerateTargets();
	mode = Gamemode::rl;
}

void TutorialGame::InitMaze() {
	Reset();
	InitDefaultFloor();
	AddMazeToWorld();
	player = AddPlayerToWorld(Vector3(10,0,180));
	player->GetRenderObject()->SetColour(Vector4(1, 0.6, 1, 1));
	pathfinder = AddPathfindingObjectToWorld(Vector3(180, 0, 10));
	lockedObject = player;
	selectionObject = lockedObject;
	mode = Gamemode::maze;
}

void TutorialGame::InitWorld() {
	Reset();

	//player = AddPlayerToWorld(Vector3(80, 10, 45));
	AddSphereToWorld(Vector3(90, 10, 55),2);
	AddCapsuleToWorld(Vector3(70, 10, 35),2,8);
	AddCapsuleToWorld(Vector3(85, 10, 35),2,8);
	lockedObject = player;
	selectionObject = lockedObject;
	mode = Gamemode::normal;

	InitMixedGridWorld(15, 15, 3.5f, 3.5f);
	
	//AddOBBToWorld(Vector3(-25,0,-30),Vector3(1,1,1)*5);
	//AddOBBToWorld(Vector3(-25,0,-10), Vector3(1, 1, 1)*5);

	//AddSphereToWorld(Vector3(-45, 0, -30), 5);
	//AddSphereToWorld(Vector3(-45, 0, -10), 5);

	//InitGameExamples();
	//InitDefaultFloor();

	//BridgeConstraintTest();
	HingeTest(Vector3(),1);

	//AddMazeToWorld();
	//testStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));
	//pathfinder = AddPathfindingObjectToWorld();
}

void TutorialGame::AddMazeToWorld() {
	grid = new NavigationGrid("TestGrid1.txt");

	int nodeSize, gridWidth, gridHeight;

	std::ifstream infile(Assets::DATADIR + "TestGrid1.txt");

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;

	GridNode* allNodes = new GridNode[gridWidth * gridHeight];

	targetIndex = 0;
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];
			char type = 0;
			infile >> type;
			n.type = type;
			n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize));
			std::cout << type << '\n';
			if (type == 120)mazeAABBs.emplace_back(AddOBBToWorld(n.position, { (float)nodeSize / 2,(float)nodeSize / 2,(float)nodeSize / 2 }, 0));
			else if (type == 103)HingeTest(n.position, nodeSize);
			else if (rand() % 10 == 0)mazeTargets.emplace_back(AddTargetToWorld(n.position, &mazeTargets));
			else if (rand() % 10 == 0)AddFrictionTargetToWorld(n.position, nullptr);
			
		}
	}
	return;
}

Target* TutorialGame::AddTargetToWorld(const Vector3& position, std::vector<Target*>* parentVector) {
	Target* sphere = new Target(this->world,&score,parentVector);
	float radius = 1;


	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(0);
	sphere->GetPhysicsObject()->InitSphereInertia();


	world->AddGameObject(sphere);
	
	

	return sphere;
}

FrictionTarget* TutorialGame::AddFrictionTargetToWorld(const Vector3& position, std::vector<Target*>* parentVector) {
	FrictionTarget* sphere = new FrictionTarget(this->world, &score,parentVector);
	float radius = 1;


	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->GetRenderObject()->SetColour(Vector4(1, 0.5, 0, 1));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(0);
	sphere->GetPhysicsObject()->InitSphereInertia();

	sphere->parentVector = parentVector;

	world->AddGameObject(sphere);



	return sphere;
}

GameObject* TutorialGame::AddCarToWorld(const Vector3& position) {
	GameObject* car = new GameObject();

	Vector3 carSize = Vector3(3, 2, 5);
	OBBVolume* volume = new OBBVolume(carSize);
	car->SetBoundingVolume((CollisionVolume*)volume);
	car->GetTransform()
		.SetScale(carSize * 2)
		.SetPosition(position)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0,0,0));

	car->SetRenderObject(new RenderObject(&car->GetTransform(), cubeMesh, basicTex, basicShader));
	car->SetPhysicsObject(new PhysicsObject(&car->GetTransform(), car->GetBoundingVolume()));

	car->GetPhysicsObject()->SetInverseMass(0.5);
	car->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(car,-2);


	return car;
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(200, 2, 200);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float radius,float halfHeight, float inverseMass) {
	GameObject* capsule = new GameObject();

	
	CapsuleVolume* volume = new CapsuleVolume(halfHeight,radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius*2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();


	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddOBBToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();


	OBBVolume* volume = new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);
		//.SetOrientation(Quaternion::EulerAnglesToQuaternion(20,20,20));

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

Player* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize		= 5;
	float inverseMass	= 0.5f;

	Player* character = new Player();
	//SphereVolume* volume  = new SphereVolume(meshSize);
	OBBVolume* volume = new OBBVolume(Vector3(1, 1, 1)*meshSize/2);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();


	world->AddGameObject(character,(int)ObjectIDs::player);

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), sphereMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

PathfindingObject* TutorialGame::AddPathfindingObjectToWorld(const Vector3& position) {
	PathfindingObject* apple = new PathfindingObject(grid,position,player,world,&mazeTargets);
	float radius = 5;
	SphereVolume* volume = new SphereVolume(radius);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(1, 1, 1)*radius)
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), sphereMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	
	

	world->AddGameObject(apple,(int)ObjectIDs::enemy);

	return apple;
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -6, 0));
}

void TutorialGame::InitGameExamples() {
#ifdef LOCKED_CAMERA
	lockedObject = AddPlayerToWorld(Vector3(0, 5, 0));
	selectionObject = lockedObject;
#else
	AddPlayerToWorld(Vector3(0, 5, 0));
#endif
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float invCubeMass = 5;
	int numLinks = 10;
	float maxDistance = 30;
	float cubeDistance = 20;

	Vector3 startPos = Vector3(50, 50, 50);

	GameObject* start = AddCubeToWorld(startPos, cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);
	GameObject* prev = start;

	for (int i = 0; i < numLinks; i++)
	{
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(prev, block, maxDistance);
		world->AddConstraint(constraint);
		prev = block;
	}
	PositionConstraint* constraint = new PositionConstraint(prev, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::HingeTest(Vector3 position, int nodeSize) {
	nodeSize = 10;
	Vector3 doorSize = Vector3(3, 2, 0.1);
	GameObject* door = AddOBBToWorld(position - Vector3(nodeSize / 2, 0, 0), doorSize, 0.5);

	GameObject* hinge = AddCubeToWorld(position - Vector3(nodeSize, 0, 0), Vector3(1, 1, 1), 0);

	OrientationConstraint* constraint1 = new OrientationConstraint(door, hinge, Vector3(0, 1, 0));
	world->AddConstraint(constraint1);

	PositionConstraint* constraint3 = new PositionConstraint(door, hinge, 10);
	world->AddConstraint(constraint3);
}
/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(5, 90));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}

#pragma endregion