#pragma once
#include "queueBoundSPSC.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>

namespace destiny::basicType::queue::queueBoundSPSC {
	inline void bench() noexcept;
}

void destiny::basicType::queue::queueBoundSPSC::bench() noexcept
{
	using namespace std::chrono;

	constexpr size_t len = 100'000'000;
	destiny::QueueBoundSPSC<void*> queue;

	std::thread thProducer;
	std::thread thConsumer;

	const auto start = high_resolution_clock::now();

	thProducer = std::thread([&] {
		for (size_t i = 1; i < len; ++i)
			queue.push((void*)(i));
	});

	thConsumer = std::thread([&] {
		void* a = nullptr;
		while (true) {
			queue.pop(a);
			if ((size_t)a == len - 1)
				break;
		}
	});

	thProducer.join();
	thConsumer.join();

	const auto end = high_resolution_clock::now();
	const auto duration = duration_cast<microseconds>(end - start);

	std::cout << "bench: " << "destiny::basicType::queue::queueBoundSPSC\n";
	std::cout << "\tsize:auto(256)\n";
	std::cout << "\tspend:" << (std::uint64_t)len * 1000 * 1000 / (size_t)duration.count() << "msg/s" << std::endl;
}
