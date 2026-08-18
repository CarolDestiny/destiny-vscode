#pragma once
#include <cstddef>
#include <type_traits>
#include <atomic>
#include <bit>
#include <thread>

namespace destiny {
	template <typename T, size_t SIZE = 256>
	    requires std::is_pointer_v<T>
	class QueueBoundMPMC;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
class destiny::QueueBoundMPMC {
public:
	inline QueueBoundMPMC() noexcept;
	inline ~QueueBoundMPMC() noexcept = default;

	inline bool empty() noexcept;
	inline size_t size() noexcept;

	inline void push(T value) noexcept;
	inline void pop(T& value) noexcept;

private:
	static_assert(SIZE > 0, "SIZE must be greater than zero");

	static constexpr size_t CAPACITY = std::bit_ceil(SIZE);
	static constexpr size_t MASK = CAPACITY - 1;
	struct Slot;

	struct alignas(64) Slot {
		std::atomic<size_t> sequence;
		T data;
	};

	alignas(64) Slot buffer_[CAPACITY];
	alignas(64) std::atomic<size_t> producerIndex_{0};
	alignas(64) std::atomic<size_t> consumerIndex_{0};
	alignas(64) std::atomic<size_t> producerWait_{0}; // the number of waiting producers
	alignas(64) std::atomic<size_t> consumerWait_{0}; // the number of waiting consumers
};

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
destiny::QueueBoundMPMC<T, SIZE>::QueueBoundMPMC() noexcept
{
	for (size_t i = 0; i < CAPACITY; ++i) {
		buffer_[i].sequence = i;
	}
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
bool destiny::QueueBoundMPMC<T, SIZE>::empty() noexcept
{
	const size_t producerIndex = producerIndex_.load(std::memory_order_relaxed);
	const size_t consumerIndex = consumerIndex_.load(std::memory_order_relaxed);
	return producerIndex == consumerIndex;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
size_t destiny::QueueBoundMPMC<T, SIZE>::size() noexcept
{
	const size_t producerIndex = producerIndex_.load(std::memory_order_relaxed);
	const size_t consumerIndex = consumerIndex_.load(std::memory_order_relaxed);
	const size_t count = producerIndex - consumerIndex;
	return count <= CAPACITY ? count : CAPACITY;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundMPMC<T, SIZE>::push(T value) noexcept
{
	const size_t index = producerIndex_.fetch_add(1, std::memory_order_relaxed);
	Slot& slot = buffer_[index & MASK];
	size_t waitTime = 0;
	while (true) {
		const size_t seq = slot.sequence.load((std::memory_order_relaxed));
		if (index == seq) {
			break;
		}
		// wait
		if (waitTime > 2048) {
			producerWait_.fetch_add(1, std::memory_order_release);
			slot.sequence.wait(seq, std::memory_order_relaxed);
			producerWait_.fetch_sub(1, std::memory_order_relaxed);
		} else if (waitTime > 1024) {
			std::this_thread::yield();
		}
		++waitTime;
	}
	std::atomic_thread_fence(std::memory_order_acquire);
	slot.data = value;
	slot.sequence.store(index + 1, std::memory_order_release);
	if (consumerWait_.load(std::memory_order_acquire) != 0) {
		slot.sequence.notify_all();
	}
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundMPMC<T, SIZE>::pop(T& value) noexcept
{
	const size_t index = consumerIndex_.fetch_add(1, std::memory_order_relaxed);
	Slot& slot = buffer_[index & MASK];
	size_t waitTime = 0;
	while (true) {
		const size_t seq = slot.sequence.load(std::memory_order_relaxed);
		if (seq == index + 1) {
			break;
		}
		// wait
		if (waitTime > 2048) {
			consumerWait_.fetch_add(1, std::memory_order_release);
			slot.sequence.wait(seq, std::memory_order_relaxed);
			consumerWait_.fetch_sub(1, std::memory_order_relaxed);
		} else if (waitTime > 1024) {
			std::this_thread::yield();
		}
		++waitTime;
	}
	std::atomic_thread_fence(std::memory_order_acquire);
	value = slot.data;
	slot.sequence.store(index + CAPACITY, std::memory_order_release);
	if (producerWait_.load(std::memory_order_acquire) != 0) {
		slot.sequence.notify_all();
	}
	return;
}
