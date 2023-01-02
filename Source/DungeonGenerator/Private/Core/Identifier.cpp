/*!
識別子クラスソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Identifier.h"

namespace dungeon
{
	Identifier::IdentifierType Identifier::mCounter = 0;

	Identifier::Identifier(const Type type) noexcept
		: mIdentifier(mCounter)
	{
		const IdentifierType value = static_cast<IdentifierType>(type) << shift;
		mIdentifier |= value;

		mCounter = (mCounter + 1) & maskCounter;
	}
}
