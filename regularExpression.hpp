#ifndef REGULAREXPRESSION_HPP
#define REGULAREXPRESSION_HPP

#include <cstddef>
#include <stdexcept>
#include <stack>
#include <utility>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include "constants.hpp"
#include "utilities.hpp"

#define NDEBUG

template<class BaseElement>
class RegularExpression
{
#ifndef NDEBUG
	std::string tokenizedRegex;
#endif
	std::string tokenizedRPN;
	std::vector<BaseElement> baseTokens;
	std::string tokenize(const std::string& regex)
	{
		std::string tokenizedRegex;
		tokenizedRegex.reserve(regex.size());
		for(std::size_t i = 0; i < regex.size(); i++)
		{
			if((BaseElement::isBegin(regex[i]) || regex[i] == Constants::EmptySet || regex[i] == Constants::OpenParenthesis) &&
			   i && regex[i - 1] != Constants::OpenParenthesis && regex[i - 1] != Constants::Union)
				tokenizedRegex.push_back(Constants::Concatenation);
			if(Constants::isOperator(regex[i]) || Constants::isParenthesis(regex[i]) || regex[i] == Constants::EmptySet)
				tokenizedRegex.push_back(regex[i]);
			else if(BaseElement::isBegin(regex[i]))
				if(auto baseStart = regex.begin() + i, baseEnd = std::find_if(baseStart, regex.end(), BaseElement::isEnd); baseEnd != regex.end())
				{
					baseTokens.emplace_back(std::string_view{baseStart, baseEnd + 1});
					tokenizedRegex.push_back(Constants::BasePlaceholder);
					i = std::distance(regex.begin(), baseEnd);
				}
				else throw std::runtime_error("Bad regular expression: unclosed base element");
			else throw std::runtime_error("Bad regular expression: unexpected character at position " + std::to_string(i));
		}
		return tokenizedRegex;
	}
	void produceRPN(const std::string& tokenizedRegex)
	{
		tokenizedRPN.reserve(tokenizedRegex.size());
		std::stack<Symbol> op;
		for(Symbol c : tokenizedRegex)
		{
			if(c == Constants::BasePlaceholder || c == Constants::EmptySet)
				tokenizedRPN.push_back(c);
			else if(Constants::isOperator(c))
			{
				while(!op.empty() && op.top() != Constants::OpenParenthesis && Constants::precedence(op.top()) >= Constants::precedence(c))
				{
					tokenizedRPN.push_back(op.top());
					op.pop();
				}
				op.push(c);
			}
			else if(c == Constants::OpenParenthesis) op.push(c);
			else if(c == Constants::CloseParenthesis)
			{
				while(!op.empty() && op.top() != Constants::OpenParenthesis)
				{
					tokenizedRPN.push_back(op.top());
					op.pop();
				}
				if(op.empty()) throw std::runtime_error("Bad regular expression: mismatched parentheses");
				op.pop();
			}
		}
		while(!op.empty())
		{
			if(op.top() == Constants::OpenParenthesis) throw std::runtime_error("Bad regular expression: mismatched parentheses");
			tokenizedRPN.push_back(op.top());
			op.pop();
		}
	}
public:
	RegularExpression(): RegularExpression(std::string(1, Constants::EmptySet)) {}
	RegularExpression(const std::string& regex)
	{
		if(regex.empty())
			throw std::runtime_error("Empty regular expression");
#	ifndef NDEBUG
		tokenizedRegex = tokenize(regex);
		produceRPN(tokenizedRegex);
#	else
		produceRPN(tokenize(regex));
#	endif
	}
#ifndef NDEBUG
	const std::string& TokenizedRegex() const noexcept { return tokenizedRegex; }
#endif
	const std::string& TokenizedReversePolishNotation() const noexcept { return tokenizedRPN; }
	const std::vector<BaseElement>& BaseTokens() const noexcept { return baseTokens; }
};

#endif
