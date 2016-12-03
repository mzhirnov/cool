#ifndef COBALT_UTILITY_ENUM_TRAITS_HPP_INCLUDED
#define COBALT_UTILITY_ENUM_TRAITS_HPP_INCLUDED

#pragma once

#include <cstdint>
#include <cstring>

#include <type_traits>
#include <ostream>
#include <sstream>

#include <boost/preprocessor/variadic/size.hpp>
#include <boost/assert.hpp>

namespace cobalt {

template <typename EnumType>
struct enum_traits {
	static constexpr bool is_enum = false;
};
	
struct enum_item_info {
	std::string name() const { return std::string(_name, _name_length); }
	size_t value() const { return _value; }
	
	const char* _name;
	size_t _name_length;
	size_t _value;
};

namespace detail {

template <typename Tag, typename IntType = size_t>
struct auto_enum_value {
	auto_enum_value(IntType v) : _value(v) { counter = _value + 1; }
	auto_enum_value() : _value(counter) { counter = _value + 1; }
	
	operator IntType() const { return _value; }
	
private:
	IntType _value;
	
	static IntType counter;
};

template <typename Tag, typename IntType>
IntType auto_enum_value<Tag, IntType>::counter;

template <int Dummy = 0>
struct helper {
	// Handler(const char* name, size_t length)
	template <typename Handler>
	static void parse_enum_values_helper(const char* str, char separator, Handler handler) {
		auto is_space = [](char c) { return c <= 32; };
		auto is_identifier = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
		
		for (const char* b = str, *e = std::strchr(b, separator); b; (b = e) ? e = std::strchr(++b, separator) : (const char*)nullptr) {
			while (std::isspace(*b))
				++b;
			
			const char* name = b;
			
			while (is_identifier(*b))
				++b;
			
			handler(name, b - name);
		}
	}
	
	static void parse_enum_values(const char* str, enum_item_info* infos, size_t* values) {
		size_t i = 0;
		parse_enum_values_helper(str, ',', [&](const char* name, size_t length) {
			infos[i]._name = name;
			infos[i]._name_length = length;
			infos[i]._value = values[i];
			i++;
		});
		infos[i]._name = nullptr;
		infos[i]._value = 0;
	}
	
	static std::string to_string(const enum_item_info* const info, size_t value) {
		for (auto item = info; item->_name; ++item) {
			if (value == item->_value) {
				return item->name();
			}
		}
		BOOST_ASSERT_MSG(false, "Couldn't find such a value");
		return std::string();
	}
	
	static std::string to_flags_string(const enum_item_info* const info, size_t value) {
		if (!value)
			return to_string(info, value);
		
		std::ostringstream ss;
		
		for (auto item = info; item->_name != nullptr; ++item) {
			if (item->_value && (value & item->_value) == item->_value) {
				value &= ~item->_value;
				if (item != info)
					ss << '|';
				ss.rdbuf()->sputn(item->_name, item->_name_length);
			}
		}
		
		BOOST_ASSERT_MSG(!value, "Unknown value found");
		
		// append unknown values as single hex
		if (value) {
			if (ss.tellp())
				ss << '|';
			ss << std::hex << std::showbase << value;
		}
		
		return ss.str();
	}
	
	static size_t from_string(const enum_item_info* const info, const char* name, size_t length = 0) {
		if (!length)
			length = std::strlen(name);
		
		while (std::isspace(*name)) {
			++name;
			--length;
		}
		
		while (std::isspace(name[length-1]))
			--length;
		
		for (auto item = info; item->_name; ++item) {
			if (item->_name_length == length && !std::strncmp(item->_name, name, length))
				return item->_value;
		}
		
		BOOST_ASSERT_MSG(false, "Couldn't find such a value");
		return 0;
	}
	
	static size_t from_flags_string(const enum_item_info* const info, const char* str, size_t /*length*/ = 0) {
		size_t value = 0;
		parse_enum_values_helper(str, '|', [&](const char* name, size_t length) {
			value |= from_string(info, name, length);
		});
		return value;
	}
};

template <typename T>
struct type_name_helper {
	static std::string name() {
		constexpr size_t length = sizeof(__FUNCTION__) - sizeof("detail::type_name_helper<>::name");
		constexpr size_t offset = sizeof("detail::type_name_helper<") - 1;
		// TODO: Use constexpr substring here
		return std::string(__FUNCTION__ + offset, length);
	}
};

} // namespace detail
	
template <typename T>
std::string type_name() {
	return detail::type_name_helper<T>::name();
}

} // namespace cobalt

