#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>

#include <zcl/thread/thread_pool.hpp>

static bool is_prime(long a)
{
	if (__builtin_expect(a < 2, false))
		return false;

	long sqrta = std::sqrt(a + 0.5);
	for (long i = 2; i <= sqrta; ++i) {
		if (a % i == 0)
			return false;
	}
	return true;
}

static long count_primes(int lo, int hi)
{
	long cnt = 0;
	for (long i = lo; i < hi; ++i) {
		if (is_prime(i))
			++cnt;
	}
	return cnt;
}

int main()
{
	zcl::thread_pool pool(4);

	long n = 200;
	long p = 500000;
	std::vector<std::future<zcl::thread_pool::task_result>> futures;

	// enqueuing tasks
	for (long i = 0; i < n; ++i) {
		auto task_info = pool.enqueue([p, i]() -> long {
			printf("task #%ld: start\n", i);
			long result = count_primes(p * i, p * (i + 1));
			printf("task #%ld: result = %ld\n", i, result);
			return result;
		});
		futures.push_back(task_info[0]->get_future());
	}

	// sum up result
	long cnt = 0;
	for (auto &future : futures) {
		auto [result, err] = future.get();
		if (err) {
			std::cerr << "task failed. what: " << err->what() << std::endl;
			return 1;
		}
		cnt += std::any_cast<long>(result);
	}
	std::cout << cnt << std::endl;

	return 0;
}
