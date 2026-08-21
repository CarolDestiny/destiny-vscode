#pragma once
#include "define/typeCode.hpp"
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
	destiny::TypeCodeBase*& errorCode() noexcept;
	destiny::TypeCodeBase* errorCode() const noexcept;

private:
	destiny::TypeCodeBase* errorCode_;
	uint32_t id_;
	std::string context_;
};
