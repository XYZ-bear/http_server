#pragma once

#include "static_json.h"
#include <vector>
#include <functional>
#include <map>

/*Linear chain storage structure
Linear: Physical Structure,line memory space,like vector,
Chain: Logical Structure, the nodes linked by logical

In this structure, we can estimate the size of space in advance by the json length, and release the space once time
When visit the nodes, we will get a high memeory catch hit rate.

|---- | -------|---- | -------|---- | -------|---- | -------
  head	value   head	value   head	value  head	value ......
 -------------- -------------- -------------- --------------
	 node1           node2           node3        node4

head -> |-------------|
			t(char)		the value type number, string, object, array
		|-------------|
		   n(uint32)	the next brother node
		|-------------|
		   cl(uint32)	if t==obj or t==arr the cl mean the first child node, or mean the value length
		|-------------|
		   kl(uint8)	the key length, the default length is 1bit
key ->  |-------------|
			str(kl)
value ->|-------------|
			value
			.......
*/


//! json_value json_iteratorator
//! \tparam T the type of json_value
template <class T>
class json_iterator
{
public:
	json_iterator(T * pp,bool o,int off) :p(pp),end(o) {
		begin = off;
	}

	json_iterator(const json_iterator<T> &i) :p(i.p) {}

	json_iterator<T> & operator = (const json_iterator<T>& i) { p = i.p; return *this; }

	bool operator != (const json_iterator<T>& i) { 
		if (end == i.end) 
			return false;
		return true;
	}

	T & operator *() { return *p; }

	T & operator ++() { 
		end = p->next(begin);
		return *p;
	}
private:
	T * p;
	bool end;
	int begin;
};

//! Array - based stack structure
class json_stack {
public:
	//! Used to record stack information
	struct info
	{
		bool is_obj;
		int off;
	};
	vector<info> vec;
	json_stack() {
		vec.reserve(6);
	}
	void push(const info& in) {
		vec.emplace_back(in);
	}
	info& top() {
		return vec.back();
	}
	void pop() {
		vec.pop_back();
	}
	size_t size() {
		return vec.size();
	}
};

//! an efficient array-linked structure
struct json_value {
public:
	//! inorder to support old compiler, separeate with type_flag_t
	typedef char flag_t; 

	//! point to the next value
	typedef uint32_t next_t;

	//! by default, the json value memery size just support 4G
	typedef uint32_t length_t;

	//! just support 8byte length number
	typedef uint64_t number_t;

	//! when the key length <= 255, then store the key length
	//  if key length > 255, then store the length as 256, use strlen to get the real length
	typedef uint8_t key_t;

	//! all the json token type
	enum type_flag_t
	{
		emp_t = 0,
		pre_t = 1,
		num_t = 2,
		nul_t = 3,
		boo_t = 4,
		str_t = 5,
		arr_t = 6,
		obj_t = 7,
		del_t = 8,
		num_double_t = 9,
		num_int_t = 10
	};

	//! help you to push the childs into a vector
	struct vector_helper {
		json_value *root;
		vector<int> vec;
		json_value& operator [] (int index) {
			if (root && index < vec.size())
				return root->to_off(vec[index]);
			return *root;
		}
	};

	//! help you to push the childs into a map with key
	struct map_helper {
		json_value *root;
		unordered_map<no_copy_string, length_t> mapp;
		json_value& operator [] (const char* key) {
			if (root)
				return root->to_off(mapp[key]);
			return *root;
		}
	};

protected:
#pragma pack(push,1)
	//! Head interpreter
	struct head_t{
		//! type
		flag_t t;

		//! next node offset
		next_t n;

		//! t == arr_t or obj_t -> cl = child offset
		//  t == num_t -> cl = sub_type (num_int_t or num_double_t)
		//  t == str_t -> cl = strlen
		length_t cl;

		//! key length
		key_t kl;

		head_t() {
			t = 0;
			kl = 0;
			cl = 0;
			n = 0;
		}

