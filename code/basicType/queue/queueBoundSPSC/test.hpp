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

	constexpr size_t producerNumber = 1;
	constexpr size_t consumerNumber = 1;
	constexpr size_t len = 100'000'000;
	destiny::QueueBoundSPSC<void*> queue;

	std::thread thProducer[producerNumber];
	std::thread thConsumer[consumerNumber];

	const auto start = high_resolution_clock::now();

	for (auto& value : thProducer) {
		value = std::thread([&] {
			for (size_t j = 1; j < len; j++) {
				queue.push(reinterpret_cast<void*>(j));
			}
			return;
		});
	}

	for (auto& value : thConsumer) {
		value = std::thread([&] {
			void* a;
			for (size_t j = 1; j < len; j++) {
				queue.pop(a);
			}
			return;
		});
	}

	for (auto& value : thProducer) {
		value.join();
	}

	for (auto& value : thConsumer) {
		value.join();
	}

	const auto end = high_resolution_clock::now();
	const auto duration = duration_cast<microseconds>(end - start);

	std::cout << "bench: " << "destiny::basicType::queue::queueBoundSPSC\n";
	std::cout << "\tsize:auto(256)\n";
	std::cout << "\tproducerNumber:" << producerNumber << '\n';
	std::cout << "\tconsumerNumber:" << consumerNumber << '\n';
	std::cout << "\tspend:" << (std::uint64_t)len * producerNumber * 1000 * 1000 / (size_t)duration.count() << "msg/s"
	          << std::endl;
}
