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
#pragma region PathfindingObject
		class PathfindingObject : public StateGameObject {
		public:
			PathfindingObject(NavigationGrid* grid, Vector3 startPos, Vector3 endPos,
				GameObject* player,GameWorld* world, std::vector<GameObject*> mazeTargets);
			~PathfindingObject();

			virtual void Update(float dt);
			bool CanSeePlayer();
			GameObject* GetNearestMazeTarget(Vector3 from);
			std::vector<GameObject*> mazeTargets;
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

		class Target : public GameObject {
		public:
			Target() {};
			Target(GameWorld* world,string objectName = "") : GameObject(objectName) {
				isTrigger = true;
				this->world = world;
				deleteOnTrigger = true;
			};
			 void OnCollisionBegin(GameObject* otherObject) override {
				 //std::cout << "triggered";
				 if (parentVector != nullptr) {
					 for (int i = 0; i < parentVector->size(); i++)
					 {
						 if (parentVector->at(i) == this) {
							 parentVector->erase(parentVector->begin() + i);
						 }
					 }
				 };
				 world->RemoveGameObject(this, true);
				 
			 }
			 GameWorld* world;
			 std::vector<GameObject*>* parentVector;
		};

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

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys(float dt);

			void Reset();

			void RLMovement(float dt);
			void InitRL();
			void InitMaze();
			void MazeMovement(float dt);
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

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameObject* AddCarToWorld(const Vector3& position);
			Target* AddTargetToWorld(const Vector3& position, std::vector<GameObject*>* parentVector = nullptr);
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
			std::vector<GameObject*> mazeTargets;
			int targetIndex;
			PathfindingObject* pathfinder = nullptr;

			GameObject* player;

			Gamemode mode;
			int score;
			Target* targets[NUM_TARGETS];
			Target nearestTarget;
		};
	}
}

