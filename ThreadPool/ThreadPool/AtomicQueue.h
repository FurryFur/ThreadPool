/*********A very basic Work Queue class***************/

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <queue>
#include <mutex>


template<typename T>
class AtomicQueue
{
public:
	AtomicQueue() {}

	//Insert an item at the back of the queue and signal any thread that might be waiting for the q to be populated
	void push(const T&& item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workQueue.push(std::forward<const T>(item));
		m_cvNotEmpty.notify_one(); 
	}

	//Attempt to get a workitem from the queue
	//If the Q is empty just return false; 
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

	//Attempt to get a workitem from the queue
	//If the Q is empty just return false; 
	void pop(T& workItem)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		//If the queue is empty block the thread from running until a work item becomes available
		m_cvNotEmpty.wait(lock, [this]{return !m_workQueue.empty();});
		workItem = std::move(m_workQueue.front());
		m_workQueue.pop();
	}

	// Clear the queue
	void clear() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workQueue.swap(std::queue<T>()); // Swap with empty queue to clear
	}

	//Checking if the queue is empty or not
	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_workQueue.empty();
	}

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
