#pragma once

#include <stack>
#include <mutex>


namespace TasksLib
{

	/*
		SharedPool based on original implementation by https://stackoverflow.com/users/1731448/swalog
		https://stackoverflow.com/questions/27827923/c-object-pool-that-provides-items-as-smart-pointers-that-are-returned-to-pool/27837534#27837534
	 */
	template <class T>
	class ResourcePool
	{
		struct ResourceDeleter
		{
			explicit ResourceDeleter(std::weak_ptr<ResourcePool<T>* > pool)
				: pool_(pool) {}

			void operator()(T* ptr)
			{
				if (!ptr)
				{
					return;
				}

				std::unique_ptr<T> uptr(ptr);
				if (auto poolPtr = pool_.lock())
				{
					try
					{
						(*poolPtr.get())->Add(std::move(uptr));
						return;
					}
					catch (...) {}			// The above is required to never throw by c++ standard
				}
			}
		private:
			std::weak_ptr<ResourcePool<T>* > pool_;
		};

	public:
		using TPtr = std::unique_ptr<T, ResourceDeleter>;

		ResourcePool() : thisPtr_(new ResourcePool<T>*(this)) {}
		virtual ~ResourcePool() {}

		void Add(std::unique_ptr<T> t)
		{
			std::lock_guard<std::mutex> lock(poolMutex_);

			pool_.push(std::move(t));
		}

		TPtr Acquire()
		{
			std::lock_guard<std::mutex> lock(poolMutex_);

			T* resourcePtr = nullptr;
			if (!pool_.empty())
			{
				resourcePtr = pool_.top().release();
				pool_.pop();
			}

			TPtr tmpUPtr(resourcePtr, ResourceDeleter{ std::weak_ptr<ResourcePool<T>*>{thisPtr_} });
			return std::move(tmpUPtr);
		}

		bool isEmpty() const
		{
			return pool_.empty();
		}

		size_t Size() const
		{
			return pool_.size();
		}

	private:
		std::mutex poolMutex_;
		std::shared_ptr<ResourcePool<T>* > thisPtr_;
		std::stack<std::unique_ptr<T> > pool_;
	};
}
