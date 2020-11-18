#pragma once

#include <iostream>
#include <string>
#include <string.h>
#include <fstream> 
#include <vector>
#include <unordered_map>
#include <map>
#include <assert.h>

extern "C" double strtod(const char *s00, char **se);
extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);

using namespace std;

#define Json(name)\
class name :public json_base<name>

//static metadata collector
#define N(name)																														\
name;																																\
private:																															\
	bool unserialize_##name( json_stream &js) {																						\
		return parser::unserialize(&name,js);																						\
	}																																\
	void serialize_##name(string &res) {																							\
		parser::serialize(&name,res);																								\
	}																																\
	class class__##name:private static_instance<class__##name>{																		\
	public:																															\
		class__##name() {																											\
			data_impl_t fie;																										\
			unserialize_fun_t us= &child_t::unserialize_##name;																		\
			memcpy(fie.unserialize, &us, sizeof(us));																				\
			serialize_fun_t s = &child_t::serialize_##name;																			\
			memcpy(fie.serialize, &s, sizeof(s));																					\
			field_collector<child_t>::fields(#name,&fie);																			\
		}																															\
	};																																\
public:

#define STRNG_TO_NUM(data_t,methodl,methodf)				\
char* endptr;												\
*data = (data_t)methodl(js.begin, &endptr, 10);				\
if (*endptr == '.' || *endptr == 'e' || *endptr == 'E')		\
	*data = (data_t)methodf(js.begin, &endptr);				\
if (endptr == js.begin) {									\
	check_skip(js);											\
	return false;											\
}															\
(js.begin) = endptr;										\
return true;												\

#define R(...) #__VA_ARGS__

#define ERROR_RETURT(opt) cout << "[error]:" << opt.begin;assert(!js.is_option(ASSERT));cout<<endl;return false;

//! singleton template
template<class T>
class static_instance {
public:
	static_instance() {
		ins;
	}
private:
	static T ins;
};
template<class T>
T static_instance<T>::ins;

//! This is a special string as the unordered map key
//This is a special string as the unordered map key, which is no copy, just a pointer to the string beginning. 
//If use std::string as the key, which will new char[] on the heap, copy some chars and release, basically no need
//when initializing the fields the key come from const char* and end with '\0'
//but when parsing the json string, we all know that the keys end with '"',
//so by this way we can do someshit on the json string directly
class no_copy_string {
public:
	const char* str;
	no_copy_string(const char* s) {
		str = s;
	}
};

namespace std {
	//! SGI STL way to generate hashcode
	template<>
	struct hash<no_copy_string> {
	public:
		size_t operator()(const no_copy_string &p) const {
			unsigned long h = 0;
			const char *s = p.str;

			for (; *s; ++s) {
				if (*s == '"')
					break;
				else if (*s == '\\') {
					h = 5 * h + *s;
					if (*(++s))
						h = 5 * h + *s;
					else
						break;
				}
				else
					h = 5 * h + *s;
			}

			return size_t(h);
		}

	};

	//! check xxx\0 == xxx", the map key end with \0, the json key str end with "
	template<>
	struct equal_to<no_copy_string> {
	public:
		bool operator()(const no_copy_string &p1, const no_copy_string &p2) const
		{
			for (int i = 0;; i++) {
				if ((!p1.str[i] || p1.str[i] == '"') || (!p2.str[i] || p2.str[i] == '"')) {
					break;
				}
				if (p1.str[i] != p2.str[i])
					return false;
				else {
					if (p1.str[i] == '\\') {
						i++;
						if (p1.str[i] != p2.str[i])
							return false;
					}
				}

			}
			return true;
		}

	};
}

//! on Windows the class member function is 8bit
// on Linux it is 16bit
// count_func_pointer_len to count the size
struct data_impl_t {
	void count_func_pointer_len() {}
	char unserialize[sizeof(&data_impl_t::count_func_pointer_len)];
	char serialize[sizeof(&data_impl_t::count_func_pointer_len)];
};