		static inline length_t head_size() {
			return sizeof(head_t);
		}

		bool keycmp(const char* key) {
			if (!strcmp((const char*)this + head_size(), key)) {
				return true;
			}
			return false;
		}

		const char* get_key() {
			return (const char*)this + head_size();
		}

		char* get_val() {
			if(kl < 255)
				return kl == 0 ? (char*)this + head_size() + kl : (char*)this + head_size() + kl + 1;
			return (char*)this + head_size() + strlen(get_key()) + 1;
		}

		template<class N>
		void set_num(N num) {
			*(N*)(get_val()) = num;
		}

		void set_string(const char* str, length_t len) {
			memcpy(get_val(), str, len);
		}

		template<class N>
		N get_num() {
			if (t == type_flag_t::num_t || type_flag_t::boo_t)
				return *(N*)(get_val());
			return 0;
		}

		void get_string(string& str) {
			if (t == type_flag_t::str_t)
				str.assign(get_val(), cl);
		}

		const char* get_string() {
			return get_val();
		}

		bool equal(const char* str) {
			if (memcmp(str, get_val(), cl) == 0)
				return true;
			return false;
		}
	};
#pragma pack(pop)
public:
	json_value() {
	}

	//! the begin json_iteratorator
	json_iterator<json_value> begin() {
		length_t off = get_off();
		if (h->t == type_flag_t::obj_t || h->t == type_flag_t::arr_t) {
			h = (head_t*)(data.data() + h->cl);
		}
		return json_iterator<json_value>(this, false, off);
	}

	//! the end json_iteratorator
	json_iterator<json_value> end() {
		return json_iterator<json_value>(this, true, 0);
	}

	//! get object value
	json_value& operator [] (const char* key) {
		if (key && h) {
			// set this head as obj_t
			if (h->t == type_flag_t::pre_t) {
				h->t = type_flag_t::obj_t;
				h->cl = (length_t)data.size();
				push_head(type_flag_t::pre_t, key, (length_t)strlen(key));
			}
			else {
				// dont find the head -> add this key
				head_t *th = get_key_head(key);
				if (th == nullptr) {
					push_head(type_flag_t::obj_t);
					h->cl = (length_t)data.size();
					push_head(type_flag_t::pre_t, key, (length_t)strlen(key));
				}
				else if (h != th) {
					th->n = (next_t)data.size();
					push_head(type_flag_t::pre_t, key, (length_t)strlen(key));
				}
			}
		}
		return *this;
	}

	//! get arrray value
	json_value& operator [] (int index) {
		if (index >= 0) {
			// set this head as obj_t
			if (h->t == type_flag_t::pre_t) {
				h->t = type_flag_t::arr_t;
				h->cl = (length_t)data.size();
				push_head(type_flag_t::pre_t);
			}
			else {
				// dont find the head -> add this key
				head_t *th = get_index_head(index);
				if (th == nullptr) {
					push_head(type_flag_t::arr_t);
					h->cl = (length_t)data.size();
					push_head(type_flag_t::pre_t);
				}
				else if (h != th) {
					th->n = (next_t)data.size();
					push_head(type_flag_t::pre_t);
				}
			}
		}
		return *this;
	}

	//! get the next value head
	/*! 
		\return	If this is the last value return true else return false
	*/
	bool next(length_t begin = 0) {
		if (h->n) {
			h = (head_t*)(data.data() + h->n);
			return false;
		}
		else {
			h = (head_t*)(data.data() + begin);
			return true;
		}
	}

	//! assign a number
	template<class N>
	void operator = (N num) {
		static_assert(is_arithmetic<N>::value != 0, "Must be arithmetic type");
		if (h) {
			if (h->t == type_flag_t::num_t)
				h->set_num(num);
			else if (h->t == type_flag_t::pre_t) {
				push_num<N>() = num;
			}
			else if (h->t == type_flag_t::emp_t) {
				push_head(type_flag_t::num_t);
				push_num<N>() = num;
			}
			else {
				h->t = type_flag_t::del_t;
				h->n = (next_t)data.size();
				push_head_from(type_flag_t::num_t, h);
				push_num<N>() = num;
			}
		}
	}

