#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "StateGameObject.h"

#include "NavigationGrid.h"
#include "NavigationPath.h"
#include "NavigationMap.h"
#include "Assets.h"

#define NUM_TARGETS 10
#define NUM_MAZE_TARGETS 10
namespace NCL {
	namespace CSC8503 {
		enum class ObjectIDs {
			player = (int)-2,
			enemy = (int)-3
		};

		

		class PowerUp {
		public:
			std::function<void(GameObject*)> startFunc;
			std::function<void(GameObject*)> endFunc;
			float timeLeft;
		};
		class FrictionPowerUp : public PowerUp {
		public:
			FrictionPowerUp(GameObject* player) {
				startFunc = [&](GameObject* player)->void {
					player->affectedByFriction = false;
				};
				endFunc = [&](GameObject* player)->void {
					player->affectedByFriction = true;
				};
			}
		};

		class Player : public GameObject {
		public:
			std::vector<PowerUp*> powerUps;
			void AddPowerUp(PowerUp* powerUp, float duration) {
				powerUp->startFunc(this);
				powerUp->timeLeft = duration;
				powerUps.push_back(powerUp);
			}
			void UpdatePowerUps(float dt) {
				std::vector<PowerUp*>::iterator it = powerUps.begin();
				while (it != powerUps.end()) {
					(*it)->timeLeft -= dt;
					if ((*it)->timeLeft <= 0) {
						(*it)->endFunc(this);
						it = powerUps.erase(it);
					}
					else it++;
				}
			}
		};

#pragma region targets
		

		class Target : public GameObject {
		public:
			Target() {};
			Target(GameWorld* world,int* score,std::vector<Target*>* parentVector, string objectName = "") : GameObject(objectName) {
				this->isTrigger = true;
				this->world = world;
				this->deleteOnTrigger = true;
				this->score = score;
				this->parentVector = parentVector;
			};
			 virtual void OnCollisionBegin(GameObject* otherObject) override {
				 //std::cout << "triggered";
				 if (otherObject->GetWorldID() == (int)ObjectIDs::player) (*score)++;
				 if (otherObject->GetWorldID() == (int)ObjectIDs::enemy) (*score)--;
				 if (parentVector != nullptr)DestroySelf();
			 }

			 void DestroySelf() {
				 std::vector<Target*>::iterator it = parentVector->begin();
				 while (it != parentVector->end()) {
					 if ((*it) == this) {
						 it = parentVector->erase(it);
					 }
					 else it++;
				 }
				 world->RemoveGameObject(this, true);
			 }
			 GameWorld* world;
			 std::vector<Target*>* parentVector;
			 int* score;
		};
		class FrictionTarget : public Target {
		public:
			FrictionTarget(GameWorld* world, int* score, std::vector<Target*>* parentVector, string objectName = "") : Target(world, score, parentVector, objectName) {

			};
			void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetWorldID() == (int)ObjectIDs::player || otherObject->GetWorldID() == (int)ObjectIDs::player) {
					FrictionPowerUp* powerUp = new FrictionPowerUp(otherObject);
					powerUp->startFunc(otherObject);
					((Player*)otherObject)->AddPowerUp((PowerUp*)powerUp, 5);
				}
				DestroySelf();
			}
		};
		
#pragma endregion

#pragma region PathfindingObject
		class PathfindingObject : public StateGameObject {
		public:
			PathfindingObject(NavigationGrid* grid, Vector3 startPos, Vector3 endPos,
				GameObject* player, GameWorld* world, std::vector<Target*>* mazeTargets);
			~PathfindingObject();

			virtual void Update(float dt);
			bool CanSeePlayer();
			Target* GetNearestMazeTarget(Vector3 from);
			std::vector<Target*>* mazeTargets;
			void DisplayPathfinding();
		protected:
			NavigationPath path;
			void FollowPath(float dt);
			StateMachine* stateMachine;
			bool finished;
			Vector3 dest;
			Vector3 destWaypoint;
			float time;
			GameObject* player;
			GameWorld* world;
			NavigationGrid* grid;
			int* testInt = new int(123456);


		};
#pragma endregion


		enum class Gamemode {
			normal,maze,rl
		};

		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);



			float camDistance;
			Vector3 lookAt;
			int score;

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys(float dt);

			void Reset();

			void RLMovement(float dt);
			void InitRL();
			void InitMaze();
			void MazeMovement(float dt);
			void UpdateMaze(float dt);
			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

			void BridgeConstraintTest();
			void HingeTest(Vector3 position, int nodeSize);

			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement(float dt);

			NavigationGrid* grid;
			void AddMazeToWorld();
			vector<Vector3> mazeNodes;

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddOBBToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			Player* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameObject* AddCarToWorld(const Vector3& position);
			Target* AddTargetToWorld(const Vector3& position, std::vector<Target*>* parentVector = nullptr);
			FrictionTarget* AddFrictionTargetToWorld(const Vector3& position, std::vector<Target*>* parentVector = nullptr);
			void GenerateTargets();
			Target* GetNearestTarget();
			
			void UpdateRLCam();

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			MeshGeometry*	capsuleMesh = nullptr;
			MeshGeometry*	cubeMesh	= nullptr;
			MeshGeometry*	sphereMesh	= nullptr;

			TextureBase*	basicTex	= nullptr;
			ShaderBase*		basicShader = nullptr;

			//Coursework Meshes
			MeshGeometry*	charMesh	= nullptr;
			MeshGeometry*	enemyMesh	= nullptr;
			MeshGeometry*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;

			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			PathfindingObject* AddPathfindingObjectToWorld();
			StateGameObject* testStateObject;

			std::vector<GameObject*> mazeAABBs;
			std::vector<Target*> mazeTargets;
			int targetIndex;
			PathfindingObject* pathfinder = nullptr;

			Player* player;

			Gamemode mode;
			
			Target* targets[NUM_TARGETS];
			Target nearestTarget;
		};
	}
}

