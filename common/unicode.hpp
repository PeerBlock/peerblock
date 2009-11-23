/*
	Copyright (C) 2007 Cory Nelson

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <boost/cstdint.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace unicode {

typedef boost::uint8_t utf8_t;
typedef boost::uint16_t utf16_t;
typedef boost::uint32_t utf32_t;

/*
	unicode_error is thrown if any decoding or encoding errors occur.
*/
class unicode_error : public std::exception {
private:
	const char* const m_msg;

public:
	unicode_error(const char *msg) : m_msg(msg) {}
	const char* what() const throw() { return m_msg; }
};

/*
	utf8 is a pure static class that decodes and encodes UTF-8 characters from
	UTF-32 iterators.
*/
class utf8 {
private:
	// decodes a single octet of a UTF-8 codepoint.
	template<typename InputIterator>
	static utf32_t decode_part(InputIterator &iter, InputIterator &end) {
		if(iter == end) {
			throw unicode_error("incomplete utf-8 sequence");
		}

		utf32_t part = static_cast<utf8_t>(*iter);

		if((part & 0xC0) != 0x80) {
			throw unicode_error("invalid utf-8 octet");
		}

		++iter;
		return part & 0x3F;
	}

public:
	typedef utf8_t char_type;
	static const unsigned int max_encodelen = 4;

	// decodes a full UTF-8 codepoint.
	template<typename InputIterator>
	static utf32_t decode(InputIterator &iter, InputIterator &end) {
		utf32_t part, val = 0;

		if(iter == end) {
			throw unicode_error("empty iterator range");
		}

		part = static_cast<utf8_t>(*iter);
		if(!(part & 0x80)) { // 1 octet
			++iter;
			val = part;
		}
		else if((part & 0xE0) == 0xC0) { // 2 octets
			++iter;
			val = (part & 0x1F) << 6;
			val |= decode_part(iter, end);

			if(val < 0x80) {
				throw unicode_error("overlong utf-8 sequence");
			}
		}
		else if((part & 0xF0) == 0xE0) { // 3 octets
			++iter;
			val = (part & 0xF) << 12;
			val |= decode_part(iter, end) << 6;
			val |= decode_part(iter, end);

			if(val < 0x800) {
				throw unicode_error("overlong utf-8 sequence");
			}
		}
		else if((part & 0xF8) == 0xF0) { // 4 octets
			++iter;
			val = (part & 0x7) << 18;
			val |= decode_part(iter, end) << 12;
			val |= decode_part(iter, end) << 6;
			val |= decode_part(iter, end);

			if(val < 0x10000) {
				throw unicode_error("overlong utf-8 sequence");
			}
		}
		else {
			throw unicode_error("invalid utf-8 octet");
		}

		if(val > 0x10FFFF) {
			throw unicode_error("invalid unicode character");
		}

		return val;
	}

	// encodes a full codepoint.
	template<typename OutputIterator>
	static OutputIterator encode(utf32_t val, OutputIterator iter) {
		if(val <= 0x7F) {
			*iter = static_cast<utf8_t>(val); ++iter;
		}
		else if(val <= 0x7FF) {
			*iter = static_cast<utf8_t>(0xC0 | ((val & 0x7C0) >> 6)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | (val & 0x3F)); ++iter;
		}
		else if(val <= 0xFFFF) {
			*iter = static_cast<utf8_t>(0xE0 | ((val & 0xF000) >> 12)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | ((val & 0xFC0) >> 6)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | (val & 0x3F)); ++iter;
		}
		else if(val <= 0x10FFFF) {
			*iter = static_cast<utf8_t>(0xF0 | ((val & 0x1C0000) >> 18)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | ((val & 0x3F000) >> 12)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | ((val & 0xFC0) >> 6)); ++iter;
			*iter = static_cast<utf8_t>(0x80 | (val & 0x3F)); ++iter;
		}
		else {
			throw unicode_error("invalid unicode character");
		}

		return iter;
	}
};

/*
	host_endian always returns the integer unchanged.
*/
struct host_endian {
	static utf16_t swap(utf16_t i) { return i; }
	static utf32_t swap(utf32_t i) { return i; }
};

/*
	swap_endian converts the integer between big-endian and little-endian.
*/
struct swap_endian {
#ifdef _MSC_VER
	// VC++ has intrinsic functions for the x86 BSWAP instruction.
	static utf16_t swap(utf16_t i) { return _byteswap_ushort(i); }
	static utf32_t swap(utf32_t i) { return _byteswap_ulong(i); }
#else
	static utf16_t swap(utf16_t i) { return (i >> 8) | (i << 8); }