	//! assign bool
	void operator = (bool num) {
		if (h) {
			if (h->t == type_flag_t::boo_t)
				h->set_num(num);
			else if (h->t == type_flag_t::pre_t) {
				push_num<bool>() = num;
				h->cl = 0;
				h->t = type_flag_t::boo_t;
			}
			else if (h->t == type_flag_t::emp_t) {
				push_head(type_flag_t::boo_t);
				push_num<bool>() = num;
			}
			else {
				h->t = type_flag_t::del_t;
				h->n = (next_t)data.size();
				push_head_from(type_flag_t::boo_t, h);
				push_num<bool>() = num;
			}
		}
	}

	//! assign null
	void operator = (nullptr_t null) {
		if (h) {
			if (h->t == type_flag_t::pre_t || h->t == type_flag_t::emp_t) {
				h->t = type_flag_t::nul_t;
			}
			else {
				h->t = type_flag_t::del_t;
				h->n = (next_t)data.size();
				push_head(type_flag_t::nul_t);
			}
		}
	}

	//! assign a string
	void operator = (const char* str) {
		if (h && str) {
			length_t len = (length_t)strlen(str);
			if (h->t == type_flag_t::str_t) {
				if (len <= h->cl) {
					h->cl = len;
					h->set_string(str, len + 1);
				}
				else {
					h->t = type_flag_t::del_t;
					h->n = (next_t)data.size();
					push_head_from(type_flag_t::str_t, h);
					h->cl = len;
					push_str(str, len);
				}
			}
			else if (h->t == type_flag_t::pre_t) {
				h->t = type_flag_t::str_t;
				h->cl = len;
				length_t off = data.size();
				push_str(str, len);
			}
			else if (h->t == type_flag_t::emp_t) {
				push_head(type_flag_t::str_t);
				h->cl = len;
				push_str(str, len);
			}
			else if ((h->t == type_flag_t::num_t || h->t == type_flag_t::boo_t) && len < sizeof(number_t)) {
				h->t = type_flag_t::str_t;
				h->cl = len;
				h->set_string(str, len + 1);
			}
			else {
				h->t = type_flag_t::del_t;
				h->n = (next_t)data.size();
				push_head_from(type_flag_t::str_t, h);
				h->cl = len;
				push_str(str, len);
			}
		}
	}

	//! get number
	template<class Num>
	operator Num()
	{
		static_assert(is_arithmetic<Num>::value != 0, "Must be arithmetic type");
		return h->get_num<Num>();
	}

	//! you'd better do not use this stupied way to get string value
	operator string() {
		string res;
		h->get_string(res);
		return res;
	}

	//! an elegant way to get the real type value 
	/*! \brief the traditional way: const char* v = value.get_string() or const char* v = value.get<string>(), urgly
			   this way: const char* v = value;
		\return	implicit convert the json_value to const char*
	*/
	operator const char*() {
		return h->get_string();
	}

	//! compare with string
	/*! 
		\tparam N it could be uint8_t,int8_t,uint16_t,int16_t,uint32_t,int32_t,float,double all the number length < 8byte
	*/
	bool operator == (const char* str) {
		if (h->t == type_flag_t::str_t)
			return h->equal(str);
		else
			return false;
	}

	//! compare with number
	/*! 
		\tparam N it could be uint8_t,int8_t,uint16_t,int16_t,uint32_t,int32_t,float,double all the number length < 8byte 
	*/
	template<class N>
	bool operator == (N num) {
		static_assert(is_arithmetic<N>::value != 0, "Must be arithmetic type");
		if (h->t == type_flag_t::num_t || h->t == type_flag_t::boo_t)
			return h->get_num<N>() == num;
		else
			return false;
	}

