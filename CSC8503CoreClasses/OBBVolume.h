#pragma once
#include "CollisionVolume.h"

namespace NCL {
	class OBBVolume : CollisionVolume
	{
	public:
		OBBVolume(const Maths::Vector3& halfDims = Maths::Vector3(1,1,1)) {
			type		= VolumeType::OBB;
			halfSizes	= halfDims;
		}
		~OBBVolume() {}

		Maths::Vector3 GetHalfDimensions() const {
			return halfSizes;
		}
	protected:
		Maths::Vector3 halfSizes;
	};
}