#define CO_DEFINE_ENUM_STRUCT(StructName, EnumName, UnderlyingType, ...)                                  \
	struct StructName {                                                                                   \
		enum EnumName : UnderlyingType {                                                                  \
			__VA_ARGS__                                                                                   \
		};                                                                                                \
	};                                                                                                    \
	namespace cobalt {                                                                                    \
		CO_DEFINE_ENUM_STRUCT_TRAITS(StructName, EnumName, __VA_ARGS__)                                   \
	}                                                                                                     \


#define CO_DEFINE_ENUM_FLAGS_STRUCT(StructName, EnumName, UnderlyingType, ...)                            \
	struct StructName {                                                                                   \
		enum EnumName : UnderlyingType {                                                                  \
			__VA_ARGS__                                                                                   \
		};                                                                                                \
	};                                                                                                    \
	namespace cobalt {                                                                                    \
		CO_DEFINE_ENUM_STRUCT_FLAGS_TRAITS(StructName, EnumName, __VA_ARGS__)                             \
	}                                                                                                     \
	CO_DEFINE_ENUM_FLAGS_OPERATORS(StructName::EnumName)                                                  \


#define CO_DEFINE_ENUM_CLASS(EnumName, UnderlyingType, ...)                                               \
	enum class EnumName : UnderlyingType {                                                                \
		__VA_ARGS__                                                                                       \
	};                                                                                                    \
	namespace cobalt {                                                                                    \
		CO_DEFINE_ENUM_CLASS_TRAITS(EnumName, __VA_ARGS__)                                                \
	}                                                                                                     \
	inline std::ostream& operator<<(std::ostream& os, EnumName e) {                                       \
		os << cobalt::enum_traits<EnumName>::to_string(e);                                                \
		return os;                                                                                        \
	}


#define CO_DEFINE_ENUM_FLAGS_CLASS(EnumName, UnderlyingType, ...)                                         \
	enum class EnumName : UnderlyingType {                                                                \
		__VA_ARGS__                                                                                       \
	};                                                                                                    \
	namespace cobalt {                                                                                    \
		CO_DEFINE_ENUM_CLASS_FLAGS_TRAITS(EnumName, __VA_ARGS__)                                          \
	}                                                                                                     \
	inline std::ostream& operator<<(std::ostream& os, EnumName e) {                                       \
		os << cobalt::enum_traits<EnumName>::to_string(e);                                                \
		return os;                                                                                        \
	}                                                                                                     \
	CO_DEFINE_ENUM_FLAGS_OPERATORS(EnumName)                                                              \


#define CO_DEFINE_ENUM_CLASS_TRAITS(EnumName, ...)                                                        \
	template<> struct enum_traits<EnumName> {                                                             \
		static constexpr bool is_enum = true;                                                             \
		static constexpr bool is_flags = false;                                                           \
		enum { num_items = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__) };                                         \
		static const enum_item_info* const items() {                                                      \
			static enum_item_info __infos[num_items + 1];                                                 \
			for (static bool __init = false; !__init && (__init = true); ) {                              \
				detail::auto_enum_value<EnumName> __VA_ARGS__;                                            \
				size_t __values[] = { __VA_ARGS__ };                                                      \
				detail::helper<>::parse_enum_values(#__VA_ARGS__, __infos, __values);                     \
			}                                                                                             \
			return __infos;                                                                               \
		}                                                                                                 \
		static std::string to_string(EnumName value)                                                      \
			{ return detail::helper<>::to_string(items(), static_cast<size_t>(value)); }                  \
		static EnumName from_string(const char* str, size_t length = 0)                                   \
			{ return static_cast<EnumName>(detail::helper<>::from_string(items(), str, length)); }        \
	};                                                                                                    \

#define CO_DEFINE_ENUM_STRUCT_TRAITS(StructName, EnumName, ...)                                           \
	template<> struct enum_traits<StructName> {                                                           \
		static constexpr bool is_enum = true;                                                             \
		static constexpr bool is_flags = false;                                                           \
		enum { num_items = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__) };                                         \
		static const enum_item_info* const items() {                                                      \
			static enum_item_info __infos[num_items + 1];                                                 \
			for (static bool __init = false; !__init && (__init = true); ) {                              \
				detail::auto_enum_value<StructName> __VA_ARGS__;                                          \
				size_t __values[] = { __VA_ARGS__ };                                                      \
				detail::helper<>::parse_enum_values(#__VA_ARGS__, __infos, __values);                     \
			}                                                                                             \
			return __infos;                                                                               \
		}                                                                                                 \
		static std::string to_string(StructName::EnumName value)                                          \
			{ return detail::helper<>::to_string(items(), static_cast<size_t>(value)); }                  \
		static StructName::EnumName from_string(const char* str, size_t length = 0)                       \
			{ return static_cast<StructName::EnumName>(detail::helper<>::from_string(items(), str, length)); } \
	};                                                                                                    \

