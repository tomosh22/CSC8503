#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "Window.h"
#include "Maths.h"
#include "Debug.h"

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}

	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r, GameObject& object, RayCollision& collision) {


	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume = object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
	case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume, collision); break;
	case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume, collision); break;
	case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume, collision); break;

	case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1);

	for (int i = 0; i < 3; i++)
	{
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}
	float bestT = tVals.GetMaxElement();
	if (bestT < 0)return false;

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001;
	for (int i = 0; i < 3; i++)
	{
		if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) return false;
	}
	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray& r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray& r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());

	Vector3 localRayPos = r.GetPosition() - position;

	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());

	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);
	if (collided)collision.collidedAt = transform * collision.collidedAt + position;
	return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray& r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	Vector3 dir = spherePos - r.GetPosition();

	float sphereProj = Vector3::Dot(dir, r.GetDirection());
	if (sphereProj < 0)return false;

	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);

	float sphereDist = (point - spherePos).Length();
	if (sphereDist > sphereRadius)return false;

	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

	collision.rayDistance = sphereProj - offset;
	collision.collidedAt = r.GetPosition() + r.GetDirection() * collision.rayDistance;
	return true;
}

CollisionDetection::CapsuleParts CollisionDetection::GetCapsuleParts(const Transform& worldTransform, const CapsuleVolume& volume) {
	Vector3 localUp = worldTransform.GetOrientation() * Vector3(0, 1, 0);
	Transform topTransform;
	topTransform.SetPosition(worldTransform.GetPosition() + localUp * (volume.GetHalfHeight() - volume.GetRadius()));
	Transform midTransform;
	midTransform.SetPosition(worldTransform.GetPosition());
	Transform bottomTransform;
	bottomTransform.SetPosition(worldTransform.GetPosition() - localUp * (volume.GetHalfHeight()-volume.GetRadius()));
	SphereVolume sphereVol(volume.GetRadius());
	OBBVolume obbVol(Vector3(volume.GetRadius(), volume.GetHalfHeight() - volume.GetRadius(), volume.GetRadius()));
	CollisionDetection::CapsuleParts parts;
	parts.localUp = localUp;
	parts.topTransform = topTransform;
	parts.midTransform = midTransform;
	parts.bottomTransform = bottomTransform;
	parts.sphereVol = sphereVol;
	parts.obbVol = obbVol;
	return parts;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	CapsuleParts parts = GetCapsuleParts(worldTransform, volume);
	

	if (RaySphereIntersection(r, parts.topTransform, parts.sphereVol, collision)) {
		collision.collidedAt += parts.localUp * volume.GetHalfHeight();
		return true;
	}
	if (RaySphereIntersection(r, parts.bottomTransform, parts.sphereVol, collision)) {
		collision.collidedAt -= parts.localUp * volume.GetHalfHeight();
		return true;
	}
	return RayOBBIntersection(r, parts.midTransform, parts.obbVol, collision);
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	//Two AABBs
	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Spheres
	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	//Two OBBs
	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Capsules

	//AABB vs Sphere pairs
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//OBB vs sphere pairs
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//OBB vs AABB pairs
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::AABB) {
		return OBBAABBIntersection((OBBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBAABBIntersection((OBBVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}

	//Capsule vs other interactions
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Capsule) {
		return CapsuleIntersection((CapsuleVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		return AABBCapsuleIntersection((CapsuleVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volB->type == VolumeType::Capsule && volA->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;

	if (abs(delta.x) < totalSize.x &&
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();
	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);
	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1, 0, 0), Vector3(1, 0, 0),
			Vector3(0, -1, 0), Vector3(0, 1, 0),
			Vector3(0, 0, -1), Vector3(0, 0, 1),
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;
		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] = {
			maxB.x - minA.x,
			maxA.x - minB.x,
			maxB.y - minA.y,
			maxA.y - minB.y,
			maxB.z - minA.z,
			maxA.z - minB.z,
		};
		float pen = FLT_MAX;
		Vector3 bestAxis;
		for (int i = 0; i < 6; i++)
		{
			if (distances[i] < pen) {
				pen = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, pen,false);
		return true;
	}
	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	float deltaLength = delta.Length();

	if (deltaLength < radii) {
		float pen = radii - deltaLength;
		Vector3 normal = delta.Normalised();
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();
		collisionInfo.AddContactPoint(localA, localB,normal, pen,false);
		return true;
	}

	return false;
}

#include <Window.h>
//https://wickedengine.net/2020/04/26/capsule-collision-detection/
Vector3 ClosestPointOnLineSegment(Vector3 a, Vector3 b, Vector3 point) {
	Vector3 ab = b - a;
	float t = Vector3::Dot(point - a, ab) / Vector3::Dot(ab,ab);
	return a + ab * Clamp(t,(float)0,(float)1);
}
bool CollisionDetection::CapsuleIntersection(const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 aUp = worldTransformA.GetOrientation() * Vector3(0, 1, 0);
	Vector3 aTip = (worldTransformA.GetPosition() + aUp * (volumeA.GetHalfHeight() + volumeA.GetRadius()));
	Vector3 aBase = (worldTransformA.GetPosition() - aUp * (volumeA.GetHalfHeight() + volumeA.GetRadius()));
	Vector3 aNormal = (aTip - aBase).Normalised();
	Vector3 aLineEndOffset = aNormal * volumeA.GetRadius();
	Vector3 aA = aBase + aLineEndOffset;
	Vector3 aB = aTip - aLineEndOffset;

	Vector3 bUp = worldTransformB.GetOrientation() * Vector3(0, 1, 0);
	Vector3 bTip = (worldTransformB.GetPosition() + bUp * (volumeB.GetHalfHeight() + volumeB.GetRadius()));
	Vector3 bBase = (worldTransformB.GetPosition() - bUp * (volumeB.GetHalfHeight() + volumeB.GetRadius()));
	Vector3 bNormal = (bTip - bBase).Normalised();
	Vector3 bLineEndOffset = bNormal * volumeB.GetRadius();
	Vector3 bA = bBase + bLineEndOffset;
	Vector3 bB = bTip - bLineEndOffset;

	Vector3 v0 = bA - aA;
	Vector3 v1 = bB - aA;
	Vector3 v2 = bA - aB;
	Vector3 v3 = bB - aB;

	float d0 = Vector3::Dot(v0, v0);
	float d1 = Vector3::Dot(v1, v1);
	float d2 = Vector3::Dot(v2, v2);
	float d3 = Vector3::Dot(v3, v3);

	Vector3 bestA;
	if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) bestA = aB;
	else bestA = aA;
	

	Vector3 bestB = ClosestPointOnLineSegment(bA, bB, bestA);
	bestA = ClosestPointOnLineSegment(aA, aB, bestB);

	Vector3 delta = bestB - bestA;
	float deltaLen = delta.Length();
	if (Window::GetWindow()->GetKeyboard()->KeyDown(KeyboardKeys::L)) {
		bool a = false;
	}
	if (volumeA.GetRadius() + volumeB.GetRadius() - deltaLen > 0) {
		float pen = volumeA.GetRadius() + volumeB.GetRadius() - (bestB - bestA).Length();
		Vector3 normal = (bestB - bestA).Normalised();
		collisionInfo.AddContactPoint(bestA - normal * volumeA.GetRadius(), bestB + normal * volumeB.GetRadius(),normal ,pen, false);
		return true;
	}
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();
	if (distance < volumeB.GetRadius()) {
		Vector3 collisionNormal = localPoint.Normalised();
		float pen = volumeB.GetRadius() - distance;
		Vector3 localA, localB;
		
		localA = Vector3();
		localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, pen,false);
		return true;
	}

	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersectionSwapped(const SphereVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeB.GetHalfDimensions();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();
	if (distance < volumeA.GetRadius()) {
		Vector3 collisionNormal = localPoint.Normalised();
		float pen = volumeA.GetRadius() - distance;
		Vector3 localA, localB;

		localA = Vector3();
		localB = -collisionNormal * volumeA.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, pen, false);
		return true;
	}

	return false;
}

bool  CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	

	Vector3 deltaPos = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	deltaPos = worldTransformA.GetOrientation().Conjugate() * deltaPos;

	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 closestPointOnBox = Maths::Clamp(deltaPos, -boxSize, boxSize);
	Vector3 localPoint = deltaPos - closestPointOnBox;
	float distance = localPoint.Length();
	if (distance < volumeB.GetRadius()) {
		Vector3 collisionNormal = worldTransformA.GetOrientation() * localPoint.Normalised();
		float pen = volumeB.GetRadius() - distance;

		Vector3 localA = closestPointOnBox;
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, pen, false);
		return true;
	}

	return false;
}

bool  CollisionDetection::OBBAABBIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	if (OBBIntersection(volumeA, worldTransformA, (OBBVolume&)volumeB, worldTransformB, collisionInfo)) {
		collisionInfo.point.localB = Vector3();
		return true;
	}
	return false;
}


