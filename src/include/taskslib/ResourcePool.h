#pragma once

#include <stack>
#include <mutex>
#include <memory>

#include "Types.h"

namespace TasksLib {

	/*
		SharedPool based on original implementation by https://stackoverflow.com/users/1731448/swalog
		https://stackoverflow.com/questions/27827923/c-object-pool-that-provides-items-as-smart-pointers-that-are-returned-to-pool/27837534#27837534
	 */

	template <class T> struct ResourceDeleter {
		ResourceDeleter() = delete;
		explicit ResourceDeleter(std::weak_ptr<ResourcePool<T>*> pool);
		ResourceDeleter(const ResourceDeleter<T>& rhs) = delete;
		ResourceDeleter(const ResourceDeleter<T>&& rhs) noexcept ;

		void operator()(T* ptr);

	private:
		std::weak_ptr<ResourcePool<T>*> pool_;
	};

    template <class T> ResourceDeleter<T>::ResourceDeleter(std::weak_ptr<ResourcePool<T>*> pool)
            : pool_(pool) {}
    template <class T> ResourceDeleter<T>::ResourceDeleter(const ResourceDeleter<T>&& rhs) noexcept
            : pool_(std::move(rhs.pool_)) {}
    template <class T> void ResourceDeleter<T>::operator()(T* ptr) {
        if (!ptr) {
            return;
        }

        std::unique_ptr<T> uPtr(ptr);
        if (auto poolPtr = pool_.lock()) {
            try {
                (*poolPtr.get())->Add(std::move(uPtr));
                return;
            }
            catch (...) {}			// The above is required to never throw by c++ standard
        }
    }

    // ==========================================================================

    template <class T> class ResourcePool {
	public:
		ResourcePool();
		virtual ~ResourcePool();

		virtual void Add(std::unique_ptr<T> elem);

        [[maybe_unused]] std::unique_ptr<T, ResourceDeleter<T>> Acquire();
        [[maybe_unused]] std::unique_ptr<T, ResourceDeleter<T>> AddAcquire(std::unique_ptr<T> elem);

        [[maybe_unused]] [[nodiscard]] bool IsEmpty() const;
        [[maybe_unused]] [[nodiscard]] size_t Size() const;

	private:
		std::mutex poolMutex_;
		std::shared_ptr<ResourcePool<T>*> thisPtr_;				// This is a shared_ptr to a ptr so that it wouldn't
                                                                // try to release *this when destructing the object
		std::stack<std::unique_ptr<T>> pool_;
	};

	template <class T> ResourcePool<T>::ResourcePool() : thisPtr_(new ResourcePool<T>*{ this }) {}
	template <class T> ResourcePool<T>::~ResourcePool() = default;

	template <class T>
	void ResourcePool<T>::Add(std::unique_ptr<T> elem) {
		std::lock_guard<std::mutex> lock(poolMutex_);

		pool_.push(std::move(elem));
	}
	template <class T>
    [[maybe_unused]] std::unique_ptr<T, ResourceDeleter<T>> ResourcePool<T>::Acquire() {
		std::lock_guard<std::mutex> lock(poolMutex_);

		T* resourcePtr = nullptr;
		if (!pool_.empty()) {
			resourcePtr = pool_.top().release();
			pool_.pop();
		}

		std::unique_ptr<T, ResourceDeleter<T>> tmpUPtr{ resourcePtr, ResourceDeleter<T>{ std::weak_ptr<ResourcePool<T>*>{ thisPtr_ } } };
		return std::move(tmpUPtr);
	}
	template <class T>
    [[maybe_unused]] std::unique_ptr<T, ResourceDeleter<T>> ResourcePool<T>::AddAcquire(std::unique_ptr<T> elem) {
		T* resourcePtr = elem.release();
		std::unique_ptr<T, ResourceDeleter<T>> tmpUPtr{ resourcePtr, ResourceDeleter<T>{ std::weak_ptr<ResourcePool<T>*>{ thisPtr_ } } };
		return std::move(tmpUPtr);
	}

	template <class T> [[maybe_unused]] bool ResourcePool<T>::IsEmpty() const {
		return pool_.empty();
	}
	template <class T> [[maybe_unused]] size_t ResourcePool<T>::Size() const {
		return pool_.size();
	}

    // ==========================================================================

    /*
     * Class Singleton
     *
     * A trivial implementation of a glorified global std::shared_ptr.
     * It's more of a caching mechanic than anything else, convenient to limit
     * the number of instances of a global to 1 (yeah, I hate globals too, but when
     * it is the Window, or the Renderer, or the Config manager it is useful).
     *
     * Use the static getInstance() to retrieve a pointer to the underlying instance, and
     * create it if needed.
     */
    template <class T> class [[maybe_unused]] Singleton {
    public:
        [[maybe_unused]] static std::weak_ptr<T> _instance;

    public:
        template <typename... Args>
        [[maybe_unused]] static std::shared_ptr<T> getInstance(Args ...args);
    };

    // We have to explicitly instantiate the static data member
    template <class T> std::weak_ptr<T> Singleton<T>::_instance;

    template <class T>
    template <typename... Args>
    [[maybe_unused]] std::shared_ptr<T> Singleton<T>::getInstance(Args ...args) {
        std::shared_ptr<T> ptr = _instance.lock();

        if (ptr.use_count() == 0) {
            ptr = std::make_shared<T>(args...);
            _instance = ptr;
        }

        return std::move(ptr);
    }

}
