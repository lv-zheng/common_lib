#pragma once

#include <any>
#include <condition_variable>
#include <exception>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace zcl {

class thread_pool {
public:
	enum class task_state {
		WAITING = 0,
		RUNNING = 1,
		FINISHED = 2,
	};

	using task_result = std::tuple<std::any, std::optional<std::exception>>;

	class task_info {
		friend class thread_pool;
	public:
		std::future<task_result> get_future();
		task_state state();

		task_info(std::future<task_result>&& future_) :_future(std::move(future_))
		{ }

	private:
		std::atomic<task_state> _state = task_state::WAITING;
		std::future<task_result> _future;
	};

	using function = std::function<task_result()>;

	thread_pool(std::size_t workers);
	~thread_pool();

	template<class Func>
	std::vector<std::shared_ptr<task_info>> enqueue(Func f, std::size_t count = 1);

	std::size_t workers() const;
	void resize(std::size_t target_workers);

	void stop();
	void join();

private:
	struct workload {
		function func;
		std::shared_ptr<task_info> info;
		std::promise<task_result> promise;
	};

	std::size_t _target_workers;

	std::unordered_map<std::thread::id, std::thread> _worker_threads;

	std::queue<workload> _task_queue;
	mutable std::mutex _task_mutex;
	std::condition_variable _task_cv;

	std::atomic<bool> _stop = false;
	std::once_flag _join_once;

	void _worker_func();
};

inline std::future<thread_pool::task_result> thread_pool::task_info::get_future()
{
	return std::move(_future);
}

inline thread_pool::task_state thread_pool::task_info::state()
{
	return _state.load();
}

inline thread_pool::thread_pool(std::size_t workers)
{
	resize(workers);
}

inline thread_pool::~thread_pool()
{
	join();
}

template<class Func>
std::vector<std::shared_ptr<thread_pool::task_info>>
thread_pool::enqueue(Func f, std::size_t count)
{
	function func = [f]() -> task_result {
		try {
			auto r = f();
			return {std::any(r), std::nullopt};
		} catch (std::exception& e) {
			return {std::any(), e};
		}
	};

	std::vector<std::shared_ptr<task_info>> result;

	{
		std::lock_guard guard(_task_mutex);

		for (std::size_t i = 0; i < count; ++i) {
			std::promise<task_result> promise;
			auto info = std::make_shared<task_info>(promise.get_future());
			result.push_back(info);
			_task_queue.push({func, info, std::move(promise)});
		}
	}
	_task_cv.notify_all();

	return result;
}

std::size_t thread_pool::workers() const
{
	std::lock_guard guard(_task_mutex);
	return _task_queue.size();
}

void thread_pool::resize(std::size_t target_workers)
{
	_target_workers = target_workers;
	while (_worker_threads.size() < target_workers) {
		std::thread thread(_worker_func, this);
		auto id = thread.get_id();
		_worker_threads[id] = std::move(thread);
	}
}

void thread_pool::stop()
{
	_stop.store(true);
	_task_cv.notify_all();
}

void thread_pool::join()
{
	stop();
	std::call_once(_join_once, [this]() {
		for (auto& [_, thread] : _worker_threads) {
			(void)_;
			thread.join();
		}
	});
}

void thread_pool::_worker_func()
{
	while (true) {
		workload load;

		{
			std::unique_lock lock(_task_mutex);
			_task_cv.wait(lock, [this](){ return !_task_queue.empty() || _stop.load(); });
			if (_task_queue.empty() && _stop.load())
				break;
			load = std::move(_task_queue.front());
			_task_queue.pop();
			load.info->_state.store(task_state::RUNNING);
		}
		
		auto result = load.func();
		load.info->_state.store(task_state::FINISHED);
		load.promise.set_value(result);
	}
}

}