	static utf32_t swap(utf32_t i) {
		return (
			(i >> 24) |
			((i & 0x00FF0000) >> 8) |
			((i & 0x0000FF00) << 8) |
			(i << 24)
		);
	}
#endif
};

/*
	define BIG_ENDIAN if you are on a big-endian CPU, like PPC.
*/
#ifdef BIG_ENDIAN
typedef swap_endian little_endian;
typedef host_endian big_endian;
#else
typedef host_endian little_endian;
typedef swap_endian big_endian;
#endif

/*
	utf16 is a pure static class that decodes and encodes UTF-16 characters from
	UTF-32 iterators.
*/
template<typename Endian>
class utf16 {
public:
	typedef utf16_t char_type;
	static const unsigned int max_encodelen = 2;

	// decodes a full UTF-16 codepoint.
	template<typename InputIterator>
	static utf32_t decode(InputIterator &iter, InputIterator &end) {
		utf32_t part, val = 0;

		if(iter == end) {
			throw unicode_error("empty iterator range");
		}

		part = Endian::swap(static_cast<utf16_t>(*iter));
		if(part < 0xD800 || part > 0xDFFF) {
			val = part;
		}
		else {
			val = (part & 0x3FF) << 10;

			if(++iter == end) {
				throw unicode_error("incomplete utf-16 surrogate pair");
			}

			part = Endian::swap(static_cast<utf16_t>(*iter));
			val |= (part & 0x3FF);
		}
		
		++iter;

		if(val > 0x10FFFF) {
			throw unicode_error("invalid unicode character");
		}

		return val;
	}

	// encodes a full codepoint.
	template<typename OutputIterator>
	static OutputIterator encode(utf32_t val, OutputIterator iter) {
		if(val <= 0xFFFF) {
			*iter = Endian::swap(static_cast<utf16_t>(val)); ++iter;
		}
		else if(val <= 0x10FFFF) {
			val -= 0x10000;

			*iter = Endian::swap(static_cast<utf16_t>(0xD800 | (val >> 10))); ++iter;
			*iter = Endian::swap(static_cast<utf16_t>(0xDC00 | (val & 0x3FF))); ++iter;
		}
		else {
			throw unicode_error("invalid unicode character");
		}

		return iter;
	}
};

// platform-specific little-endian and big-endian decoders.
typedef utf16<little_endian> utf16le;
typedef utf16<big_endian> utf16be;

/*
	utf32 is a pure static class that performs basic checks and decodes UTF-32
	iterators to a proper endian.
*/
template<typename Endian>
class utf32 {
public:
	typedef utf32_t char_type;
	static const unsigned int max_encodelen = 1;

	// this does nothing but check for a valid codepoint, and change it to the proper endian.
	template<typename InputIterator>
	static utf32_t decode(InputIterator &iter, InputIterator &end) {
		if(iter == end) {
			throw unicode_error("empty iterator range");
		}

		utf32_t val = Endian::swap(static_cast<utf32_t>(*iter)); ++iter;

		if(val > 0x10FFFF) {
			throw unicode_error("invalid unicode character");
		}

		return val;
	}

	// this does nothing but check for a valid codepoint, and change it to the proper endian.
	template<typename OutputIterator>
	static OutputIterator encode(utf32_t val, OutputIterator iter) {
		if(val > 0x10FFFF) {
			throw unicode_error("invalid unicode character");
		}

		*iter = Endian::swap(val);
		return ++iter;
	}
};

// platform-specific little-endian and big-endian decoders.
typedef utf32<little_endian> utf32le;
typedef utf32<big_endian> utf32be;

/*
	wchar_encoding is a typedef that guesses the encoding of wchar_t on your
	platform.  it assumes wchar_t is UTF-16 (as on Windows) or UTF-32 (everywhere
	else)
*/
typedef boost::mpl::if_<
	boost::mpl::equal_to<boost::mpl::sizeof_<wchar_t>, boost::mpl::sizeof_<utf16_t> >,
	utf16<host_endian>,
	utf32<host_endian>
>::type wchar_encoding;

// does a quick transcode between two encodings.
template<typename InEnc, typename OutEnc, typename InputIterator, typename OutputIterator>
OutputIterator transcode(InputIterator iter, InputIterator end, OutputIterator out) {
	while(iter != end) {
		utf32_t ch = InEnc::decode(iter, end);
		out = OutEnc::encode(ch, out);
	}

	return out;
}

// transcodes one Range into another Range.
template<typename InEnc, typename OutEnc, typename InputRange, typename OutputRange>
void transcode(const InputRange &input, OutputRange &output) {
	transcode<InEnc, OutEnc>(boost::begin(input), boost::end(input), std::inserter(output, boost::end(output)));
}

}
