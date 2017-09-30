#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "AtomicQueue.h"

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <type_traits>
#include <future>
#include <utility>
#include <memory>
#include <string>

template <typename ThreadLocalStorageT>
class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(size_t size);
	~ThreadPool();
	
	template<typename Callable, typename... Args>
	std::future<std::result_of_t<Callable(Args...)>> submit(Callable&& workItem, Args&&... args);
	
	void start();
	void stop();

	size_t getNumThreads() const;

private:
	//The ThreadPool is non-copyable.
    ThreadPool(const ThreadPool& _kr) = delete;
    ThreadPool& operator= (const ThreadPool& _kr) = delete;

	void doWork(size_t threadId);

	//An atomic boolean variable to stop all threads in the threadpool.
	std::atomic_bool m_stop{ false };

	//A WorkQueue of tasks which are functors
	AtomicQueue<std::function<void()>> m_workQueue;

	//Create a pool of worker threads
	std::vector<std::thread> m_workerThreads; 

	//A variable to hold the number of threads we want in the pool
	size_t m_numThreads;
	static thread_local size_t tl_threadId;
};

template <typename 	ThreadLocalStorageT>
class ThreadPoolWithStorage : ThreadPool {
public:
	ThreadLocalStorageT& getThreadLocalStorage();
};

template<typename ThreadLocalStorageT>
inline ThreadLocalStorageT& ThreadPoolWithStorage<ThreadLocalStorageT>::getThreadLocalStorage()
{
	// TODO: insert return statement here
	ThreadPool();
	ThreadPool(size_t size);
	~ThreadPool();
}

// Arguments will all stored by copy for safety
template<typename Callable, typename... Args>
inline std::future<std::result_of_t<Callable(Args...)>> ThreadPool::submit(Callable&& work_item, Args&&... args)
{
	using ResultT = std::result_of_t<Callable(Args...)>; // result_of_t returns the type result of calling Callable with Args
	using TaskT = std::packaged_task<ResultT(Args...)>;

	auto task = std::make_shared<TaskT>(std::forward<Callable>(work_item));

	m_workQueue.push(std::bind([](const std::shared_ptr<TaskT>& task, Args&... args) { // Bind always passes in arguments by lvalue
		(*task)(args...);
	}, task, std::forward<Args>(args)...));

	return task->get_future();
}

#endif
