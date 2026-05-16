#pragma once

#include "Parameter/DungeonMeshSetSelectionMethod.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonSelectionPolicy.h"

namespace dungeon
{
namespace selection
{
	inline EDungeonSelectionPolicy ToPolicy(const EDungeonMeshSetSelectionMethod method)
	{
		switch (method)
		{
		case EDungeonMeshSetSelectionMethod::Random:
			return EDungeonSelectionPolicy::Random;
		case EDungeonMeshSetSelectionMethod::Identifier:
			return EDungeonSelectionPolicy::Identifier;
		case EDungeonMeshSetSelectionMethod::DepthFromStart:
			return EDungeonSelectionPolicy::DepthFromStart;
		default:
			return EDungeonSelectionPolicy::Random;
		}
	}

	inline EDungeonSelectionPolicy ToPolicy(const EDungeonPartsSelectionMethod method)
	{
		switch (method)
		{
		case EDungeonPartsSelectionMethod::Random:
			return EDungeonSelectionPolicy::Random;
		case EDungeonPartsSelectionMethod::GridIndex:
			return EDungeonSelectionPolicy::GridIndex;
		case EDungeonPartsSelectionMethod::Direction:
			return EDungeonSelectionPolicy::Direction;
		default:
			return EDungeonSelectionPolicy::Random;
		}
	}

	inline EDungeonMeshSetSelectionMethod ToLegacyMeshSetMethod(const EDungeonSelectionPolicy policy)
	{
		switch (policy)
		{
		case EDungeonSelectionPolicy::Identifier:
			return EDungeonMeshSetSelectionMethod::Identifier;
		case EDungeonSelectionPolicy::DepthFromStart:
			return EDungeonMeshSetSelectionMethod::DepthFromStart;
		case EDungeonSelectionPolicy::Random:
		case EDungeonSelectionPolicy::GridIndex:
		case EDungeonSelectionPolicy::Direction:
		default:
			return EDungeonMeshSetSelectionMethod::Random;
		}
	}

	inline EDungeonPartsSelectionMethod ToLegacyPartsMethod(const EDungeonSelectionPolicy policy)
	{
		switch (policy)
		{
		case EDungeonSelectionPolicy::GridIndex:
			return EDungeonPartsSelectionMethod::GridIndex;
		case EDungeonSelectionPolicy::Direction:
			return EDungeonPartsSelectionMethod::Direction;
		case EDungeonSelectionPolicy::Random:
		case EDungeonSelectionPolicy::Identifier:
		case EDungeonSelectionPolicy::DepthFromStart:
		default:
			return EDungeonPartsSelectionMethod::Random;
		}
	}

	inline bool IsSupportedMeshSetPolicy(const EDungeonSelectionPolicy policy)
	{
		switch (policy)
		{
		case EDungeonSelectionPolicy::Random:
		case EDungeonSelectionPolicy::Identifier:
		case EDungeonSelectionPolicy::DepthFromStart:
			return true;
		default:
			return false;
		}
	}

	inline bool IsSupportedPartsPolicy(const EDungeonSelectionPolicy policy)
	{
		switch (policy)
		{
		case EDungeonSelectionPolicy::Random:
		case EDungeonSelectionPolicy::GridIndex:
		case EDungeonSelectionPolicy::Direction:
		case EDungeonSelectionPolicy::Identifier:
		case EDungeonSelectionPolicy::DepthFromStart:
			return true;
		default:
			return false;
		}
	}

	inline EDungeonSelectionPolicy SanitizeMeshSetPolicy(const EDungeonSelectionPolicy policy)
	{
		return IsSupportedMeshSetPolicy(policy) ? policy : EDungeonSelectionPolicy::Random;
	}

	inline EDungeonSelectionPolicy SanitizePartsPolicy(const EDungeonSelectionPolicy policy)
	{
		return IsSupportedPartsPolicy(policy) ? policy : EDungeonSelectionPolicy::Random;
	}
}
}
