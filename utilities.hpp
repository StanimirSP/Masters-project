#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <iterator>
#include <utility>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <string>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <tuple>
#include "constants.hpp"

namespace hash_tuple
{
	template<class T>
	struct hash
	{
		std::size_t operator()(const T& value) const
		{
			return std::hash<T>{}(value);
		}
	};

	class hash_combine
	{
		std::size_t seed;
	public:
		hash_combine(std::size_t seed = 0): seed(seed) {}
		template<class T>
		hash_combine& operator,(const T& value)
		{
			// algorithm taken from boost::hash_combine
			seed ^= (hash<T>{}(value)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return *this;
		}
		explicit operator std::size_t() const noexcept
		{
			return seed;
		}
	};

	template<class... Args>
	struct hash<std::tuple<Args...>>
	{
		std::size_t operator()(const std::tuple<Args...>& value) const
		{
			return std::apply([](const auto&... args) {
				return static_cast<std::size_t>((hash_combine{}, ..., args));
			}, value);
		}
	};
}

template<class Map>
inline typename Map::mapped_type value_or(const Map& cont, const typename Map::key_type& key, const typename Map::mapped_type& def)
{
	if(auto it = cont.find(key); it != cont.end())
		return it->second;
	return def;
}

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

template<class ProfileType>
inline std::size_t find_colors_helper(std::vector<State>& color_of, const std::vector<ProfileType>& profile, const std::vector<std::uint32_t>& index_of_state)
{
	color_of.clear();
	color_of.reserve(index_of_state.size());
	std::map<const ProfileType*, State, IndirectlyCompare<>> map_profiles;
	for(std::uint32_t index : index_of_state)
		color_of.push_back(map_profiles.try_emplace(&profile[index], map_profiles.size()).first->second);
	return map_profiles.size();
}

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
	auto old_flags = is.flags();
	is >> std::noskipws >> s.c;
	is.flags(old_flags);
	return is;
}
inline std::istream& operator>>(std::istream& is, SymbolPair& sp) // [a,b]
{
	char ignore;
	auto old_flags = is.flags();
	is >> std::noskipws >> ignore >> sp.coords[0] >> ignore >> sp.coords[1] >> ignore;
	is.flags(old_flags);
	return is;
}
inline std::istream& operator>>(std::istream& is, Symbol_Word& sw)
{
	char ignore;
	auto old_flags = is.flags();
	is >> std::noskipws >> ignore >> sw.first >> ignore >> std::quoted(sw.second) >> ignore;
	is.flags(old_flags);
	return is;
}
inline std::istream& operator>>(std::istream& is, WordPair& wp) // ["a","b"]
{
	char ignore;
	auto old_flags = is.flags();
	is >> std::noskipws >> ignore >> std::quoted(wp.coords[0]) >> ignore >> std::quoted(wp.coords[1]) >> ignore;
	is.flags(old_flags);
	return is;
}

#endif
