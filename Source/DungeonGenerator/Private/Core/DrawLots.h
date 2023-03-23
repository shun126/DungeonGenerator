/*
抽選に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include <random>
#include <vector>

namespace dungeon
{
	/*
	重み付き抽選
	*/
	template <class InputIterator, class Predicate>
	InputIterator DrawLots(InputIterator first, InputIterator last, Predicate pred) noexcept
	{
		std::random_device random;
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

		// TODO: specify random numbers externally.
		const std::size_t rnd = random() % totalWeight;

		for (const auto& weight : weights)
		{
			// cppcheck-suppress [useStlAlgorithm]
			if (rnd < weight.mWeight)
				return weight.mBody;
		}

		return last;
	}
}
