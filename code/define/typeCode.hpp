#pragma once
#include <cstdint>
#include <string>

namespace destiny {
	class TypeCodeBase;
	template <std::uint64_t TYPECODE, std::uint64_t PROTECTLEN = UINT64_MAX> class TypeCode;
} // namespace destiny

class destiny::TypeCodeBase {
public:
	virtual ~TypeCodeBase() = default;
	virtual std::uint64_t getTypeCode() const noexcept = 0;
	virtual std::uint64_t getProtectLen() const noexcept = 0;
};

/* destiny AI type code: 0000 */
inline constexpr std::uint64_t DESTINY_TYPECODE_DESTINY_END = 0x0000'0000'0000'0000ULL; // [TypeCode:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_DESTINY_END> : public destiny::TypeCodeBase {
public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_DESTINY_END; }
	inline std::uint64_t getProtectLen() const noexcept override { return -1; }
};

inline constexpr std::uint64_t DESTINY_TYPECODE_DESTINY_START = 0x0000'0000'0000'0001ULL; // [TypeCode:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_DESTINY_START> : public destiny::TypeCodeBase {
public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_DESTINY_START; }
	inline std::uint64_t getProtectLen() const noexcept override { return -1; }
};

/* destiny file type code: 0001 */
/** file:destiny-core-log: 0001'0000 **/
inline constexpr std::uint64_t DESTINY_TYPECODE_FILE_LOG_MAIN = 0x0001'0000'0000'0000ULL; // [TypeCode:uint64][protectLen:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_FILE_LOG_MAIN> : public destiny::TypeCodeBase {
private:
	uint64_t protectLen_;

public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_FILE_LOG_MAIN; }
	inline std::uint64_t getProtectLen() const noexcept override { return protectLen_; }
	inline std::uint64_t& protectLen() noexcept { return protectLen_; }
};

inline constexpr std::uint64_t DESTINY_TYPECODE_FILE_LOG_INDEX = 0x0001'0000'0000'0001ULL; // [TypeCode:uint64][protectLen:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_FILE_LOG_INDEX> : public destiny::TypeCodeBase {
private:
	uint64_t protectLen_;

public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_FILE_LOG_INDEX; }
	inline std::uint64_t getProtectLen() const noexcept override { return protectLen_; }
	inline std::uint64_t& protectLen() noexcept { return protectLen_; }
};

inline constexpr std::uint64_t DESTINY_TYPECODE_FILE_LOG_SLOW = 0x0001'0000'0000'0002ULL; // [TypeCode:uint64][protectLen:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_FILE_LOG_SLOW> : public destiny::TypeCodeBase {
private:
	uint64_t protectLen_;

public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_FILE_LOG_SLOW; }
	inline std::uint64_t getProtectLen() const noexcept override { return protectLen_; }
	inline std::uint64_t& protectLen() noexcept { return protectLen_; }
};

/* destiny error type code: 0002 */
/** destiny public error type code: 0002'0000 **/
inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_PUBLIC_UNKNOWN = 0x0002'0000'0000'0000ULL; // [TypeCode:uint64][protectLen:uint64]
template <std::uint64_t PROTECTLEN> class destiny::TypeCode<DESTINY_TYPECODE_ERROR_PUBLIC_UNKNOWN, PROTECTLEN> : public destiny::TypeCodeBase {
private:
	std::uint64_t protectLen_;
	std::uint64_t buffer_[PROTECTLEN / 8 + bool(PROTECTLEN % 8)];

public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_ERROR_PUBLIC_UNKNOWN; }
	inline std::uint64_t getProtectLen() const noexcept override { return PROTECTLEN; }
	inline char* buffer() const noexcept { return &buffer_; }
};

/** error:destiny-core-log: 0002'0001 **/
inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_LOG_END = 0x0002'0001'0000'0000ULL; // [TypeCode:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_ERROR_LOG_END> : public destiny::TypeCodeBase {
public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_ERROR_LOG_END; }
	inline std::uint64_t getProtectLen() const noexcept override { return -1; }
};

inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_LOG_MAIN = 0x0002'0001'0000'0001ULL; // [TypeCode:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_ERROR_LOG_MAIN> : public destiny::TypeCodeBase {
public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_ERROR_LOG_MAIN; }
	inline std::uint64_t getProtectLen() const noexcept override { return -1; }
};
/** error:destiny-core-memory: 0002'0002 **/

/** error:destiny-public **/
inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_MALLOC = 0x0002'0002'0000'0001ULL; // [TypeCode:uint64]
template <> class destiny::TypeCode<DESTINY_TYPECODE_ERROR_MALLOC> : public destiny::TypeCodeBase {
public:
	inline std::uint64_t getTypeCode() const noexcept override { return DESTINY_TYPECODE_ERROR_MALLOC; }
	inline std::uint64_t getProtectLen() const noexcept override { return -1; }
};
// [TypeCode:uint64][protectLen:uint64][detailType:uint64]-
// detailType = 0: -[lostPointer:void*][lostLen:uint64]; if(lostPointer == nullptr) -> unknown lost pointer; if(lostLen == 0) -> unknown lost len
// detailType = 1: -[lostThreadID:uint64][?]
inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_MLACK = 0x0002'0002'0000'0002ULL;
// [TypeCode:uint64][protectLen:uint64][detailType:uint64]-
// ?
inline constexpr std::uint64_t DESTINY_TYPECODE_ERROR_MFREE = 0x0002'0002'0000'0003ULL;