	//! compare with null
	bool operator == (nullptr_t null) {
		if (h->t == type_flag_t::nul_t)
			return true;
		else
			return false;
	}

	//! erase a value
	/*! \brief	For efficiency, just set the value flag as del_t, so there the memory move will not happen
				when insert a new value the memery may be use again. 
	*/
	void erase() {
		if (h) {
			h->t = type_flag_t::del_t;
		}
	}

	//! find a key
	//! \return exist -> true or -> false
	bool find(const char* key) {
		if (head_t *th = get_key_head(key)) {
			if (th == h)
				return true;
		}
		return false;
	}

	bool is_object() {
		return h->t == type_flag_t::obj_t;
	}

	bool is_array() {
		return h->t == type_flag_t::arr_t;
	}

	bool is_string() {
		return h->t == type_flag_t::str_t;
	}

	bool is_number() {
		return h->t == type_flag_t::num_t;
	}

	bool is_bool() {
		return h->t == type_flag_t::boo_t;
	}

	bool is_null() {
		return h->t == type_flag_t::nul_t;
	}

	bool is_number_int() {
		return h->cl == type_flag_t::num_int_t;
	}

	bool is_number_double() {
		return h->cl == type_flag_t::num_double_t;
	}

	//! get the child size or value size
	/*! \return [],{} -> child size,
				number -> sizeof(number_t) = 8byte
				string -> length(string)
	*/
	size_t size() {
		if (h->t == type_flag_t::arr_t || h->t == type_flag_t::obj_t)
			return count_size();
		else if (h->t == type_flag_t::num_t) {
			return sizeof(number_t);
		}
		return h->cl;
	}

	//! get the key of the value
	/*! \return in [] -> return null
				in {} -> return the key
	*/
	const char* key() {
		if (h->kl)
			return h->get_key();
		else
			return "";
	}

	//! build all the same level data to a map
	/*! \brief If you need to find a value by index in high frequency, this will help you.
			   If you want to know why this api is necessary, you should know about the json_value Memeary model
		\param vh a vector_helper reference
	*/
	void build_vector_helper(vector_helper& vh) {
		if (h && h->t == type_flag_t::arr_t) {
			vh.root = this;
			const char* begin = data.data();
			//// point to the child begin
			head_t *th = (head_t*)(begin + h->cl);
			vh.vec.push_back(h->cl);
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					vh.vec.push_back(th->n);
				}
				// return the end head
				if (!th->n)
					return;
				// point to the brother head
				th = (head_t*)(begin + th->n);

			}
		}
	}

	//! build all the same level data to a map
	/*! \brief if you need to find a value by key in high frequency, this will help you 
		\param mh a map_helper reference
	*/
	void build_map_helper(map_helper& mh) {
		if (h && h->t == type_flag_t::obj_t) {
			mh.root = this;
			const char* begin = data.data();
			//// point to the child begin
			head_t *th = (head_t*)(begin + h->cl);
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					mh.mapp[th->get_key()] = (length_t)((const char*)th - data.data());
				}
				// return the end head
				if (!th->n)
					return;
				// point to the brother head
				th = (head_t*)(begin + th->n);

			}
		}
	}

	void serialize(string &str) {
		str.resize(0);
		str.reserve(data.size());
		json_stack stack;
		head_t *th = h;

		while (th) {
			if (th->t != type_flag_t::del_t) {
				if (stack.size() && stack.top().is_obj) {
					if (th->kl) {
						str += "\"";
						str += th->get_key();
						str += "\":";
					}
					else {
						str += "\"\":";
					}
				}

				if (th->t == type_flag_t::obj_t) {
					if (th->cl) {
						str += "{";
						stack.push({ true,(int)((const char*)th - data.data()) });
						th = (head_t*)(data.data() + th->cl);
						continue;
					}
					else {
						th = (head_t*)(data.data() + th->n);
						str += "{},";
					}
					//continue;
				}
				else if (th->t == type_flag_t::arr_t) {
					if (th->cl) {
						str += "[";
						stack.push({ false,(int)((const char*)th - data.data()) });
						th = (head_t*)(data.data() + th->cl);
						continue;
					}
					else {
						th = (head_t*)(data.data() + th->n);
						str += "[],";
					}
					
				}

				if (th->t == type_flag_t::num_t) {
					if (th->cl == type_flag_t::num_int_t) {
						int64_t i64 = th->get_num<int64_t>();
						parser::serialize(&i64, str);
					}
					else {
						double d64 = th->get_num < double > ();
						parser::serialize(&d64, str);
					}
					str += ',';
				}
				else if (th->t == type_flag_t::str_t) {
					str += "\"";
					str += th->get_string();
					str += "\",";
				}
				else if (th->t == type_flag_t::nul_t) {
					str += "null";
					str += ',';
				}
				else if (th->t == type_flag_t::boo_t) {
					if (th->get_num<bool>())
						str += "true";
					else
						str += "false";
					str += ',';
				}
			}
			while (!th->n) {
				str.pop_back();
				if (stack.size()) {
					if (stack.top().is_obj)
						str += "},";
					else
						str += "],";
					th = (head_t*)(data.data() + stack.top().off);
					stack.pop();
				}
				else
					return;
			}
			th = (head_t*)(data.data() + th->n);
		}
	}

	void swap(json_value& jv) {
		data.swap(jv.data);
		init();
	}

