#include "pch.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "CoreMacro.h"
#include "ThreadManager.h"

CoreGlobal Core;

void ThreadMain()
{
	while (true)
	{
		cout << "Hello I am thraed.. " << LThreadId << endl;
		this_thread::sleep_for(chrono::seconds(1));
	}
}

int main()
{
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch(ThreadMain);
	}

	GThreadManager->Join();
}