bool CollisionDetection::AABBCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
	/*CapsuleParts parts = GetCapsuleParts(worldTransformA, volumeA);

	if (AABBSphereIntersectionSwapped(parts.sphereVol, parts.topTransform, volumeB, worldTransformB, collisionInfo)) {
		std::cout << "collided with top sphere\n";
		collisionInfo.point.localA += parts.localUp * volumeA.GetHalfHeight();
		collisionInfo.point.normal = (collisionInfo.point.localA - worldTransformA.GetPosition()).Normalised();
		return true;
	}
	if (AABBSphereIntersectionSwapped(parts.sphereVol, parts.bottomTransform, volumeB, worldTransformB, collisionInfo)) {
		std::cout << "collided with bottom sphere\n";
		collisionInfo.point.localA -= parts.localUp * volumeA.GetHalfHeight();
		collisionInfo.point.normal = (collisionInfo.point.localA - worldTransformA.GetPosition()).Normalised();
		return true;
	}
	if (OBBAABBIntersection(parts.obbVol, parts.midTransform, volumeB, worldTransformB, collisionInfo)) {
		std::cout << "collided with middle\n";
		return true;
	}
	return false;*/
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 capsuleUp = worldTransformA.GetOrientation() * Vector3(0, 1, 0);

	Vector3 deltaPos = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 normal = deltaPos.Normalised();
	float howFarAlongCapsule = Vector3::Dot(normal, capsuleUp.Normalised());
	Vector3 pointOnCapsuleLine = worldTransformA.GetPosition() + capsuleUp * howFarAlongCapsule * volumeA.GetHalfHeight();

	SphereVolume tempSphereVol(volumeA.GetRadius());
	Transform tempTransform;
	tempTransform.SetPosition(pointOnCapsuleLine);

	return SphereIntersection(tempSphereVol, tempTransform, volumeB,worldTransformB, collisionInfo);
}