private:
	// delete the copy structor and operator, avoid freshman to do some stupid things
	// if you really really want to copy a entity, please use copy_from
	json_value(const json_value &) = delete;
	json_value& operator = (const json_value&) = delete;

	head_t* goto_next_usable_head() {
		if (h ) {
			const char* begin = data.data();
			// point to the child begin
			head_t *th = h;
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					h = th;
					return th;
				}
				// return the end head
				if (!th->n)
					return th;
				// point to the brother head
				th = (head_t*)(begin + th->n);
			}
			return th;
		}
		return nullptr;
	}

	length_t count_size() {
		if (h && h->cl) {
			// point to the child begin
			head_t *th = (head_t*)(data.data() + h->cl);
			length_t i = 0;
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					i++;
				}
				// return the end head
				if (!th->n)
					return i;
				// point to the brother head
				th = (head_t*)(data.data() + th->n);

			}
			return i;
		}
		return 0;
	}
	head_t* get_index_head(length_t index) {
		if (h && h->t == type_flag_t::arr_t) {
			const char* begin = data.data();
			// point to the child begin
			head_t *th = (head_t*)(begin + h->cl);
			uint32_t i = 0;
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					// if find the index -> reset the head
					if (i == index) {
						h = th;
						return th;
					}
					i++;
				}
				// return the end head
				if (!th->n)
					return th;
				// point to the brother head
				th = (head_t*)(begin + th->n);

			}
			return th;
		}
		return nullptr;
	}

	head_t* get_key_head(const char* key) {
		if (h && h->t == type_flag_t::obj_t) {
			const char* begin = data.data();
			// point to the child begin
			head_t *th = (head_t*)(begin + h->cl);
			while (th) {
				// if this node was deleted -> jump
				if (th->t != type_flag_t::del_t) {
					// if find the key -> reset the head
					if (th->keycmp(key)) {
						h = th;
						return th;
					}
				}
				// return the end head
				if (!th->n)
					return th;
				// point to the brother head
				th = (head_t*)(begin + th->n);
			}
			return th;
		}
		return nullptr;
	}