#define CO_DEFINE_ENUM_CLASS_FLAGS_TRAITS(EnumName, ...)                                                  \
	template<> struct enum_traits<EnumName> {                                                             \
		static constexpr bool is_enum = true;                                                             \
		static constexpr bool is_flags = true;                                                            \
		enum { num_items = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__) };                                         \
		static const enum_item_info* const items() {                                                      \
			static enum_item_info __infos[num_items + 1];                                                 \
			for (static bool __init = false; !__init && (__init = true); ) {                              \
				detail::auto_enum_value<EnumName> __VA_ARGS__;                                            \
				size_t __values[] = { __VA_ARGS__ };                                                      \
				detail::helper<>::parse_enum_values(#__VA_ARGS__, __infos, __values);                     \
			}                                                                                             \
			return __infos;                                                                               \
		}                                                                                                 \
		static std::string to_string(EnumName value)                                                      \
			{ return detail::helper<>::to_flags_string(items(), static_cast<size_t>(value)); }            \
		static EnumName from_string(const char* str, size_t length = 0)                                   \
			{ return static_cast<EnumName>(detail::helper<>::from_flags_string(items(), str, length)); }  \
	};                                                                                                    \

#define CO_DEFINE_ENUM_STRUCT_FLAGS_TRAITS(StructName, EnumName, ...)                                     \
	template<> struct enum_traits<StructName> {                                                           \
		static constexpr bool is_enum = true;                                                             \
		static constexpr bool is_flags = true;                                                            \
		enum { num_items = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__) };                                         \
		static const enum_item_info* const items() {                                                      \
			static enum_item_info __infos[num_items + 1];                                                 \
			for (static bool __init = false; !__init && (__init = true); ) {                              \
				detail::auto_enum_value<StructName> __VA_ARGS__;                                          \
				size_t __values[] = { __VA_ARGS__ };                                                      \
				detail::helper<>::parse_enum_values(#__VA_ARGS__, __infos, __values);                     \
			}                                                                                             \
			return __infos;                                                                               \
		}                                                                                                 \
		static std::string to_string(StructName::EnumName value)                                        \
			{ return detail::helper<>::to_flags_string(items(), static_cast<size_t>(value)); }            \
		static StructName::EnumName from_string(const char* str, size_t length = 0)                     \
			{ return static_cast<StructName::EnumName>(detail::helper<>::from_flags_string(items(), str, length)); } \
	};                                                                                                    \

#define CO_ENUM_TO_VAL(EnumName, value) static_cast<std::underlying_type_t<EnumName>>(value)
#define CO_ENUM_TO_REF(EnumName, value) reinterpret_cast<std::underlying_type_t<EnumName>&>(value)

#define CO_DEFINE_ENUM_FLAGS_OPERATORS(EnumName)                                                                \
	constexpr EnumName operator~(EnumName elem) noexcept                                                        \
		{ return static_cast<EnumName>(~CO_ENUM_TO_VAL(EnumName, elem)); }                                      \
	constexpr EnumName operator|(EnumName lhs, EnumName rhs) noexcept                                           \
		{ return static_cast<EnumName>(CO_ENUM_TO_VAL(EnumName, lhs) | CO_ENUM_TO_VAL(EnumName, rhs)); }        \
	constexpr EnumName operator&(EnumName lhs, EnumName rhs) noexcept                                           \
		{ return static_cast<EnumName>(CO_ENUM_TO_VAL(EnumName, lhs) & CO_ENUM_TO_VAL(EnumName, rhs)); }        \
	constexpr EnumName operator^(EnumName lhs, EnumName rhs) noexcept                                           \
		{ return static_cast<EnumName>(CO_ENUM_TO_VAL(EnumName, lhs) ^ CO_ENUM_TO_VAL(EnumName, rhs)); }        \
	inline EnumName& operator|=(EnumName& lhs, EnumName rhs) noexcept                                           \
		{ return reinterpret_cast<EnumName&>(CO_ENUM_TO_REF(EnumName, lhs) |= CO_ENUM_TO_VAL(EnumName, rhs)); } \
	inline EnumName& operator&=(EnumName& lhs, EnumName rhs) noexcept                                           \
		{ return reinterpret_cast<EnumName&>(CO_ENUM_TO_REF(EnumName, lhs) &= CO_ENUM_TO_VAL(EnumName, rhs)); } \
	inline EnumName& operator^=(EnumName& lhs, EnumName rhs) noexcept                                           \
		{ return reinterpret_cast<EnumName&>(CO_ENUM_TO_REF(EnumName, lhs) ^= CO_ENUM_TO_VAL(EnumName, rhs)); } \

#endif // COBALT_UTILITY_ENUM_TRAITS_HPP_INCLUDED