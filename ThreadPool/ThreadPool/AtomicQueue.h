/*********A very basic Work Queue class***************/

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <queue>
#include <mutex>


template<typename T>
class Atomic_Queue
{
public:
	Atomic_Queue() {}

	//Insert an item at the back of the queue and signal any thread that might be waiting for the q to be populated
	void push(const T&& item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		work_queue.push(std::forward<const T>(item));
		m_cvNotEmpty.notify_one(); 
	}

	//Attempt to get a workitem from the queue
	//If the Q is empty just return false; 
	bool tryPop(T& workItem)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		//If the queue is empty return false
		if(work_queue.empty())
		{
			return false;
		}
		workItem = std::move(work_queue.front());
		work_queue.pop();
		return true;
	}

	//Attempt to get a workitem from the queue
	//If the Q is empty just return false; 
	void pop(T& workItem)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		//If the queue is empty block the thread from running until a work item becomes available
		m_cvNotEmpty.wait(lock, [this]{return !work_queue.empty();});
		workItem = std::move(work_queue.front());
		work_queue.pop();
	}

	//Checking if the queue is empty or not
	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return work_queue.empty();
	}

private:
	std::queue<T> work_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_cvNotEmpty;
	
};

#endif