protected:
	void update_head(int off) {
		h = (head_t*)(data.data() + off);
	}

	void init() {
		h = (head_t*)data.data();
	}

	void pre_allocate(size_t base_size) {
		data.reserve(base_size + base_size / 3 );
		data.resize(0);
	}

	json_value& to_off(int off) {
		h = (head_t*)(data.data() + off);
		return *this;
	}

	void set_next() {
		h->n = (next_t)data.size();
	}


	void update_cl() {
		h->cl = (length_t)data.size();
	}

	void update_cl(length_t length) {
		h->cl = length;
	}

	void update_cl0() {
		h->cl = 0;
	}

	flag_t get_flag() {
		return h->t;
	}

	length_t get_off() {
		return length_t((const char*)h - data.data());
	}

	void push_head(flag_t t) {
		//add head
		length_t off = (length_t)data.size();
		data.resize(data.size() + head_t::head_size());
		h = (head_t*)(data.data() + off);
		h->t = t;
	}

	void push_head(flag_t t, const char* key, length_t kl) {
		//add head
		length_t off = (length_t)data.size();
		data.resize(data.size() + head_t::head_size());
		//add key end with '\0'
		push_str(key, kl);

		h = (head_t*)(data.data() + off);
		h->t = t;

		if (kl < 255)
			h->kl = kl;
		else
			h->kl = 255;
	}

	inline string& get_data() {
		return data;
	}

	void set_flag(char f) {
		h->t = f;
	}

	void push_head_from(flag_t t, head_t* from) {
		length_t old_off = length_t((const char*)from - data.data());
		//add head
		length_t off = length_t(data.size());
		data.resize(data.size() + head_t::head_size());
		//add key end with '\0'
		from = (head_t*)(data.data() + old_off);
		key_t kl = from->kl;
		if (kl)
			push_str(from->get_key());
		//cur head
		h = (head_t*)(data.data() + off);
		//old_head
		h->t = t;
		h->kl = kl;
	}

	inline void push_str(const char* str, length_t len) {
		length_t head_off = length_t((const char*)h - data.data());
		data.append(str, len);
		data += '\0';
		h = (head_t*)(data.data() + head_off);
	}

	inline void push_str(const char* str) {
		length_t head_off = length_t((const char*)h - data.data());
		data += str;
		data += '\0';
		h = (head_t*)(data.data() + head_off);
	}

	template<class N>
	inline N& push_num() {
		h->t = type_flag_t::num_t;
		length_t head_off = length_t((const char*)h - data.data());
		data.resize(data.size() + sizeof(number_t));
		h = (head_t*)(data.data() + head_off);
		set_num_cl<N>();
		return *(N*)h->get_val();
	}

	template<class N>
	inline void set_num_cl() {
		h->cl = type_flag_t::num_int_t;
	}

	template<>
	inline void set_num_cl<double>() {
		h->cl = type_flag_t::num_double_t;
	}

	template<>
	inline void set_num_cl<float>() {
		h->cl = type_flag_t::num_double_t;
	}

	void root_copy(json_value& jv) {
		data = jv.data;
		h = jv.h;
	}

private:
	string data;
	head_t* h;
};

//! a Non - recursive based parser
class dynamic_json :public json_value {
public:
	dynamic_json() {
	}
	~dynamic_json() {
	}
private:
	bool parse_string(json_stream &js) {
		parser::get_next(js);
		set_flag(type_flag_t::str_t);
		int head_off = get_off();
		size_t end = get_data().size();
		if (!parser::parse_str(get_data(), js)) {
			ERROR_RETURT(js);
		}
		update_head(head_off);
		update_cl(length_t(get_data().size() - end));
		get_data() += '\0';
		return true;
	}