Vector3 CollisionDetection::OBBSupport(const Transform& worldTransform, Vector3 worldDir) {
	Vector3 localDir = worldTransform.GetOrientation().Conjugate() * worldDir;
	Vector3 vertex;
	vertex.x = localDir.x < 0 ? -0.5f : 0.5f;
	vertex.y = localDir.y < 0 ? -0.5f : 0.5f;
	vertex.z = localDir.z < 0 ? -0.5f : 0.5f;
	return (Matrix4::Scale(worldTransform.GetScale()) * worldTransform.GetOrientation() * Matrix4::Translation(vertex)).GetPositionVector();
}

bool CollisionDetection::SAT(const Vector3 delta, Vector3 plane, const Transform& worldTransformA, 
	const Transform& worldTransformB, const Vector3 halfSizeA, const Vector3 halfSizeB, float& penDistance, Vector3& normal, Vector3& pointA,Vector3& pointB) {
	//todo stop calculating these twice
	Vector3 AForward = worldTransformA.GetOrientation() * Vector3(0, 0, 1);
	Vector3 BForward = worldTransformB.GetOrientation() * Vector3(0, 0, 1);
	Vector3 ARight = worldTransformA.GetOrientation() * Vector3(1, 0, 0);
	Vector3 BRight = worldTransformB.GetOrientation() * Vector3(1, 0, 0);
	Vector3 AUp = worldTransformA.GetOrientation() * Vector3(0, 1, 0);
	Vector3 BUp = worldTransformB.GetOrientation() * Vector3(0, 1, 0);

	float deltaPlaneDot = fabs(Vector3::Dot(delta, plane));
	float rest =
		fabs(Vector3::Dot(ARight * halfSizeA.x, plane)) +
		fabs(Vector3::Dot(AUp * halfSizeA.y, plane)) +
		fabs(Vector3::Dot(AForward * halfSizeA.z, plane)) +

		fabs(Vector3::Dot(BRight * halfSizeB.x, plane)) +
		fabs(Vector3::Dot(BUp * halfSizeB.y, plane)) +
		fabs(Vector3::Dot(BForward * halfSizeB.z, plane));

	bool result = deltaPlaneDot <= rest;
	if (result) {
		float delta2 = rest - deltaPlaneDot;
		if (delta2 < penDistance && plane.LengthSquared() > 0) {
			penDistance = delta2;
			

			if (Vector3::Dot(plane, delta) < 0) {
				plane *= -1;
			}
			normal = plane;
			pointA = OBBSupport(worldTransformA,plane);
			pointB = OBBSupport(worldTransformB, -plane);
		}
		return true;
	}
	return false;
}



