#ifndef CLASSICALFSA_HPP
#define CLASSICALFSA_HPP

#include <concepts>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <cstddef>
#include <vector>
#include <string_view>
#include "monoidalFSA.hpp"
#include "transition.hpp"
#include "constants.hpp"

class ClassicalFSA: public MonoidalFSA<SymbolOrEpsilon>
{
	template<bool, class>
	friend class Transducer;
	friend class TSBM_LeftAutomaton;
	friend class TSBM_RightAutomaton;
	friend class TwostepBimachine;
	friend class BimachineWithFinalOutput;

	virtual std::vector<SymbolOrEpsilon> findPseudoAlphabet() const override
	{
		return {alphabet.begin(), alphabet.end()};
	}
	// if epsilonFree is set, then *this must be epsilon free
	std::map<std::set<State>, State> convertToDFSA_ret(bool epsilonFree = false)
	{
		if(!epsilonFree)
			this->removeEpsilon().trim();
		this->transitions.sort(this->statesCnt);
		std::map<std::set<State>, State> newStates;
		std::queue<const std::set<State>*> q;
		newStates[std::set<State>(this->initial.begin(), this->initial.end())] = 0;
		q.push(&newStates.begin()->first);
		this->statesCnt = 1;
		TransitionList<SymbolOrEpsilon> newTransitions;
		std::unordered_set<State> newFinal;
		if(containsFinalState(*q.front()))
			newFinal.insert(0);
		std::unordered_map<Symbol, std::set<State>> nextSets;
		newTransitions.startInd.push_back(0);
		for(State step = 0; !q.empty(); step++)
		{
			auto& currSet = *q.front();
			q.pop();
			for(State st : currSet)
				for(const auto& tr : this->transitions(st))
					nextSets[tr.Label()].insert(tr.To());
			for(Symbol letter : this->alphabet)
			{
				std::set<State>& next = nextSets[letter];
				auto [it, inserted] = newStates.try_emplace(std::move(next), this->statesCnt);
				next.clear();
				if(inserted)
				{
					this->statesCnt++;
					q.push(&it->first);
					if(containsFinalState(it->first))
						newFinal.insert(it->second);
				}
				newTransitions.buffer.emplace_back(step, letter, it->second);
			}
			newTransitions.startInd.push_back(newTransitions.buffer.size());
		}
		newTransitions.isSorted = true;
		this->transitions = std::move(newTransitions);
		this->initial = {0};
		this->final = std::move(newFinal);
		return newStates;
	}
public:
	ClassicalFSA() = default;
	ClassicalFSA(const MonoidalFSA<SymbolOrEpsilon>& mFSA): MonoidalFSA<SymbolOrEpsilon>(mFSA) {}
	ClassicalFSA(MonoidalFSA<SymbolOrEpsilon>&& mFSA): MonoidalFSA<SymbolOrEpsilon>(std::move(mFSA)) {}

	ClassicalFSA& convertToDFSA()
	{
		convertToDFSA_ret();
		return *this;
	}
	ClassicalFSA& complement()
	{
		convertToDFSA();
		std::unordered_set<State> newFinal;
		for(State i = 0; i < this->statesCnt; i++)
			if(!this->final.contains(i))
				newFinal.insert(i);
		this->final = std::move(newFinal);
		return *this;
	}
	ClassicalFSA intersect(ClassicalFSA& rhs)
	{
		convertToDFSA();
		rhs.convertToDFSA();
		auto tranformLabel = [](SymbolOrEpsilon first, SymbolOrEpsilon second) -> SymbolOrEpsilon {
			return first;
			};
		auto labelCondition = [](SymbolOrEpsilon first, SymbolOrEpsilon second) -> bool {
			return first == second && first != Constants::Epsilon;
			};
		return this->product(rhs, tranformLabel, labelCondition);
	}
	static ClassicalFSA createFromSymbolSet(const std::ranges::input_range auto& symbols)
	{
		ClassicalFSA res;
		res.initial.insert(res.statesCnt++);
		for(Symbol c : symbols)
		{
			res.alphabetUnion(c);
			res.transitions.buffer.emplace_back(0, c, res.statesCnt);
			res.final.insert(res.statesCnt++);
		}
		return res;
	}

	// *this must be deterministic, total and the transitions must be sorted by From() in ascending order, then by Label() according to alphabetOrder
	// otherwise the behavior is undefined
	State successor(State from, Symbol with) const
	{
		if(from >= statesCnt)
			throw std::invalid_argument("cannot get successor: state 'from' is out of range");
		auto letterIndexIterator = alphabetOrder.find(with);
		if(letterIndexIterator == alphabetOrder.end())
			throw std::invalid_argument("cannot get successor: '" + std::string{with} + "' is not in the alphabet");
		return (transitions(from).begin() + letterIndexIterator->second)->To();
	}
	// *this must satisfy the precondition for calling 'successor'
	std::vector<State> findPath(const std::ranges::forward_range auto& input) const
	{
		std::vector<State> path;
		path.reserve(input.size() + 1);
		State currSt = *initial.begin();
		path.push_back(currSt);
		for(Symbol s : input)
			path.push_back(currSt = successor(currSt, s));
		return path;
	}
};

#endif
