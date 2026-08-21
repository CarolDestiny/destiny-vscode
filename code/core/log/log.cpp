#include "log.hpp"

destiny::TypeCodeBase*& destiny::Log::errorCode() noexcept
{
	return errorCode_;
}

destiny::TypeCodeBase* destiny::Log::errorCode() const noexcept
{
	return errorCode_;
}