//! Field collector
template<class T>
class field_collector {
public:
	static const std::unordered_map<no_copy_string, data_impl_t>& fields(const char* name = nullptr, const data_impl_t *field = nullptr) {
		static std::unordered_map<no_copy_string, data_impl_t> fields;
		if (field && fields.find(no_copy_string(name)) == fields.end()) {
			fields[no_copy_string(name)] = *field;
		}
		return fields;
	}
};

#define ASSERT (char)0x1
#define UNESCAPE (char)0x2
#define UNESCAPE_UNICODE (char)0x4

//! json stream with option
struct json_stream {
	const char* begin;
	const char* end;
	char option;
	void set_option(char op) {
		option = op;
	}
	bool is_option(char op) {
		if (option&op)
			return true;
		return false;
	}
};

//! json parser
class parser {
public:
	enum json_key_symbol
	{
		object_begin = '{',
		object_end = '}',

		array_begin = '[',
		array_end = ']',

		key_value_separator = ':',
		next_key_value = ',',
		str = '"',
		space = ' '
	};
public:
	static bool is_null(json_stream &js) {
		char n[5] = "null";
		char index = 0;
		while (char ch = *js.begin) {
			if (index >= 4)
				break;
			else if (index < 4 && ch != n[index]) {
				return false;
			}
			index++;
			get_next(js);
		}
		if (index != 4)
			return false;
		return true;
	}
	static bool parse_bool(bool &val, json_stream &js) {
		char t[5] = "true";
		char f[6] = "false";
		char index = 0;
		if (*js.begin == 't') {
			while (char ch = *js.begin) {
				if (index >= 4)
					break;
				else if (index < 4 && ch != t[index]) {
					val = false;
					return false;
				}
				index++;
				get_next(js);
			}
			if (index != 4)
				return false;
			val = true;
			return true;
		}
		else if (*js.begin == 'f') {
			while (char ch = *js.begin) {
				if (index >= 5)
					break;
				else if (index < 5 && ch != f[index]) {
					val = false;
					return false;
				}
				index++;
				ch = get_next(js);
			}
			if (index != 5)
				return false;
			val = false;
			return true;
		}
		else {
			char* endptr;
			val = strtol(js.begin, &endptr, 10);
			if (*endptr == '.')
				val = strtof(js.begin, &endptr);
			if (endptr == js.begin)
				return false;
			else
				return true;
		}
		val = false;
		return false;
	}

	//escape the controll char like \t \r \f etc and whitespace
	static bool inline is_ctr_or_space_char(char ch) {
		return (ch == ' ' || (ch >= 0x00 && ch <= 0x1F) || ch == 0x7F);
	}

	//this is not a strict api to check double,-1 -> double, 1 -> int, 0 -> error
	static char inline is_double(json_stream &js) {
		json_stream t = js;
		if (*js.begin == '0') {
			if (char ch = get_next(t)) {
				if (ch == '.') {
					ch = get_next(t);
					if (ch >= '0'&&ch <= '9')
						return -1;
					else
						return 0;
				}
				else if (ch == ',' || is_ctr_or_space_char(ch) || ch == ']' || ch == '}')
					return 1;
				else
					return 0;
			}
		}

		while (char ch = get_cur_and_next(t)) {
			if (ch == '.') {
				ch = get_cur_and_next(t);
				if (ch >= '0'&&ch <= '9')
					return -1;
				else
					return 0;
			}
			else if( ch == 'e' || ch == 'E'){
				return -1;
			}
			else if (ch == ',' || is_ctr_or_space_char(ch) || ch == ']' || ch == '}')
				return 1;
			else if (ch == '+')
				return 0;
		}
		return 1;
	}

	static void check_skip(json_stream &js) {
		char ch = get_cur_and_next(js);
		if (ch == json_key_symbol::object_begin) {
			skip_object(js);
		}
		else if (ch == json_key_symbol::array_begin) {
			skip_array(js);
		}
		else if (ch == json_key_symbol::str) {
			skip_str(js);
		}
	}

	static void skip_object(json_stream &js) {
		while (char ch = get_cur_and_next(js)) {
			if (ch == json_key_symbol::object_begin)
				skip_object(js);
			else if (ch == json_key_symbol::object_end)
				return;
		}
	}

