#pragma once

#include <mutex>

template<typename T>
class LockQueue
{
public:
	LockQueue() {}

	LockQueue(const LockQueue& other) = delete;
	LockQueue& operator=(const LockQueue& other) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_queue.push(move(value));
		_condVar.notify_one();
	}

	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_queue.empty())
		{
			return false;
		}
		value = move(_queue.front());
		_queue.pop();
		return true;
	}

	void WaitAndPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] { return !_queue.empty(); });
		value = move(_queue.front());
		_queue.pop();
	}

private:
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condVar;
};