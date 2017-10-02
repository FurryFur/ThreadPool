//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : A thread pool
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "AtomicQueue.h"
#include "Utils.h"

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

class ThreadPool
{
public:
	ThreadPool();
	// numThreads specifies the number of threads that 
	// should be used by this thread pool.
	ThreadPool(size_t numThreads);
	~ThreadPool();

	// The ThreadPool is non-copyable.
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator= (const ThreadPool&) = delete;
	
	// Submits a function the thread pool for execution.
	// Arguments to the callable can be supplied after the callable.
	// To pass a value by reference use std::ref otherwise values will be copied.
	template<typename Callable, typename... Args>
	std::future<std::result_of_t<Callable(Args...)>> submit(Callable&& workItem, Args&&... args);
	
	// Start executing work items submitted to the threadpool
	void start();

	// Stop the thread pool.
	// The thread pool is reset to its initial state after this call
	// and it is safe to call start again.
	void stop();

	// Empty all work items from the work queue.
	void clearWork();

	// Sets the number of threads to run inside the thread pool
	// WARNING: Using this after the threadpool has started will
	// result in undefined behavior, and likely crash.
	void setNumThreads(size_t numThreads);

	// Gets the number of threads running or that will run in the
	// thread pool.
	size_t getNumThreads() const;

private:
	// The main function that threads are executing in.
	// Handles removing work items from the queue and executing them.
	void doWork(size_t threadId);

	// An atomic boolean variable to stop all threads in the threadpool.
	std::atomic_bool m_stop{ false };

	//A WorkQueue of tasks which are functors
	AtomicQueue<std::function<void()>> m_workQueue;

	//Create a pool of worker threads
	std::vector<std::thread> m_workerThreads; 
protected:
	//A variable to hold the number of threads we want in the pool
	size_t m_numThreads;

	static thread_local size_t tl_threadId;
};

template <typename 	ThreadLocalStorageT>
class ThreadPoolWithStorage : public ThreadPool {
public:
	ThreadPoolWithStorage();
	ThreadPoolWithStorage(size_t size);

	// Returns the storage object associated with the specified thread
	// ID. Thread ID 0 is reserved for the main thread, so there are
	// m_numThreads + 1 thread local storage objects.
	ThreadLocalStorageT& getThreadLocalStorage(size_t threadId);

	// Returns the storage object for the current thread pool thread
	// Note: This should only be call from inside a function that is 
	// executing on the thread pool.
	ThreadLocalStorageT& getThreadLocalStorage();

private:
	std::vector<ThreadLocalStorageT> m_threadStores;
};

template<typename ThreadLocalStorageT>
inline ThreadPoolWithStorage<ThreadLocalStorageT>::ThreadPoolWithStorage()
	: ThreadPool()
	, m_threadStores{ m_numThreads + 1 } // Extra store for main thread
{
}

template<typename ThreadLocalStorageT>
inline ThreadPoolWithStorage<ThreadLocalStorageT>::ThreadPoolWithStorage(size_t size)
	: ThreadPool(size)
	, m_threadStores{ m_numThreads + 1 } // Extra store for main thread
{
}

template<typename ThreadLocalStorageT>
inline ThreadLocalStorageT & ThreadPoolWithStorage<ThreadLocalStorageT>::getThreadLocalStorage(size_t threadId)
{
	return m_threadStores.at(threadId);
}

template<typename ThreadLocalStorageT>
inline ThreadLocalStorageT& ThreadPoolWithStorage<ThreadLocalStorageT>::getThreadLocalStorage()
{
	return m_threadStores.at(tl_threadId);
}

// Arguments will all be stored by copy for safety
template<typename Callable, typename... Args>
inline std::future<std::result_of_t<Callable(Args...)>> ThreadPool::submit(Callable&& workItem, Args&&... args)
{
	using ResultT = std::result_of_t<Callable(Args...)>; // result_of_t returns the result type of calling Callable with Args
	using TaskT = std::packaged_task<ResultT(Args...)>;

	auto task = std::make_shared<TaskT>(std::forward<Callable>(workItem));
	std::future<ResultT> future = task->get_future();

	m_workQueue.push(std::bind([](const std::shared_ptr<TaskT>& task, InvokeTypeT<Args>... args) { // Bind always passes in arguments by lvalue
		(*task)(args...);
	}, std::move(task), std::forward<Args>(args)...));

	return future;
}

#endif