	static void skip_array(json_stream &js) {
		while (char ch = get_cur_and_next(js)) {
			if (ch == json_key_symbol::array_begin)
				skip_array(js);
			else if (ch == json_key_symbol::array_end)
				return;
		}
	}

	static void skip_value(json_stream &js) {
		while (char ch = *js.begin) {
			if (ch == json_key_symbol::next_key_value || ch == json_key_symbol::object_end || ch == json_key_symbol::array_end) {
				return;
			}
			get_next(js);
		}
	}

	static size_t get_array_size(json_stream &js) {
		int size = 0;
		while (char ch = get_cur_and_next(js)) {
			if (ch == json_key_symbol::array_begin) {
				skip_array(js);
			}
			else if (ch == json_key_symbol::object_begin) {
				skip_object(js);
			}
			else if (ch == json_key_symbol::next_key_value) {
				size++;
			}
			else if (ch == json_key_symbol::array_end)
				return ++size;
		}
		return 0;
	}

	static size_t skip_str(json_stream &js) {
		size_t i = 0;
		while (char ch = get_cur_and_next(js)) {
			if (ch == json_key_symbol::str) {
				return i;
			}
			else if (ch == '\\') {
				get_cur_and_next(js);
				i++;
			}
			i++;
		}
		return i;
	}

	inline static void skip_space(json_stream &js) {
		while (char ch = *js.begin) {
			if (!is_ctr_or_space_char(ch))
				return;
			get_next(js);
		}
	}

	static char inline get_next(json_stream &js) {
		(js.begin)++;
		if (js.end && js.end < js.begin)
			return '\0';
		else
			return *js.begin;
	}
public:
	static char inline get_cur_and_next(json_stream &js) {
		if (js.end && js.end < (js.begin + 1))
			return '\0';
		else
			return *((js.begin)++);
	}

