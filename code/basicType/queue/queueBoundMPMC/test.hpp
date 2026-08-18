#include "queueBoundMPMC.hpp"
#include <chrono>
#include <iostream>

namespace destiny::basicType::queue::queueBoundMPMC {
	inline void bench() noexcept;
}

void destiny::basicType::queue::queueBoundMPMC::bench() noexcept
{
	using namespace std::chrono;

	constexpr size_t producerNumber = 1;
	constexpr size_t consumerNumber = 1;
	constexpr size_t len = 1000'000'000;
	destiny::QueueBoundMPMC<void*> queue;

	std::thread thProducer[producerNumber];
	std::thread thConsumer[consumerNumber];

	const auto start = high_resolution_clock::now();

	for (auto& value : thProducer) {
		value = std::thread([&] {
			for (int j = 1; j < len; j++) {
				queue.push(reinterpret_cast<void*>(j));
			}
			return;
		});
	}

	for (auto& value : thConsumer) {
		value = std::thread([&] {
			void* a;
			while (true) {
				queue.pop(a);
				if (a == nullptr)
					break;
			}
			return;
		});
	}

	for (auto& value : thProducer) {
		value.join();
	}

	for (auto& value : thConsumer) {
		queue.push(nullptr);
	}

	for (auto& value : thConsumer) {
		value.join();
	}

	const auto end = high_resolution_clock::now();
	const auto duration = duration_cast<microseconds>(end - start);

	std::cout << "bench: " << "destiny::basicType::queue::queueBoundMPMC\n";
	std::cout << "\tsize:auto(256)\n";
	std::cout << "\tproducerNumber:" << producerNumber << '\n';
	std::cout << "\tconsumerNumber:" << consumerNumber << '\n';
	std::cout << "\tspend:" << (std::uint64_t)len * producerNumber * 1000 * 1000 / (size_t)duration.count() << "msg/s"
	          << std::endl;
}
