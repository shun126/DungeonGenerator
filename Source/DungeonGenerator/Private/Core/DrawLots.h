/*
抽選に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
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
			weights.emplace_back(weight, i);
			totalWeight += weight;
		}

		// TODO: specify random numbers externally.
		std::size_t rnd = random() % totalWeight;

		for (const auto& weight : weights)
		{
			if (rnd < weight.mWeight)
				return weight.mBody;
		
			rnd -= weight.mWeight;
		}

		return last;
	}
}
