#pragma once

#include <mutex>

template<typename T>
class LockStack
{
public:
	LockStack() {}

	LockStack(const LockStack& other) = delete;
	LockStack& operator=(const LockStack& other) = delete;

	~LockStack() {}

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_stack.push(move(value));
		_condVar.notify_one();
	}

	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_stack.empty())
		{
			return false;
		}
		value = move(_stack.top());
		_stack.pop();
		return true;
	}

	void WaitAndPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] { return !_stack.empty(); });
		value = move(_stack.top());
		_stack.pop();
	}

private:
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condVar;
};

template<typename T>
class LockFreeStack
{
	struct Node
	{
		Node(T& data) : data(data), next(nullptr) {
		}

		T data;
		Node* next;
	};
public:
	void Push(T& data)
	{
		Node* newNode = new Node(data);
		newNode->next = _head.load();
		while (!_head.compare_exchange_weak(newNode->next, newNode));
	}

	bool TryPop(T& data)
	{
		++_popCount;
		Node* oldHead = _head.load();
		while (oldHead && !_head.compare_exchange_weak(oldHead, oldHead->next));
		if (oldHead)
		{
			data = oldHead->data;
			TryDelete(oldHead);
			
			return true;
		}

		--_popCount;
		return false;
	}

	void TryDelete(Node* oldNode)
	{
		if (_popCount == 1)
		{

			Node* node = _pendingList.exchange(nullptr);

			if (--_popCount == 0)
			{
				DeleteNode(node);
			}
			else
			{
				ChainPendingList(node);
			}

			delete oldNode;
		}
		else if(oldNode)
		{
			ChainPendingList(oldNode);
			--_popCount;
		}
	}

	void ChainPendingList(Node* node)
	{
		Node* oldNode = _pendingList.load();
		while (!_pendingList.compare_exchange_weak(oldNode, node));
	}

	void ChainPendingList(Node* first, Node* last)
	{
		last->next = _pendingList.load();
		while (!_pendingList.compare_exchange_weak(last->next, first));
	}

	void ChainPendingNode(Node* node)
	{
		Node* oldNode = _pendingList.load();
		while (!_pendingList.compare_exchange_weak(oldNode, node));
	}

	static void DeleteNode(Node* node)
	{
		while (node)
		{
			Node* next = node->next;
			delete node;
			node = next;
		}
	}

private: 
	atomic<Node*> _head;
	atomic<uint32> _popCount = 0;
	atomic<Node*> _pendingList = nullptr;
};