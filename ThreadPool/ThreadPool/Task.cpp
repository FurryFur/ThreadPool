#include <iostream>
#include <thread>

#include "Task.h"

ITask::ITask()
	:m_ivalue(0)
{

}

ITask::ITask(int _value)
	: m_ivalue(_value)
{

}

ITask::~ITask()
{

}

void ITask::operator()() const
{
	//Sleep to simulate work being done
	std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 101));
}

int ITask::getValue() const
{
	return m_ivalue;
}