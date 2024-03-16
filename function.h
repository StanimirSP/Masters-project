#ifndef FUNCTION_H
#define FUNCTION_H

#include <vector>
#include <tuple>
#include <utility>
#include <ranges>

template<class Ret, class... Args>
	requires(sizeof...(Args) > 0)
class Function
{
public: //!!!
	using element_type = std::tuple<Ret, Args...>;
	using leftmost_arg_type = std::tuple_element_t<1, element_type>;
	static constexpr std::size_t argsCnt = sizeof...(Args);
	static constexpr std::size_t skip = -2;
	std::vector<element_type> buf;
	std::vector<std::size_t> startInd;

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
	auto binary_search(std::ranges::random_access_range auto in, const element_type& value) const
	{
		if constexpr(I > argsCnt) return in;
		else
		{
			//auto proj = static_cast<const std::tuple_element_t<I, element_type>&(*)(const element_type&)>(std::get<I, Ret, Args...>);
			const std::tuple_element_t<I, element_type>&(*proj)(const element_type&) = std::get<I, Ret, Args...>;
			return binary_search<I + 1>(std::ranges::equal_range(in, std::get<I>(value), {}, proj), value);
		}
	}
public:
	Function() = default;
	Function(const std::vector<element_type>& buf, const std::array<std::size_t, argsCnt>& max_values): Function(auto(buf), max_values) {}
	Function(std::vector<element_type>&& buf, const std::array<std::size_t, argsCnt>& max_values): buf(std::move(buf))
	{
		sort<argsCnt>(max_values);
	}

	const Ret& operator()(const Args&... args) const
	{
		element_type value({}, args...);
		auto range = binary_search<2>(search_first_arg(std::get<1>(value)), value); // startInd gives us O(1) search on the first argument
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
