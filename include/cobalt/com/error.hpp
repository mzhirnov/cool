#ifndef COBALT_COM_ERROR_HPP_INCLUDED
#define COBALT_COM_ERROR_HPP_INCLUDED

#pragma once

#include <system_error>
#include <cstdio>

namespace cobalt {
namespace com {

enum class errc {
	success = 0,
	failure,
	no_such_interface,
	no_such_class,
	aggregation_not_supported,
	class_disabled,
	not_implemented
};

namespace detail {

class com_error_category : public std::error_category {
public:
	virtual const char* name() const noexcept override { return "com"; }
	
	virtual std::string message(int ev) const override {
		switch (static_cast<errc>(ev)) {
		case errc::success:
			return "operation succeeded";
		case errc::failure:
			return "operation failed";
		case errc::no_such_interface:
			return "no such interface";
		case errc::no_such_class:
			return "no such class";
		case errc::aggregation_not_supported:
			return "aggregation not supported";
		case errc::class_disabled:
			return "class disabled";
		case errc::not_implemented:
			return "not implemented";
		}
		
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "unknown error: 0x%X", ev);
		return buffer;
	}
};

struct com_error_category_helper {
	inline static com_error_category instance;
};

} // namespace detail

inline const std::error_category& com_category() noexcept {
	return detail::com_error_category_helper::instance;
}

inline std::error_code make_error_code(errc e) noexcept {
	return std::error_code{static_cast<int>(e), com_category()};
}

inline std::error_condition make_error_condition(errc e) noexcept {
	return std::error_condition{static_cast<int>(e), com_category()};
}

} // namespace com
} // namespace cobalt

namespace std {
	template <>
	struct is_error_condition_enum<::cobalt::com::errc> : true_type {};
}

#endif // COBALT_COM_ERROR_HPP_INCLUDED
