// A functor class

#ifndef __CTASK_H__
#define __CTASK_H__

#include <future>
#include <utility>

class ITask
{
public:
	ITask();
	ITask(int value);
	virtual ~ITask();
	virtual void operator()() const;
	int getValue() const;
private:
	int m_ivalue;
};

template <typename CallableT>
class CTask : public ITask
{
public:
	CTask(CallableT&& task) : 
		m_task{ std::forward<CallableT>(task) },
		ITask()
	{

	}

	CTask(CallableT&& task, int value) :
		m_task{ std::forward<CallableT>(task) },
		ITask(value)
	{

	}

	virtual void operator()() const override
	{
		m_task();
	}

private:
	CallableT m_task;
};

#endif
