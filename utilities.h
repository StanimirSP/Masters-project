#ifndef UTILITIES_H
#define UTILITIES_H

#include <iterator>
#include <utility>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <string>
#include <string_view>
#include <cstddef>
#include <functional>
#include "constants.h"

template<class Compare = std::less<>>
struct IndirectlyCompare
{
	Compare cmp;
	IndirectlyCompare(const Compare& cmp = Compare()): cmp(cmp) {}
	template<std::indirectly_readable T, std::indirectly_readable U>
	constexpr bool operator()(T&& a, U&& b) const
	{
		return cmp(*std::forward<T>(a), *std::forward<U>(b));
	}
};

struct SymbolOrEpsilon
{
	Symbol c;
	constexpr SymbolOrEpsilon(): c(Constants::Epsilon) {}
	constexpr SymbolOrEpsilon(Symbol c) : c(c) {}
	constexpr SymbolOrEpsilon(std::string_view s) : c(s[0]) {}
	static constexpr SymbolOrEpsilon epsilon() noexcept { return {}; }
	static constexpr bool isBegin(Symbol c) noexcept
	{
		return c == Constants::Epsilon || !Constants::isSpecial(c);
	}
	static constexpr bool isEnd(Symbol c) noexcept
	{
		return isBegin(c);
	}
	auto operator<=>(Symbol c) const noexcept
	{
		return this->c <=> c;
	}
	bool operator==(Symbol c) const noexcept
	{
		return this->c == c;
	}
	operator Symbol() const noexcept
	{
		return c;
	}
	auto operator<=>(const SymbolOrEpsilon&) const noexcept = default;
	bool operator==(const SymbolOrEpsilon&) const noexcept = default;
};

struct SymbolPair
{
	SymbolOrEpsilon coords[2];
	static constexpr SymbolPair epsilon() { return {}; }
	auto operator<=>(const SymbolPair&) const noexcept = default;
	bool operator==(const SymbolPair&) const noexcept = default;
};

inline void normalize(Word& w)
{
	std::erase(w, Constants::Epsilon);
}

struct WordPair
{
	Word coords[2];
	constexpr WordPair(): coords{Word(1, Constants::Epsilon)/*, Word(1, Constants::Epsilon)*/} {}
	constexpr WordPair(std::string_view wp)
	{
		wp.remove_prefix(1); // Constants::BaseElementBegin
		wp.remove_suffix(1); // Constants::BaseElementEnd
		std::size_t delimPos = wp.find(Constants::BaseElementDelim);
		if(delimPos == std::string_view::npos)
			throw std::runtime_error("Bad regular expression: missing delimiter in base element");
		coords[0] = wp.substr(0, delimPos);
		coords[1] = wp.substr(delimPos + 1);
		if(coords[0].empty() || coords[1].empty() || coords[1].find(Constants::BaseElementDelim) != std::string::npos)
			throw std::runtime_error("Bad regular expression: missing coordinate in base element or there are extra delimiters");
		//normalize(coords[0]);
		normalize(coords[1]);
	}
	static constexpr WordPair epsilon() { return {}; }
	static constexpr bool isBegin(Symbol c) noexcept
	{
		return c == Constants::BaseElementBegin;
	}
	static constexpr bool isEnd(Symbol c) noexcept
	{
		return c == Constants::BaseElementEnd;
	}
	auto operator<=>(const WordPair&) const noexcept = default;
	bool operator==(const WordPair&) const noexcept = default;

	SymbolPair operator()(std::size_t ind) const noexcept
	{
		auto proj = [](const Word& w, std::size_t ind) -> Symbol { return ind < w.size() ? w[ind] : Constants::Epsilon; };
		return {proj(coords[0], ind), proj(coords[1], ind)};
	}
};

struct Symbol_Word
{
	Symbol first;
	Word second;
	constexpr Symbol_Word(): first(Constants::Epsilon)/*, second(1, Constants::Epsilon)*/ {}
	Symbol_Word(Symbol first, const Word& second): first(first), second(second) { normalize(this->second); }
	Symbol_Word(Symbol first, Word&& second): first(first), second(std::move(second)) { normalize(this->second); }
	static constexpr Symbol_Word epsilon() { return {}; }
	auto operator<=>(const Symbol_Word&) const noexcept = default;
	bool operator==(const Symbol_Word&) const noexcept = default;
};

inline std::ostream& operator<<(std::ostream& os, const SymbolPair& sp) // [a,b]
{
	return os << Constants::BaseElementBegin << sp.coords[0] << Constants::BaseElementDelim << sp.coords[1] << Constants::BaseElementEnd;
}
inline std::ostream& operator<<(std::ostream& os, const Symbol_Word& sw) // [a,"b"]
{
	return os << Constants::BaseElementBegin << sw.first << Constants::BaseElementDelim << std::quoted(sw.second) << Constants::BaseElementEnd;
}
inline std::ostream & operator<<(std::ostream & os, const WordPair & wp) // ["a","b"]
{
	return os << Constants::BaseElementBegin << std::quoted(wp.coords[0]) << Constants::BaseElementDelim << std::quoted(wp.coords[1]) << Constants::BaseElementEnd;
}

inline std::istream& operator>>(std::istream& is, SymbolOrEpsilon& s) // a
{
	return is >> s.c;
}
inline std::istream& operator>>(std::istream& is, SymbolPair& sp) // [a,b]
{
	char ignore;
	return is >> ignore >> sp.coords[0] >> ignore >> sp.coords[1] >> ignore;
}
inline std::istream& operator>>(std::istream& is, Symbol_Word& sw)
{
	char ignore;
	return is >> ignore >> sw.first >> ignore >> std::quoted(sw.second) >> ignore;
}
inline std::istream& operator>>(std::istream& is, WordPair& wp) // ["a","b"]
{
	char ignore;
	return is >> ignore >> std::quoted(wp.coords[0]) >> ignore >> std::quoted(wp.coords[1]) >> ignore;
}

#endif
