#pragma once
#include <fstream>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <bit>

namespace destiny::detail {
	inline std::fstream fileLog;
	inline std::fstream fileIndex;
	inline std::fstream fileSlow;
	inline std::mutex mtxSlow;
} // namespace destiny::detail
