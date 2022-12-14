#include "OrientationConstraint.h"
#include "GameObject.h"
#include "PhysicsObject.h"
using namespace NCL;
using namespace Maths;
using namespace CSC8503;

OrientationConstraint::OrientationConstraint(GameObject* a, GameObject* b,Vector3 angle)
{
	objectA = a;
	objectB = b;

	this->angle = angle;
}

OrientationConstraint::~OrientationConstraint()
{

}
#include <Window.h>
void OrientationConstraint::UpdateConstraint(float dt) {
	
	Vector3 relativeOrientation = objectA->GetTransform().GetOrientation().ToEuler() - objectB->GetTransform().GetOrientation().ToEuler();
	//Vector3 orientationOffset = angle - relativeOrientation;
	float offset = 1 - Vector3::Dot(relativeOrientation.Normalised(), angle);

	if (abs(offset) > 0) {
		Vector3 offsetDir = Vector3::Cross(angle, relativeOrientation);

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeAngVel = Vector3::Cross(physA->GetAngularVelocity() - physB->GetAngularVelocity(),angle);

		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();
		if (constraintMass > 0) {
			float angVelDot = Vector3::Dot(relativeAngVel, offsetDir);
			float biasFactor = 0.01;
			float bias = -(biasFactor / dt) * offset;
			float lambda = -(angVelDot + bias) / constraintMass;
			if (lambda != lambda) {
				return;
			}
			if (lambda > 100000) {
				//lambda = 0;
			}
			if (lambda < 0) {
				lambda = 0;
			}

			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;

			physA->ApplyAngularImpulse(aImpulse);
			physB->ApplyAngularImpulse(bImpulse);
			if (Window::GetWindow()->GetKeyboard()->KeyDown(KeyboardKeys::L)) {
				bool a = false;
			}
			if (Window::GetWindow()->GetMouse()->ButtonDown(MouseButtons::RIGHT)) {
				bool a = false;
				//std::cout << offsetDir << '\n';
			}

		}
	}
}