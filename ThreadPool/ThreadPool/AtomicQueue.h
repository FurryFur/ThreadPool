//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : A thread safe queue
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <queue>
#include <mutex>

template<typename T>
class AtomicQueue
{
public:
	AtomicQueue() {}

	// Insert an item at the back of the queue.
	void push(const T&& item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workQueue.push(std::forward<const T>(item));
		m_cvNotEmpty.notify_one(); 
	}

	// Attempts to get a workitem from the queue
	// If the queue is empty just return false; 
	bool tryPop(T& workItem)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		//If the queue is empty return false
		if(m_workQueue.empty())
		{
			return false;
		}
		workItem = std::move(m_workQueue.front());
		m_workQueue.pop();
		return true;
	}

	// Attempts to get a workitem from the queue
	// If the queue is empty then block and wait for items.
	void pop(T& workItem)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		//If the queue is empty block the thread from running until a work item becomes available
		m_cvNotEmpty.wait(lock, [this]{return !m_workQueue.empty();});
		workItem = std::move(m_workQueue.front());
		m_workQueue.pop();
	}

	// Clears the queue
	void clear() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workQueue.swap(std::queue<T>()); // Swap with empty queue to clear
	}

	// Checks if the queue is empty or not
	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_workQueue.empty();
	}

	// Returns the number of items in the queue
	size_t size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_workQueue.size();
	}

private:
	std::queue<T> m_workQueue;
	mutable std::mutex m_mutex;
	std::condition_variable m_cvNotEmpty;
	
};

#endif