	bool parse_number(json_stream &js) {
		if (char ch = *js.begin) {
			if (ch == 'f' || ch == 't') {
				if (!parser::parse_bool(push_num<bool>(), js)) {
					ERROR_RETURT(js);
				}
				set_flag(type_flag_t::boo_t);
			}
			else if (ch == 'n') {
				if (!parser::is_null(js)) {
					ERROR_RETURT(js);
				}
				set_flag(type_flag_t::nul_t);
			}
			else {
				char res = parser::is_double(js);
				if (res == -1) {
					if (!parser::unserialize(&push_num<double>(), js)) {
						ERROR_RETURT(js);
					}
				}
				else if (res == 1) {
					if (!parser::unserialize(&push_num<uint64_t>(), js)) {
						ERROR_RETURT(js);
					}
				}
				else {
					ERROR_RETURT(js);
				}
			}
		}
		return true;
	}


	//! Non recursive implementation, so there is no limit on the depth, it is up on your memory size
	/*! \brief 	obj:{ -> "key" -> : -> value -> }
				arr:[ -> value -> ]
		\return a reference of json_value
	*/
	bool parse(json_stream &js) {
		json_stack stack;
		parser::skip_space(js);


		if (*js.begin == parser::json_key_symbol::str) {
			push_head(type_flag_t::pre_t);
			if (!parse_string(js))
				return false;
			parser::skip_space(js);
			if (parser::get_cur_and_next(js) != '\0') {
				ERROR_RETURT(js);
			}
			return true;
		}
		else if (*js.begin == parser::json_key_symbol::object_begin || *js.begin == parser::json_key_symbol::array_begin) {
			while (char ch = parser::get_cur_and_next(js)) {
				//step 1: check start tokens , [ {
				//--------------------------------------------------------
				parser::skip_space(js);
				// , end and begin token
				if (ch == parser::json_key_symbol::next_key_value) {
					set_next();
				}
				else if (ch == parser::json_key_symbol::object_begin) {
					if (get_flag() == type_flag_t::pre_t) {
						set_flag(type_flag_t::obj_t);
						update_cl();
					}
					else {
						push_head(type_flag_t::obj_t);
						update_cl();
					}
					stack.push({ true,(int)get_off() });
				}
				else if (ch == parser::json_key_symbol::array_begin) {
					//[], empty array
					if (*js.begin == parser::json_key_symbol::array_end) {
						parser::get_next(js);
						parser::skip_space(js);
						set_flag(type_flag_t::arr_t);
						update_cl0();
						continue;
					}
					//[x,x,x] and [],[],[]
					if (get_flag() == type_flag_t::pre_t) {
						set_flag(type_flag_t::arr_t);
						update_cl();
					}
					else {
						push_head(type_flag_t::arr_t);
						update_cl();
					}
					stack.push({ false,(int)get_off() });
				}
				else if (ch == parser::json_key_symbol::object_end) {
					if (stack.size() > 0 && stack.top().is_obj) {
						update_head(stack.top().off);
						stack.pop();
					}
					else {
						ERROR_RETURT(js);
					}
					parser::skip_space(js);
					continue;
				}
				else if (ch == parser::json_key_symbol::array_end) {
					if (stack.size() > 0 && !stack.top().is_obj) {
						update_head(stack.top().off);
						stack.pop();
					}
					else {
						ERROR_RETURT(js);
					}
					parser::skip_space(js);
					continue;
				}
				if (stack.size() == 0) {
					ERROR_RETURT(js);
				}

				//step 2: parse key
				//-----------------------------------------------------------
				//{ "x":x } obj head with key ,or [x,x,x] arr head without key 
				parser::skip_space(js);
				if (stack.top().is_obj) {
					if (ch = parser::get_cur_and_next(js)) {
						//key must start with quote
						if (ch == parser::json_key_symbol::str) {
							//push head
							const char* b = js.begin;
							push_head(type_flag_t::pre_t, b, (length_t)parser::skip_str(js));

							//check key_value separator
							parser::skip_space(js);
							if (parser::get_cur_and_next(js) != parser::json_key_symbol::key_value_separator) {
								ERROR_RETURT(js);
							}
						}
						//if no value pop stack
						else if (ch == parser::json_key_symbol::object_end) {
							update_cl0();
							stack.pop();
							continue;
						}
						else {
							//error format
							ERROR_RETURT(js);
						}
					}
					else {
						//error format
						ERROR_RETURT(js);
					}
				}
				else {
					push_head(type_flag_t::pre_t);
				}

				//step 3: parse value
				//-----------------------------------------------------------
				parser::skip_space(js);
				if (char ch = *js.begin) {
					if (ch == parser::json_key_symbol::str) {
						if (!parse_string(js))
							return false;
					}
					else if (ch == parser::json_key_symbol::object_begin) {
						continue;
					}
					else if (ch == parser::json_key_symbol::array_begin) {
						continue;
					}
					else {
						if (!parse_number(js))
							return false;
					}
				}
				parser::skip_space(js);

				//step 4: check end tokens
				//-----------------------------------------------------------
				//check end token only } ] , is allowed
				if (*js.begin == parser::json_key_symbol::next_key_value || *js.begin == parser::json_key_symbol::object_end || *js.begin == parser::json_key_symbol::array_end) {
					continue;
				}

				ERROR_RETURT(js);
				//-----------------------------------------------------------
			}

			if (stack.size() == 0)
				return true;
		}
		else {
			push_head(type_flag_t::pre_t);
			if (!parse_number(js))
				return false;
			parser::skip_space(js);
			if (parser::get_cur_and_next(js) != '\0') {
				ERROR_RETURT(js);
			}
			return true;
		}
		return false;
	}
public:
	void copy_from(json_value& jv) {
		root_copy(jv);
	}

