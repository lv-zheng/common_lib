#include "ztl/split.hpp"

#include <iostream>
#include <numeric>
#include <string>
#include <vector>


static void test_string_split()
{
	std::string s = "abc|def,ghi";
	std::string delims = ",|";

	auto splitter = ztl::splitter_in(s.begin(), s.end(), delims);

	std::vector<std::string> result;
	splitter.yield_all(ztl::back_emplacer(result));

	for (auto& field : result)
		std::cout << field << std::endl;
}

static void test_generic_split()
{
	std::vector<double> numbers = {3.14, 1.59, -2.65, 3.58, 9.79, -3.23, 8.46};
	auto is_negative = [](auto v) {
		return v < 0;
	};
	auto splitter = ztl::splitter_if(numbers.begin(), numbers.end(), is_negative);
	splitter.yield_all([](auto first, auto last) {
		std::cout << std::accumulate(first, last, 0.) << std::endl;
	});
}

int main()
{
	test_string_split();
	test_generic_split();

	return 0;
}
