#include <iostream>
//#include <vld.h>

#include "ThreadPool.h"
#include "WorkQueue.h"
#include "Task.h"

int main()
{
	srand((unsigned int)time(0));
	const int kiTOTALITEMS = 20;
	//Create a ThreadPool Object capable of holding as many threads as the number of cores
	ThreadPool& threadPool = ThreadPool::GetInstance();
	//Initialize the pool
	threadPool.Initialize();
	threadPool.Start();
	// The main thread writes items to the WorkQueue
	for(int i =0; i< kiTOTALITEMS; i++)
	{
		threadPool.Submit(CTask(i));
		std::cout << "Main Thread wrote item " << i << " to the Work Queue " << std::endl;
		//Sleep for some random time to simulate delay in arrival of work items
		//std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1001));
	}
	if (threadPool.getItemsProcessed() == kiTOTALITEMS)
	{
		threadPool.Stop();
	}

	int iTemp;
	std::cin >> iTemp;

	return 0;
}