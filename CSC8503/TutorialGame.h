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

		

		
		
		class Player : public GameObject {
		public:
			Player(Vector3 spawnPoint, GameWorld* world) : GameObject() {
				//this->game = game;
				this->spawnPoint = spawnPoint;
				this->world = world;
				this->linearDamping = 2;
				this->angularDamping = 2;
			};
			/*std::vector<PowerUp*> powerUps;
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
			}*/
			GameWorld* world;
			Vector3 spawnPoint;
			//TutorialGame* game;
			float frictionTime;
			void Respawn() {
				GetTransform().SetPosition(spawnPoint);
				GetPhysicsObject()->ClearForces();
			};
			void UpdateFrictionTime(float dt) {
				frictionTime -= dt;
				if (frictionTime <= 0) {
					frictionTime = 0;
					affectedByFriction = true;
				}
			}
			GameObject* Shoot() {
				Vector3 localForward = GetTransform().GetOrientation() * Vector3(0, 0, 1);
				//Bullet* capsule = new Bullet(world);


				/*CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
				capsule->SetBoundingVolume((CollisionVolume*)volume);

				capsule->GetTransform()
					.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
					.SetPosition(position);

				capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
				capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

				capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
				capsule->GetPhysicsObject()->InitCubeInertia();

				world->AddGameObject(capsule);

				return capsule;*/

			}

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
				if (player->GetWorldID() == (int)ObjectIDs::player) {
					startFunc = [&](GameObject* player)->void {
						player->affectedByFriction = false;
						((Player*)player)->frictionTime = 5;
				};
				}
				else if (player->GetWorldID() == (int)ObjectIDs::enemy) {
					startFunc = [&](GameObject* player)->void {
						player->affectedByFriction = false;
						player->frictionTime = 5;
					};
				}
				/*startFunc = [&](GameObject* player)->void {
					player->affectedByFriction = false;
				};
				endFunc = [&](GameObject* player)->void {
					player->affectedByFriction = true;
				};*/
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
				 if (parentVector != nullptr) {
					 std::vector<Target*>::iterator it = parentVector->begin();
					 while (it != parentVector->end()) {
						 if ((*it) == this) {
							 it = parentVector->erase(it);
						 }
						 else it++;
					 }
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
				if (otherObject->GetWorldID() == (int)ObjectIDs::player) {
					FrictionPowerUp* powerUp = new FrictionPowerUp(otherObject);
					powerUp->startFunc(otherObject);
					//((Player*)otherObject)->AddPowerUp((PowerUp*)powerUp, 5);
				}
				else if (otherObject->GetWorldID() == (int)ObjectIDs::enemy) {
					FrictionPowerUp* powerUp = new FrictionPowerUp(otherObject);
					powerUp->startFunc(otherObject);
				}
				DestroySelf();
			}
		};
		
#pragma endregion

#pragma region PathfindingObject
		class PathfindingObject : public StateGameObject {
		public:
			PathfindingObject(NavigationGrid* grid, Vector3 startPos, 
				GameObject* player, GameWorld* world, std::vector<Target*>* mazeTargets
				, MeshGeometry* capsuleMesh, TextureBase* basicTex, ShaderBase* basicShader,std::vector<GameObject*>* mazeBullets);
			~PathfindingObject();

			void Shoot(Vector3 direction);

			virtual void Update(float dt);
			bool CanSeePlayer();
			Target* GetNearestMazeTarget(Vector3 from);
			std::vector<Target*>* mazeTargets;
			std::vector<GameObject*>* mazeBullets;
			void DisplayPathfinding();
			void Respawn();
			float frictionTime;
			void UpdateFrictionTime(float dt);
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
			Vector3 startPos;

			MeshGeometry* capsuleMesh;
			TextureBase* basicTex;
			ShaderBase* basicShader;
			float invFireRate;
			float lastShotTime;
			bool recalculatePath;

			
		};

		class Bullet : public GameObject {
		public:
			Bullet(GameWorld* world, float lifetime, std::vector<GameObject*>* parentVector, string objectName = "") : GameObject(objectName) {
				this->isTrigger = true;
				this->world = world;
				this->deleteOnTrigger = true;
				this->parentVector = parentVector;
				this->lifetime = lifetime;
				this->linearDamping = 0.1;
				this->angularDamping = 0.1;
				this->deleteMe = false;
				this->ignoreRaycast = true;
			}
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetWorldID() == (int)ObjectIDs::player) {
					((Player*)otherObject)->Respawn();
				}
				if (otherObject->GetWorldID() == (int)ObjectIDs::enemy) {
					((PathfindingObject*)otherObject)->Respawn();
				}
				deleteMe = true;
				/*if (parentVector != nullptr) {
					std::vector<GameObject*>::iterator it = parentVector->begin();
					while (it != parentVector->end()) {
						if ((*it) == this) {
							it = parentVector->erase(it);
						}
						else it++;
					}
				}
				world->RemoveGameObject(this, true);*/
			}
			bool Update(float dt) {
				lifetime -= dt;
				if (lifetime <= 0) {
					return false;
				}
				return true;
			};
			float lifetime;
			GameWorld* world;
			std::vector<GameObject*>* parentVector;
			bool deleteMe;
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

			GameObject* AddCapsuleToWorld(const Vector3& position, float radius, float halfHeight, float inverseMass = 10.0f);


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
			PathfindingObject* AddPathfindingObjectToWorld(const Vector3& pos);
			StateGameObject* testStateObject;

			std::vector<GameObject*> mazeAABBs;
			std::vector<Target*> mazeTargets;
			std::vector<GameObject*> mazeBullets;
			int targetIndex;
			PathfindingObject* pathfinder = nullptr;

			Player* player;

			Gamemode mode;
			
			Target* targets[NUM_TARGETS];
			Target nearestTarget;
		};
	}
}