//bool CollisionDetection::SAT(const Vector3 delta, const Vector3 plane, const Transform& worldTransformA,
//	const Transform& worldTransformB, const Vector3 halfSizeA, const Vector3 halfSizeB, float& penDistance, Vector3& normal, Vector3& point) {
//	
//	Vector3 minA = worldTransformA.GetPosition() - plane * halfSizeA;
//	Vector3 minB = worldTransformB.GetPosition() - plane * halfSizeB;
//	Vector3 maxA = worldTransformA.GetPosition() + plane * halfSizeA;
//	Vector3 maxB = worldTransformB.GetPosition() + plane * halfSizeB;
//
//	float A = Vector3::Dot(plane, minA);
//	float B = Vector3::Dot(plane, maxA);
//	float C = Vector3::Dot(plane, minB);
//	float D = Vector3::Dot(plane, maxB);
//
//	if (A <= C && B >= C) {
//		if (C - B < penDistance) {
//			penDistance = C - B;
//			normal = plane;
//			point = maxA + normal * penDistance;
//		}
//		return true;
//	}
//
//	if (C <= A && D >= A) {
//		if (A - D < penDistance) {
//			penDistance = A - D;
//			normal = -plane;
//			point = minA + normal * penDistance;
//		}
//		return true;
//	}
//	return false;
//}



bool CollisionDetection::OBBIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 deltaPos = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 AForward = worldTransformA.GetOrientation() * Vector3(0, 0, 1);
	Vector3 BForward = worldTransformB.GetOrientation() * Vector3(0, 0, 1);
	Vector3 ARight = worldTransformA.GetOrientation() * Vector3(1, 0, 0);
	Vector3 BRight = worldTransformB.GetOrientation() * Vector3(1, 0, 0);
	Vector3 AUp = worldTransformA.GetOrientation() * Vector3(0, 1, 0);
	Vector3 BUp = worldTransformB.GetOrientation() * Vector3(0, 1, 0);

	bool results[15] = { false };
	float penDistance = FLT_MAX;
	Vector3 normal;
	Vector3 pointA;
	Vector3 pointB;
	Vector3 planes[15] = {
		//faces
		ARight.Normalised(),
		AUp.Normalised(),
		AForward.Normalised(),
		BRight.Normalised(),
		BUp.Normalised(),
		BForward.Normalised(),

		//edges
		Vector3::Cross(ARight, BRight).Normalised(),
		Vector3::Cross(ARight, BUp).Normalised(),
		Vector3::Cross(ARight, BForward).Normalised(),
		Vector3::Cross(AUp, BRight).Normalised(),
		Vector3::Cross(AUp, BUp).Normalised(),
		Vector3::Cross(AUp, BForward).Normalised(),
		Vector3::Cross(AForward, BRight).Normalised(),
		Vector3::Cross(AForward, BUp).Normalised(),
		Vector3::Cross(AForward, BForward).Normalised()
	};
	for (int i = 0; i < 15; i++)
	{
		if (!SAT(deltaPos, planes[i], worldTransformA, worldTransformB, volumeA.GetHalfDimensions(), volumeB.GetHalfDimensions(), penDistance, normal,pointA,pointB))return false;
	}
	//todo calculate contact point properly
	collisionInfo.AddContactPoint(pointA, pointB, normal, penDistance,false);
	//std::cout << "collision\n";
	return true;

}

Matrix4 GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float nearPlane, float farPlane, float fov) {
	float negDepth = nearPlane - farPlane;

	float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

	Matrix4 m;

	float h = 1.0f / tan(fov * PI_OVER_360);

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = invNegDepth;//// +PI_OVER_360;
	m.array[3][2] = -1.0f;
	m.array[3][3] = (0.5f / nearPlane) + (0.5f / farPlane);

	return m;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = screenSize.x / screenSize.y;
	float fov = cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Matrix4 proj = cam.BuildProjectionMatrix(aspect);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov * PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f * (nearPlane * farPlane) / neg_depth;

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = 1.0f / d;

	m.array[3][2] = 1.0f / e;
	m.array[3][3] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

