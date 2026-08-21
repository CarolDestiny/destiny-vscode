#include <iostream>
#include <iomanip>
#include "core/log/log.hpp"
#include "core/log/slowWrite.hpp"
int main()
{
	destiny::core::log::onload();
	destiny::Log log;
	destiny::TypeCode<DESTINY_TYPECODE_DESTINY_START> errorCode;
	log.errorCode() = &errorCode;
	destiny::detail::slowWrite(log);
}
