#pragma once
#include <cstdint>
#include <string>

namespace destiny::core::log {
	bool onload() noexcept;
	void unload() noexcept;
} // namespace destiny::core::log

namespace destiny {
	class Log;
}

class destiny::Log {
public:
private:
	// error Code
	uint32_t id_;
	std::string context_;
};
