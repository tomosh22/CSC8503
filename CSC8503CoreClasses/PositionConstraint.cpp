#include "PositionConstraint.h"
//#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "PhysicsObject.h"
//#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d)
{
	objectA		= a;
	objectB		= b;
	distance	= d;
	this->axis = axis;
}

PositionConstraint::~PositionConstraint()
{

}

//a simple constraint that stops objects from being more than <distance> away
//from each other...this would be all we need to simulate a rope, or a ragdoll
void PositionConstraint::UpdateConstraint(float dt)	{
	Vector3 relativePos = objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition();
	float currentDistance = relativePos.Length();
	if (currentDistance > distance) {
		float offset = distance - currentDistance;
		if (abs(offset) > 0) {
			Vector3 offsetDir = relativePos.Normalised();

			PhysicsObject* physA = objectA->GetPhysicsObject();
			PhysicsObject* physB = objectB->GetPhysicsObject();

			Vector3 relativeVel = physA->GetLinearVelocity() - physB->GetLinearVelocity();

			float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();
			if (constraintMass > 0) {
				float velocityDot = Vector3::Dot(relativeVel, offsetDir);
				float biasFactor = 0.01;
				float bias = -(biasFactor / dt) * offset;
				float lambda = -(velocityDot + bias) / constraintMass;

				Vector3 aImpulse = offsetDir * lambda;
				Vector3 bImpulse = -offsetDir * lambda;

				physA->ApplyLinearImpulse(aImpulse);
				physB->ApplyLinearImpulse(bImpulse);
			}
		}
	}

	float relativeXPos = objectA->GetTransform().GetPosition().x - objectB->GetTransform().GetPosition().x;
	if (relativeXPos == 0)return;
	PhysicsObject* physA = objectA->GetPhysicsObject();
	PhysicsObject* physB = objectB->GetPhysicsObject();

	Vector3 relativeVel = physA->GetLinearVelocity() - physB->GetLinearVelocity();

	float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();
	if (constraintMass > 0) {
		float velocityDot = Vector3::Dot(relativeVel, -Vector3(1,0,0));
		float biasFactor = 0.01;
		float bias = -(biasFactor / dt) * relativeXPos;
		float lambda = -(velocityDot + bias) / constraintMass;

		Vector3 aImpulse = -Vector3(1,0,0) * lambda;
		Vector3 bImpulse = Vector3(1, 0, 0) * lambda;

		physA->ApplyLinearImpulse(aImpulse);
		physB->ApplyLinearImpulse(bImpulse);
	}

}
