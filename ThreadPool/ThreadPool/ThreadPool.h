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

class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(unsigned int _size);
	~ThreadPool();
	
	template<typename Callable, typename... Args>
	std::future<std::result_of_t<Callable(Args...)>> Submit(Callable&& workItem, Args&&... args);

	void DoWork();
	void Start();
	void Stop();
	std::atomic_int& getItemsProcessed();

private:
	//The ThreadPool is non-copyable.
    ThreadPool(const ThreadPool& _kr) = delete;
    ThreadPool& operator= (const ThreadPool& _kr) = delete;

private:
	//An atomic boolean variable to stop all threads in the threadpool.
	std::atomic_bool m_stop{ false };

	//A WorkQueue of tasks which are functors
	CAtomicQueue<std::function<void()>> m_workQueue;

	//Create a pool of worker threads
	std::vector<std::thread> m_workerThreads; 

	//A variable to hold the number of threads we want in the pool
	unsigned int m_numThreads;

	//An atomic variable to keep track of how many items have been processed.
	std::atomic_int m_itemsProcessed{ 0 };
	
};

template<typename Callable, typename... Args>
inline std::future<std::result_of_t<Callable(Args...)>> ThreadPool::Submit(Callable&& workItem, Args&&... args)
{
	using ResultT = std::result_of_t<Callable(Args...)>;
	using TaskT = std::packaged_task<ResultT(Args...)>;

	auto pTask = std::make_shared<TaskT>(std::forward<Callable>(workItem));

	m_workQueue.push(std::bind([](const std::shared_ptr<TaskT>& rpTask, Args&... args) { // Bind always passes in arguments by lvalue
		(*rpTask)(args...);
	}, pTask, std::forward<Args>(args)...));

	return pTask->get_future();
}

#endif