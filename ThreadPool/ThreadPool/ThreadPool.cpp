//Library Includes
#include <iostream>
#include <thread>
#include <functional>

//Local Includes
#include "AtomicQueue.h"

//This Include
#include "ThreadPool.h"

Thread_Pool::Thread_Pool() :
	m_num_threads{ std::thread::hardware_concurrency() }
{ 

}


Thread_Pool::Thread_Pool(unsigned int size) :
	m_num_threads{ size }
{

}

Thread_Pool::~Thread_Pool()
{
	m_stop = true;
	for(unsigned int i=0; i < m_num_threads; ++i)
	{
		m_work_queue.push([]() {}); // Dummy task to wake threads up
	}
	for (unsigned int i = 0; i < m_num_threads; ++i)
	{
		m_worker_threads[i].join();
	}
}

void Thread_Pool::start()
{
	for (unsigned int i = 0; i < m_num_threads; i++)
	{
		m_worker_threads.push_back(std::thread(&Thread_Pool::do_work, this, i));
	}
}

void Thread_Pool::stop()
{
	m_stop = true;
}


void Thread_Pool::do_work(size_t thread_idx)
{
	//Entry point of  a thread.
	//std::cout << std::endl << "Thread with id " << thread_idx << "starting........" << std::endl;
	while(!m_stop)
	{
		std::function<void()> task;
		//If there is an item in the queue to be processed; just take it off the q and process it
		m_work_queue.pop(task);
		//std::cout << std::endl << "Thread with id " << thread_idx << " is working on an item in the work queue" << std::endl;
		task();
		//std::cout << std::endl << "Thread with id " << thread_idx << " finished processing an item " << std::endl;
		//std::cout << "Items remaining: " << m_work_queue.size() << std::endl;
		//Sleep to simulate work being done
		//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%101));
	}
}