	//! get a value in object by key
	/*! \param key
		\return a reference of json_value
	*/
	json_value & operator [] (const char* key) {
		init();
		return json_value::operator[](key);
	}

	//! get a value in array by index
	/*! \param index 
		\return a reference of json_value
	*/
	json_value& operator [] (int index) {
		init();
		return json_value::operator[](index);
	}

	//! assign a number
	template<class N>
	void operator = (N num) {
		init();
		return json_value::operator=(num);
	}

	//! assign a string
	void operator = (const char* str) {
		init();
		return json_value::operator=(str);
	}

	//! assign a string
	void operator = (bool num) {
		init();
		return json_value::operator=(num);
	}

	//! assign null
	void operator = (nullptr_t null) {
		init();
		return json_value::operator=(null);
	}

	//json_value& next(traverse_helper& helper) {
	//	init();
	//	return json_value::next(helper);
	//}

	void serialize(string &str) {
		init();
		return json_value::serialize(str);
	}

	//! unserialize a string with length
	/*! \brief In this API, the value memery will pre_allocated once time, this is benificial to parse a big json with a high memery
			   allocation efficiency, algorithm: size = size + size / 3 
		\param js json format string
		\param size the length of the jsonbuffer
		\param option ASSERT|UNESCAPE|UNESCAPE_UNICODE, when there are multiple options, you can combat them by OR operation
		\return the lenght of
	*/
	size_t unserialize(const char* json, size_t size, char option = 0) {
		pre_allocate(size);
		init();
		const char* begin = json;
		json_stream js{ begin,begin + size,option };
		if (parse(js))
			return js.begin - json;
		return 0;
	}

	//! unserialize a string with end flag '\0'.
	/*! \param js json format string
		\param option ASSERT|UNESCAPE|UNESCAPE_UNICODE, when there are multiple options, you can combat them by OR operation
		\return the lenght of
	*/
	size_t unserialize(const char* json, char option = 0) {
		pre_allocate(0);
		init();
		const char* begin = json;
		json_stream js{ begin,nullptr,option };
		if (parse(js))
			return js.begin - json;
		return 0;
	}

	//! unserialize a string.
	/*! \param js json format string
		\param option ASSERT|UNESCAPE|UNESCAPE_UNICODE, when there are multiple options, you can combat them by OR operation
		\return the lenght of
	*/
	size_t unserialize(const string &js, char option = 0) {
		return unserialize(js.data(), js.size(), option);
	}
};