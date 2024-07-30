/*
抽選に関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "../Math/Random.h"
#include <memory>
#include <vector>

namespace dungeon
{
	/*
	重み付き抽選
	*/
	template <class InputIterator, class Predicate>
	InputIterator DrawLots(const std::shared_ptr<Random>& random, InputIterator first, InputIterator last, Predicate pred) noexcept
	{
		std::size_t totalWeight = 0;

		struct Entry final
		{
			Entry(const std::size_t weight, const InputIterator& body)
				: mWeight(weight)
				, mBody(body)
			{}
			std::size_t mWeight;
			InputIterator mBody;
		};
		std::vector<Entry> weights;
		weights.reserve(std::distance(first, last));
		for (InputIterator i = first; i != last; ++i)
		{
			auto weight = pred(*i);
			if (weight < 1)
				weight = 1;
			const auto currentWeight = totalWeight + weight;
			weights.emplace_back(currentWeight, i);
			totalWeight = currentWeight;
		}

		const std::size_t rnd = random->Get<size_t>(totalWeight);
		const auto i = std::find_if(weights.begin(), weights.end(), [rnd](const Entry& weight)
			{
				return rnd < weight.mWeight;
			}
		);
		return i != weights.end() ? i->mBody : last;
	}
}
