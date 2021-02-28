/* Copyright 2021 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NUSPELL_UNICODE_HXX
#define NUSPELL_UNICODE_HXX
#include <string_view>
#include <unicode/utf16.h>
#include <unicode/utf8.h>

namespace nuspell {
inline namespace v5 {

// UTF-8, work on malformed

inline constexpr auto u8_max_cp_length = U8_MAX_LENGTH;

auto inline u8_is_cp_error(int32_t cp) -> bool { return cp < 0; }

template <class Range>
auto u8_advance_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	using std::size, std::data;
#if U_ICU_VERSION_MAJOR_NUM <= 60
	auto s_ptr = data(str);
	int32_t idx = i;
	int32_t len = size(str);
	U8_NEXT(s_ptr, idx, len, cp);
	i = idx;
#else
	auto len = size(str);
	U8_NEXT(str, i, len, cp);
#endif
}

template <class Range>
auto u8_advance_cp_index(const Range& str, size_t& i) -> void
{
	using std::size;
	auto len = size(str);
	U8_FWD_1(str, i, len);
}

template <class Range>
auto u8_reverse_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	using std::size, std::data;
	auto ptr = data(str);
	int32_t idx = i;
	U8_PREV(ptr, 0, idx, cp);
	i = idx;
}

template <class Range>
auto u8_reverse_cp_index(const Range& str, size_t& i) -> void
{
	using std::size, std::data;
	auto ptr = data(str);
	int32_t idx = i;
	U8_BACK_1(ptr, 0, idx);
	i = idx;
}

template <class Range>
auto u8_write_cp_and_advance(Range& buf, size_t& i, int32_t cp, bool& error)
    -> void
{
	using std::size, std::data;
#if U_ICU_VERSION_MAJOR_NUM <= 60
	auto ptr = data(buf);
	int32_t idx = i;
	int32_t len = size(buf);
	U8_APPEND(buf, idx, len, cp, error);
	i = idx;
#else
	auto len = size(buf);
	U8_APPEND(buf, i, len, cp, error);
#endif
}

// UTF-8, valid

template <class Range>
auto valid_u8_advance_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	U8_NEXT_UNSAFE(str, i, cp);
}

template <class Range>
auto valid_u8_advance_cp_index(const Range& str, size_t& i) -> void
{
	U8_FWD_1_UNSAFE(str, i);
}

template <class Range>
auto valid_u8_reverse_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	U8_PREV_UNSAFE(str, i, cp);
}

template <class Range>
auto valid_u8_reverse_cp_index(const Range& str, size_t& i) -> void
{
	U8_BACK_1_UNSAFE(str, i);
}

template <class Range>
auto valid_u8_write_cp_and_advance(Range& buf, size_t& i, char32_t cp) -> void
{
	U8_APPEND_UNSAFE(buf, i, cp);
}

// UTF-16, work on malformed

inline constexpr auto u16_max_cp_length = U16_MAX_LENGTH;

auto inline u16_is_cp_error(int32_t cp) -> bool { return U_IS_SURROGATE(cp); }

template <class Range>
auto u16_advance_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	using std::size;
	auto len = size(str);
	U16_NEXT(str, i, len, cp);
}

template <class Range>
auto u16_advance_cp_index(const Range& str, size_t& i) -> void
{
	using std::size;
	auto len = size(str);
	U16_FWD_1(str, i, len);
}

template <class Range>
auto u16_reverse_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	U16_PREV(str, 0, i, cp);
}

template <class Range>
auto u16_reverse_cp_index(const Range& str, size_t& i) -> void
{
	U16_BACK_1(str, 0, i);
}

template <class Range>
auto u16_write_cp_and_advance(Range& buf, size_t& i, int32_t cp, bool& error)
    -> void
{
	using std::size;
	auto len = size(buf);
	U16_APPEND(buf, i, len, cp, error);
}

// UTF-16, valid

template <class Range>
auto valid_u16_advance_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	U16_NEXT_UNSAFE(str, i, cp);
}

template <class Range>
auto valid_u16_advance_cp_index(const Range& str, size_t& i) -> void
{
	U16_FWD_1_UNSAFE(str, i);
}

template <class Range>
auto valid_u16_reverse_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	U16_PREV_UNSAFE(str, i, cp);
}

template <class Range>
auto valid_u16_reverse_cp_index(const Range& str, size_t& i) -> void
{
	U16_BACK_1_UNSAFE(str, i);
}

template <class Range>
auto valid_u16_write_cp_and_advance(Range& buf, size_t& i, char32_t cp) -> void
{
	U16_APPEND_UNSAFE(buf, i, cp);
}

// higer level funcs

struct U8_CP_Pos {
	size_t begin_i = 0;
	size_t end_i = begin_i;
};

class U8_Encoded_CP {
	char d[u8_max_cp_length];
	int sz;

      public:
	explicit U8_Encoded_CP(std::string_view str, U8_CP_Pos pos)
	    : sz(pos.end_i - pos.begin_i)
	{
		str.copy(d, sz, pos.begin_i);
	}
	U8_Encoded_CP(char32_t cp)
	{
		size_t z = 0;
		valid_u8_write_cp_and_advance(d, z, cp);
		sz = z;
	}
	auto size() const noexcept -> size_t { return sz; }
	auto data() const noexcept -> const char* { return d; }
	operator std::string_view() const noexcept
	{
		return std::string_view(data(), size());
	}
};

// bellow go func without out-parametars

// UTF-8, can be malformed, no out-parametars

struct Idx_And_Next_CP {
	size_t end_i;
	int32_t cp;
};

struct Idx_And_Prev_CP {
	size_t begin_i;
	int32_t cp;
};

struct Write_CP_Idx_and_Error {
	size_t end_i;
	bool error;
};

template <class Range>
[[nodiscard]] auto u8_next_cp(const Range& str, size_t i) -> Idx_And_Next_CP
{
	int32_t cp;
	u8_advance_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
[[nodiscard]] auto u8_next_cp_index(const Range& str, size_t i) -> size_t
{
	u8_advance_cp_index(str, i);
	return i;
}

template <class Range>
[[nodiscard]] auto u8_prev_cp(const Range& str, size_t i) -> Idx_And_Prev_CP
{
	int32_t cp;
	u8_reverse_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
[[nodiscard]] auto u8_prev_cp_index(const Range& str, size_t i) -> size_t
{
	u8_reverse_cp_index(str, i);
	return i;
}

template <class Range>
[[nodiscard]] auto u8_write_cp(Range& buf, size_t i, int32_t cp)
    -> Write_CP_Idx_and_Error
{
	bool err;
	u8_write_cp_and_advance(buf, i, cp, err);
	return {i, err};
}

// UTF-8, valid, no out-parametars

struct Idx_And_Next_CP_Valid {
	size_t end_i;
	char32_t cp;
};

struct Idx_And_Prev_CP_Valid {
	size_t begin_i;
	char32_t cp;
};

template <class Range>
[[nodiscard]] auto valid_u8_next_cp(const Range& str, size_t i)
    -> Idx_And_Next_CP_Valid
{
	char32_t cp;
	valid_u8_advance_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
[[nodiscard]] auto valid_u8_next_cp_index(const Range& str, size_t i) -> size_t
{
	valid_u8_advance_cp_index(str, i);
	return i;
}

template <class Range>
[[nodiscard]] auto valid_u8_prev_cp(const Range& str, size_t i)
    -> Idx_And_Prev_CP_Valid
{
	char32_t cp;
	valid_u8_reverse_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
[[nodiscard]] auto valid_u8_prev_cp_index(const Range& str, size_t i) -> size_t
{
	valid_u8_reverse_cp_index(str, i);
	return i;
}

template <class Range>
[[nodiscard]] auto valid_u8_write_cp(Range& buf, size_t i, int32_t cp) -> size_t
{
	valid_u8_write_cp_and_advance(buf, i, cp);
	return i;
}

template <class CharT>
struct UTF16_Traits {
	static_assert(sizeof(CharT) == 2);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = u16_max_cp_length;
	struct Encoded_CP {
		CharT seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp)
		{
			valid_u16_write_cp_and_advance(seq, size, cp);
		}
	};
	UTF16_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		u16_advance_cp(s, i, c);
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return u16_is_cp_error(cp);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		valid_u16_advance_cp(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View s, size_t& i) -> void
	{
		valid_u16_reverse_cp_index(s, i);
	}
};

template <class CharT>
struct UTF_Traits {
	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = 1;
	struct Encoded_CP {
		CharT seq[1];
		static constexpr size_t size = 1;
		Encoded_CP(char32_t cp) { seq[0] = cp; }
	};
	UTF_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return !(0 <= cp && cp <= 0x10ffff);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View, size_t& i) -> void { --i; }
};

template <>
struct UTF_Traits<char> {
	using String_View = std::string_view;

	static constexpr size_t max_width = U8_MAX_LENGTH;

	struct Encoded_CP {
		char seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp)
		{
			valid_u8_write_cp_and_advance(seq, size, cp);
		}
	};
	UTF_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		u8_advance_cp(s, i, c);
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return u8_is_cp_error(cp);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		char32_t c;
		valid_u8_advance_cp(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View s, size_t& i) -> void
	{
		valid_u8_reverse_cp_index(s, i);
	}
};

template <>
struct UTF_Traits<char16_t> : UTF16_Traits<char16_t> {
};
#if U_SIZEOF_WCHAR_T == 2
template <>
struct UTF_Traits<wchar_t> : UTF16_Traits<wchar_t> {
};
#endif

} // namespace v5
} // namespace nuspell
#endif // NUSPELL_UNICODE_HXX