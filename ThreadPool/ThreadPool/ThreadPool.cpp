//Library Includes
#include <iostream>
#include <thread>
#include <functional>

//Local Includes
#include "AtomicQueue.h"

//This Include
#include "ThreadPool.h"

ThreadPool::ThreadPool() :
	m_numThreads{ std::thread::hardware_concurrency() }
{ 

}


ThreadPool::ThreadPool(size_t size) :
	m_numThreads{ size }
{

}

ThreadPool::~ThreadPool()
{
	m_stop = true;
	for(size_t i=0; i < m_numThreads; ++i)
	{
		m_workQueue.push([]() {}); // Dummy task to wake threads up
	}
	for (size_t i = 0; i < m_numThreads; ++i)
	{
		m_workerThreads[i].join();
	}
}

void ThreadPool::start()
{
	for (size_t i = 0; i < m_numThreads; ++i)
	{
		m_workerThreads.push_back(std::thread(&ThreadPool::doWork, this, i));
	}
}

void ThreadPool::stop()
{
	m_stop = true;
}

size_t ThreadPool::getNumThreads() const
{
	return m_numThreads;
}

void ThreadPool::doWork(size_t thread_idx)
{
	//Entry point of  a thread.
	//std::cout << std::endl << "Thread with id " << thread_idx << "starting........" << std::endl;
	while(!m_stop)
	{
		std::function<void()> task;
		//If there is an item in the queue to be processed; just take it off the q and process it
		m_workQueue.pop(task);
		//std::cout << std::endl << "Thread with id " << thread_idx << " is working on an item in the work queue" << std::endl;
		task();
		//std::cout << std::endl << "Thread with id " << thread_idx << " finished processing an item " << std::endl;
		//std::cout << "Items remaining: " << m_work_queue.size() << std::endl;
		//Sleep to simulate work being done
		//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%101));
	}
}

