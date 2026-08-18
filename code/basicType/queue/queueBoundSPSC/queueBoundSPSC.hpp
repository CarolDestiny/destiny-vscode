#pragma once
#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <thread>
#include <type_traits>

namespace destiny {
	template <typename T, size_t SIZE = 256>
	    requires std::is_pointer_v<T>
	class QueueBoundSPSC;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
class destiny::QueueBoundSPSC {
public:
	inline QueueBoundSPSC() noexcept;
	inline ~QueueBoundSPSC() noexcept = default;

	inline bool empty() noexcept;
	inline size_t size() noexcept;

	inline void push(T value) noexcept;
	inline void pop(T& value) noexcept;

private:
	static_assert(SIZE > 0, "SIZE must be greater than zero");

	static constexpr size_t CAPACITY = std::bit_ceil(SIZE);
	static constexpr size_t MASK = CAPACITY - 1;

	alignas(64) std::atomic<T> buffer_[CAPACITY];
	alignas(64) size_t producerIndex_{0};
	alignas(64) size_t consumerIndex_{0};
	alignas(64) std::atomic<size_t> producerGate_{0};
	alignas(64) std::atomic<size_t> consumerGate_{0};
};

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
destiny::QueueBoundSPSC<T, SIZE>::QueueBoundSPSC() noexcept
{
	for (auto& slot : buffer_) {
		slot.store(nullptr, std::memory_order_relaxed);
	}
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
bool destiny::QueueBoundSPSC<T, SIZE>::empty() noexcept
{
	for (const auto& slot : buffer_) {
		if (slot.load(std::memory_order_relaxed) != nullptr) {
			return false;
		}
	}
	return true;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
size_t destiny::QueueBoundSPSC<T, SIZE>::size() noexcept
{
	size_t count = 0;
	for (const auto& slot : buffer_) {
		count += slot.load(std::memory_order_relaxed) != nullptr;
	}
	return count;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundSPSC<T, SIZE>::push(T value) noexcept
{
	assert(value != nullptr);

	const size_t index = producerIndex_;
	std::atomic<T>& slot = buffer_[index & MASK];
	size_t waitTime = 0;
	while (true) {
		if (slot.load(std::memory_order_relaxed) == nullptr) {
			break;
		}

		if (waitTime > 2048) {
			const size_t waitState = producerGate_.fetch_or(1, std::memory_order_acq_rel) | 1;
			const T observed = slot.load(std::memory_order_relaxed);
			if (observed != nullptr) {
				producerGate_.wait(waitState, std::memory_order_acquire);
			}
			producerGate_.fetch_and(~size_t(1), std::memory_order_relaxed);
		} else if (waitTime > 1024) {
			std::this_thread::yield();
		}
		++waitTime;
	}
	slot.store(value, std::memory_order_release);
	producerIndex_ = index + 1;

	const size_t consumerGate = consumerGate_.fetch_add(2, std::memory_order_release);
	if ((consumerGate & 1) != 0) {
		consumerGate_.notify_one();
	}
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundSPSC<T, SIZE>::pop(T& value) noexcept
{
	const size_t index = consumerIndex_;
	std::atomic<T>& slot = buffer_[index & MASK];
	size_t waitTime = 0;
	T observed = nullptr;

	while (true) {
		observed = slot.load(std::memory_order_acquire);
		if (observed != nullptr) {
			break;
		}

		if (waitTime > 2048) {
			const size_t waitState = consumerGate_.fetch_or(1, std::memory_order_acq_rel) | 1;
			if (slot.load(std::memory_order_acquire) == nullptr) {
				consumerGate_.wait(waitState, std::memory_order_acquire);
			}
			consumerGate_.fetch_and(~size_t(1), std::memory_order_relaxed);
		} else if (waitTime > 1024) {
			std::this_thread::yield();
		}
		++waitTime;
	}
	value = observed;

	slot.store(nullptr, std::memory_order_relaxed);
	consumerIndex_ = index + 1;

	const size_t producerGate = producerGate_.fetch_add(2, std::memory_order_release);
	if ((producerGate & 1) != 0) {
		producerGate_.notify_one();
	}
	return;
}
