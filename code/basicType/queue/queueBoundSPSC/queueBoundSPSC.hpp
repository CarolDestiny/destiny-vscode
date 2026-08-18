#pragma once
#include <bit>
#include <cstddef>
#include <type_traits>

namespace destiny {
	template <typename T, size_t SIZE>
	    requires std::is_pointer_v<T>
	class QueueBoundSPSC;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
class destiny::QueueBoundSPSC {
public:
	inline QueueBoundSPSC() noexcept;
	inline ~QueueBoundSPSC() noexcept;

	inline bool empty() noexcept;
	inline size_t size() noexcept;

	inline void push(T value) noexcept;
	inline void pop(T& value) noexcept;

private:
	static constexpr size_t CAPACITY = std::bit_ceil(SIZE);
};

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
destiny::QueueBoundSPSC<T, SIZE>::QueueBoundSPSC() noexcept
{
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
destiny::QueueBoundSPSC<T, SIZE>::~QueueBoundSPSC() noexcept
{
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
bool destiny::QueueBoundSPSC<T, SIZE>::empty() noexcept
{
	return true;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
size_t destiny::QueueBoundSPSC<T, SIZE>::size() noexcept
{
	return size_t();
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundSPSC<T, SIZE>::push(T value) noexcept
{
	return;
}

template <typename T, size_t SIZE>
    requires std::is_pointer_v<T>
void destiny::QueueBoundSPSC<T, SIZE>::pop(T& value) noexcept
{
	return;
}
