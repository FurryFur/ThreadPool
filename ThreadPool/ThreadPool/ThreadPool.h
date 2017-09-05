#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <type_traits>
#include <future>
#include <utility>
#include <memory>

#include "AtomicQueue.h"

class Thread_Pool
{
public:
	Thread_Pool();
	Thread_Pool(unsigned int size);
	~Thread_Pool();
	
	template<typename Callable, typename... Args>
	std::future<std::result_of_t<Callable(Args...)>> submit(Callable&& workItem, Args&&... args);
	
	void start();
	void stop();

private:
	//The ThreadPool is non-copyable.
    Thread_Pool(const Thread_Pool& _kr) = delete;
    Thread_Pool& operator= (const Thread_Pool& _kr) = delete;

	void do_work(size_t thread_idx);

private:
	//An atomic boolean variable to stop all threads in the threadpool.
	std::atomic_bool m_stop{ false };

	//A WorkQueue of tasks which are functors
	Atomic_Queue<std::function<void()>> m_work_queue;

	//Create a pool of worker threads
	std::vector<std::thread> m_worker_threads; 

	//A variable to hold the number of threads we want in the pool
	unsigned int m_num_threads;
	
};

// Arguments will all stored by copy for safety
template<typename Callable, typename... Args>
inline std::future<std::result_of_t<Callable(Args...)>> Thread_Pool::submit(Callable&& work_item, Args&&... args)
{
	using ResultT = std::result_of_t<Callable(Args...)>; // result_of_t returns the type result of calling Callable with Args
	using TaskT = std::packaged_task<ResultT(Args...)>;

	auto task = std::make_shared<TaskT>(std::forward<Callable>(work_item));

	m_work_queue.push(std::bind([](const std::shared_ptr<TaskT>& task, Args&... args) { // Bind always passes in arguments by lvalue
		(*task)(args...);
	}, task, std::forward<Args>(args)...));

	return task->get_future();
}

#endif