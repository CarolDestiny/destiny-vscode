#include "slowWrite.hpp"
#include "share.hpp"

void destiny::detail::slowWrite(const Log& log) noexcept
{
	const std::uint64_t code = log.errorCode()->getTypeCode();
	const std::uint64_t protectLen = log.errorCode()->getProtectLen();
	if (protectLen == -1) [[unlikely]] {
		Log log;
		destiny::TypeCode<DESTINY_TYPECODE_ERROR_PUBLIC_UNKNOWN, 2> errorCode;
		errorCode.buffer()[0] = code;
		errorCode.buffer()[1] = protectLen;
		log.errorCode() = &errorCode;
		slowWrite(log);
	}
}
