#ifndef FUNCTION_H
#define FUNCTION_H

#include <vector>
#include <tuple>
#include <utility>
#include <ranges>
#include <type_traits>
#include <array>
#include <algorithm>

template<class Ret, class... Args>
	requires((sizeof...(Args) > 0) && ... && std::is_unsigned_v<Args>)
class Function
{
public: //!!!
	using element_type = std::tuple<Ret, Args...>;
	using args_tuple = std::tuple<Args...>;
	using leftmost_arg_type = std::tuple_element_t<0, args_tuple>;
	static constexpr std::size_t argsCnt = sizeof...(Args);
	static constexpr std::size_t skip = -2;
	std::vector<element_type> buf;
	std::vector<std::size_t> startInd;
	bool prepared = false;

	std::span<const element_type> search_first_arg(const leftmost_arg_type& arg) const
	{
		if(arg + 1 >= startInd.size())
			throw std::runtime_error("bad call to Function::operator()(const leftmost_arg_type& arg)");
		return {buf.begin() + startInd[arg], buf.begin() + startInd[arg + 1]};
	}
	template<std::size_t I>
	void sort(std::size_t maxValue)
	{
		if(maxValue == skip) return;
		count<I>(maxValue);
		for(std::size_t i = 1; i < startInd.size(); i++)
			startInd[i] += startInd[i - 1];
		std::vector<element_type> sorted(buf.size());
		for(element_type& el : buf | std::views::reverse)
			sorted[--startInd[std::get<I>(el)]] = std::move(el);
		buf = std::move(sorted);
	}
	template<std::size_t I>
	void count(std::size_t maxValue)
	{
		startInd.resize(maxValue + 2);
		std::ranges::fill(startInd, 0);
		for(const element_type& el : buf)
			startInd[std::get<I>(el)]++;
	}
	template<std::size_t N>
	void sort(const std::array<std::size_t, argsCnt>& max_values)
	{
		if constexpr(N)
		{
			sort<N>(max_values[N - 1]);
			sort<N - 1>(max_values);
		}
	}
	template<std::size_t I>
	auto binary_search(std::ranges::random_access_range auto in, const args_tuple& value) const
	{
		if constexpr(I > argsCnt) return in;
		else
		{
			const std::tuple_element_t<I, element_type>&(*proj)(const element_type&) = std::get<I, Ret, Args...>;
			return binary_search<I + 1>(std::ranges::equal_range(in, std::get<I - 1>(value), {}, proj), value);
		}
	}
public:
	void emplace(const Ret& ret, const Args&... args)
	{
		buf.emplace_back(ret, args...);
		prepared = false;
	}
	void prepare(const std::array<std::size_t, argsCnt>& max_values, bool erase_duplicates = false)
	{
		sort<argsCnt>(max_values);
		if(erase_duplicates)
			buf.erase(std::ranges::unique(buf).begin(), buf.end());
		sort<1>(max_values[0]);
		prepared = true;
	}
	const Ret& operator()(const Args&... args) const
	{
		args_tuple value(args...);
		auto range = binary_search<2>(search_first_arg(std::get<0>(value)), value); // startInd gives us O(1) search on the first argument
																					// for the rest arguments binary search is used
		if(std::ranges::size(range) < 1)
			throw std::out_of_range("function not defined");
		if(std::ranges::size(range) > 1)
			throw std::runtime_error("function ambiguously defined");
		return std::get<0>(*std::ranges::begin(range));
	}
	const Ret& operator()(const Args&... args, const Ret& def) const
	{
		try
		{
			return (*this)(args...);
		}
		catch(const std::out_of_range&)
		{
			return def;
		}
	}
};

#endif