	inline static void serialize(bool *data, string &res) {
		if (*data)
			res += "true";
		else
			res += "false";
	}
	inline static void serialize(int64_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(int32_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(int16_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(int8_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(uint64_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(uint32_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(uint16_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(uint8_t *data, string &res) {
		res += to_string(*data);
	}
	inline static void serialize(double *data, string &res) {
#ifdef IEEE_8087
		int si = 0;
		int de = 0;

		const char* re = dtoa(*data, 0, 0, &de, &si, 0);
		if (si)
			res += '-';
		if (de > 0) {
			size_t rel = strlen(re);
			if (de < rel) {
				res.append(re, de);
				res += '.';
				res += (re + de);
			}
			else {
				res.append(re, rel);
				res.append(de - rel, '0');
			}
		}
		else if (de < 0) {
			res += "0.";
			res.append(-1 * de, '0');
			res += re;
		}
#else
		res += to_string(*data);
#endif // IEEE_8087
	}
	inline static void serialize(string *data, string &res) {
		res += "\"" + *data + "\"";
	}
	inline static void serialize(float *data, string &res) {
		res += to_string(*data);
	}
	template<class V>
	inline static void serialize(vector<V> *data, string &res) {
		res += json_key_symbol::array_begin;
		for (auto &item : *data) {
			serialize(&item, res);
			res += json_key_symbol::next_key_value;
		}
		if (data->size() > 0)
			res.pop_back();
		res += json_key_symbol::array_end;
	}
	template<class V>
	inline static void serialize(V *data, string &res) {
		data->serialize(res);
	}

	inline static bool unserialize(bool *data, json_stream &js) {
		if (!parse_bool(*data, js)) {
			check_skip(js);
			return false;
		}
		return true;
	}

	inline static bool unserialize(int32_t *data, json_stream &js) {
		STRNG_TO_NUM(int32_t, strtol, strtof);
	}
	inline static bool unserialize(int8_t *data, json_stream &js) {
		STRNG_TO_NUM(int8_t, strtol, strtof);
		return true;
	}
	inline static bool unserialize(int16_t *data, json_stream &js) {
		STRNG_TO_NUM(int16_t, strtol, strtof);
		return true;
	}
	inline static bool unserialize(int64_t *data, json_stream &js) {
		STRNG_TO_NUM(int64_t, strtoll, strtod);
	}
	inline static bool unserialize(uint32_t *data, json_stream &js) {
		STRNG_TO_NUM(uint32_t, strtoul, strtod);
	}
	inline static bool unserialize(uint8_t *data, json_stream &js) {
		STRNG_TO_NUM(uint8_t, strtoul, strtod);
	}
	inline static bool unserialize(uint16_t *data, json_stream &js) {
		STRNG_TO_NUM(uint16_t, strtoul, strtod);
	}
	inline static bool unserialize(uint64_t *data, json_stream &js) {
		STRNG_TO_NUM(uint64_t, strtoull, strtod);
	}

	inline static bool unserialize(double *data, json_stream &js) {
		char *endptr;
		*data = strtod(js.begin, &endptr);
		if (endptr == js.begin) {
			check_skip(js);
			return false;
		}
		(js.begin) = endptr;
		return true;
	}
	inline static bool unserialize(float *data, json_stream &js) {
		char *endptr;
		*data = strtof(js.begin, &endptr);
		if (endptr == js.begin) {
			check_skip(js);
			return false;
		}
		(js.begin) = endptr;
		return true;
	}
	inline static bool unserialize(string *data, json_stream &js) {
		if (get_cur_and_next(js) == json_key_symbol::str) {
			data->resize(0);
			parse_str(*data, js);
		}
		else {
			check_skip(js);
			return false;
		}
		return true;
	}
	template<class V>
	inline static bool unserialize(V *data, json_stream &js) {
		if (*js.begin == json_key_symbol::object_begin) {
			return data->unserialize(js);;
		}
		else {
			check_skip(js);
			return false;
		}
		return true;
	}

	template<class V>
	static bool unserialize(vector<V> *data, json_stream &js) {
		// skip the white space and control char
		skip_space(js);

		//check the value type
		if (*js.begin != json_key_symbol::array_begin) {
			check_skip(js);
			return false;
		}

		data->reserve(2);
		while (char ch = get_cur_and_next(js)) {
			// '[' and ',' as the falg of value begin
			if (ch == json_key_symbol::array_begin || ch == json_key_symbol::next_key_value) {
				skip_space(js);

				if (*js.begin == json_key_symbol::array_end)
					return true;

				size_t index = data->size();
				data->resize(index + 1);
				unserialize(&((*data)[index]), js);

			}
			else if (ch == json_key_symbol::array_end) {
				return true;
			}
		}
		return false;
	}

	static bool parse_hex4(string &str,json_stream &js) {
		unsigned codepoint = 0;
		for (int i = 0; i < 4; i++) {
			char c = parser::get_cur_and_next(js);
			codepoint <<= 4;
			codepoint += static_cast<unsigned>(c);
			if (c >= '0' && c <= '9')
				codepoint -= '0';
			else if (c >= 'A' && c <= 'F')
				codepoint -= 'A' - 10;
			else if (c >= 'a' && c <= 'f')
				codepoint -= 'a' - 10;
			else {
				return 0;
			}
		}

		if (codepoint <= 0x7F)
			str += static_cast<char>(codepoint & 0xFF);
		else if (codepoint <= 0x7FF) {
			str += (static_cast<char>(0xC0 | ((codepoint >> 6) & 0xFF)));
			str += (static_cast<char>(0x80 | ((codepoint & 0x3F))));
		}
		else if (codepoint <= 0xFFFF) {
			str += (static_cast<char>(0xE0 | ((codepoint >> 12) & 0xFF)));
			str += (static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
			str += (static_cast<char>(0x80 | (codepoint & 0x3F)));
		}
		else {
			str += (static_cast<char>(0xF0 | ((codepoint >> 18) & 0xFF)));
			str += (static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
			str += (static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
			str += (static_cast<char>(0x80 | (codepoint & 0x3F)));
		}
		return true;
	}

	// end with '"' and skip "\""
	static bool parse_str(string &val, json_stream &js) {
		const char* b = js.begin;
		while (char ch = get_cur_and_next(js)) {
			if (ch == json_key_symbol::str) {
				val.append(b, js.begin - b - 1);
				return true;
			}
			else if (ch == '\\') {
				ch = get_cur_and_next(js);
				if (!js.is_option(UNESCAPE)) {
					if (ch == '\"') {
						val += '\"';
						b = js.begin;
					}
					else if (ch == '\\') {
						val += '\\';
						b = js.begin;
					}
					else if (ch == 'n') {
						val += '\n';
						b = js.begin;
					}
					else if (ch == 'b') {
						val += '\b';
						b = js.begin;
					}
					else if (ch == 'f') {
						val += '\f';
						b = js.begin;
					}
					else if (ch == 't') {
						val += '\t';
						b = js.begin;
					}
					else if (ch == 'r') {
						val += '\r';
						b = js.begin;
					}
					else if (ch == '/') {
						val += '/';
						b = js.begin;
					}
					else if (ch == 'u') {
						if (!js.is_option(UNESCAPE_UNICODE)) {
							parse_hex4(val, js);
							b = js.begin;
						}
					}
					else
						return false;
				}
			}
			else if (ch >= 0 && ch < 0x20) {
				return false;
			}
		}
		return false;
	}
};

//! Recursion-based parser
template<class T>
class json_base {

public:
	typedef T child_t;
	typedef bool(T::*unserialize_fun_t)(json_stream &js);
	typedef void(T::*serialize_fun_t)(string &res);

	// case 1:"xxx":xxx
	// case 2:"xxx":"xxx"
	// case 3:"xxx":{xxx}
	// case 4:"xxx":[xxx]
	bool parse_key_value(json_stream &js) {
		parser::skip_space(js);
		if (parser::get_cur_and_next(js) == parser::json_key_symbol::str) {
			const char* b = js.begin;
			parser::skip_str(js);
			no_copy_string key(b);
			parser::skip_space(js);
			if (parser::get_cur_and_next(js) == parser::json_key_symbol::key_value_separator) {
				parser::skip_space(js);
				return unserialize(key, js);
			}
			else
				return false;
		}
		return false;
	}

	bool parse_object(json_stream &js) {
		// only the key_value should be parsed in {}
		while (char ch = parser::get_cur_and_next(js)) {
			if (ch == parser::json_key_symbol::object_begin || ch == parser::json_key_symbol::next_key_value) {
				parse_key_value(js);
			}
			else if (ch == parser::json_key_symbol::object_end) {
				return true;
			}
		}
		return false;
	}

	bool unserialize(no_copy_string &key, json_stream &js) {
		auto &fields = field_collector<child_t>::fields();
		auto iter = fields.find(key);
		if (iter != fields.end()) {
			unserialize_fun_t &uns = (unserialize_fun_t&)iter->second.unserialize;
			return (((T*)this)->*uns)(js);
		}
		parser::check_skip(js);
		return false;
	}
public:
	bool unserialize(json_stream &js) {
		return parse_object(js);
	}
	// if *json end with '\0',don't need the size arg
	bool unserialize(const char* json, size_t size, char option = 0) {
		const char* begin = json;
		json_stream js{ begin,begin + size,option };
		return parse_object(js);
	}

	bool unserialize(const char* json, char option = 0) {
		const char* begin = json;
		json_stream js{ begin,nullptr,option };
		return parse_object(js);
	}

	void serialize(string &res) {
		auto &fields = field_collector<child_t>::fields();
		res += parser::json_key_symbol::object_begin;
		for (auto &filed : fields) {
			res += "\"";
			res += filed.first.str;
			res += "\":";
			serialize_fun_t &s = (serialize_fun_t &)filed.second.serialize;
			(((T*)this)->*s)(res);
			res += parser::json_key_symbol::next_key_value;
		}
		//pop the json_key_symbol::next_key_value(',')
		if (fields.size() > 0)
			res.pop_back();
		res += parser::json_key_symbol::object_end;
	}
};