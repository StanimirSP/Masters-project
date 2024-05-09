#ifndef TRANSDUCER_HPP
#define TRANSDUCER_HPP

#include <concepts>
#include <cstddef>
#include <vector>
#include <iostream>
#include <utility>
#include <type_traits>
#include <unordered_set>
#include <string>
#include "constants.hpp"
#include "monoidalFSA.hpp"
#include "classicalFSA.hpp"
#include "transition.hpp"

template<bool IsLetterType = false, class Label = WordPair>
class Transducer: public MonoidalFSA<std::conditional_t<IsLetterType, SymbolPair, Label>>
{
public:
	using LabelType = std::conditional_t<IsLetterType, SymbolPair, Label>;
	using LetterTransducer = Transducer<true, SymbolPair>;
private:
	template<bool, class>
	friend class Transducer;
	friend class TwostepBimachine;
	friend class BimachineWithFinalOutput;

	[[nodiscard]] ClassicalFSA project(std::invocable<LabelType> auto proj) &&
		requires std::convertible_to<decltype(proj(std::declval<LabelType>())), SymbolOrEpsilon>
	{
		ClassicalFSA pr;
		pr.statesCnt = this->statesCnt;
		pr.initial = std::move(this->initial);
		pr.final = std::move(this->final);
		pr.alphabet = std::move(this->alphabet);
		pr.alphabetOrder = std::move(this->alphabetOrder);
		pr.transitions.isSorted = this->transitions.isSorted;
		pr.transitions.startInd = std::move(this->transitions.startInd);
		for(const auto& tr : this->transitions.buffer)
			pr.transitions.buffer.emplace_back(tr.From(), proj(tr.Label()), tr.To());
		return pr;
	}
	TransitionList<Word> epsilonClosureTape1() const
		requires IsLetterType
	{
		TransitionList<SymbolOrEpsilon> epsTape1;
		TransitionList<Word> closure;
		for(const auto& tr : this->transitions.buffer)
			if(tr.Label().first == Constants::Epsilon)
			{
				epsTape1.buffer.emplace_back(tr.From(), tr.Label().second, tr.To());
				closure.buffer.emplace_back(tr.From(), tr.Label().second == Constants::Epsilon ? Word{} : Word{tr.Label().second}, tr.To());
			}
		epsTape1.sort(this->statesCnt);
		for(std::size_t i = 0; i < closure.buffer.size(); i++)
		{
			const Transition<Word>& curr = closure.buffer[i];
			if(curr.From() == curr.To() && !curr.Label().empty())
				throw std::runtime_error("cannot compute epsilon closure: transducer is infinitely ambiguous");
			for(const auto& tr : epsTape1(curr.To()))
				closure.buffer.emplace_back(curr.From(), curr.Label() + tr.Label().c, tr.To());
		}
		for(State st = 0; st < this->statesCnt; st++)
			closure.buffer.emplace_back(st, Word{}, st);
		return closure;
	}
public:
	Transducer() = default;
	Transducer(const MonoidalFSA<LabelType>& mFSA): MonoidalFSA<LabelType>(mFSA) {}
	Transducer(MonoidalFSA<LabelType>&& mFSA): MonoidalFSA<LabelType>(std::move(mFSA)) {}

	[[nodiscard]] LetterTransducer expand() &&
		requires (!IsLetterType)
	{
		LetterTransducer expanded;
		expanded.statesCnt = this->statesCnt;
		expanded.initial = std::move(this->initial);
		expanded.final = std::move(this->final);
		expanded.alphabet = std::move(this->alphabet);
		expanded.alphabetOrder = std::move(this->alphabetOrder);
		expanded.transitions.buffer.reserve(this->transitions.buffer.size());
		for(const auto& tr : this->transitions.buffer)
		{
			std::size_t maxLen = std::max(tr.Label().first.size(), tr.Label().second.size());
			if(maxLen == 0)
				throw std::runtime_error("bad label");
			State lastState = tr.From();
			for(std::size_t i = 0; i < maxLen - 1; i++)
			{
				expanded.transitions.buffer.emplace_back(lastState, tr.Label()(i), expanded.statesCnt);
				lastState = expanded.statesCnt++;
			}
			expanded.transitions.buffer.emplace_back(lastState, tr.Label()(maxLen - 1), tr.To());
		}
		expanded.transitions.isSorted = false;
		return expanded;
	}
	[[nodiscard]] Transducer<true> expand() const& requires (!IsLetterType) { return Transducer{*this}.expand(); }
	Transducer compose(Transducer& rhs)
		requires IsLetterType
	{
		auto tranformLabel = [](LabelType first, LabelType second) -> LabelType {
			return {first.first, second.second};
			};
		auto labelCondition = [](LabelType first, LabelType second) -> bool {
			return first.second == second.first;
			};
		return this->product(rhs, tranformLabel, labelCondition);
	}
	[[nodiscard]] ClassicalFSA Domain() && requires IsLetterType { return std::move(*this).project([](LabelType lbl) { return lbl.first; }); }
	[[nodiscard]] ClassicalFSA Domain() && requires (!IsLetterType) { return std::move(*this).project([](LabelType lbl) { return lbl.first; }); }
	[[nodiscard]] ClassicalFSA Range() && requires IsLetterType { return std::move(*this).project([](LabelType lbl) { return lbl.second; }); }
	[[nodiscard]] ClassicalFSA Range() && requires (!IsLetterType) { return std::move(*this).project([](LabelType lbl) { return lbl.second; }); }
	[[nodiscard]] ClassicalFSA Domain() const& { return Transducer{*this}.Domain(); }
	[[nodiscard]] ClassicalFSA Range() const& { return Transducer{*this}.Range(); }
	[[nodiscard]] static LetterTransducer identity(ClassicalFSA&& fsa)
		requires IsLetterType
	{
		LetterTransducer id;
		id.statesCnt = std::exchange(fsa.statesCnt, 0);
		id.initial = std::move(fsa.initial);
		id.final = std::move(fsa.final);
		id.alphabet = std::move(fsa.alphabet);
		id.alphabetOrder = std::move(fsa.alphabetOrder);
		if(fsa.transitions.isSorted)
		{
			id.transitions.isSorted = true;
			id.transitions.startInd = std::move(fsa.transitions.startInd);
		}
		for(const auto& tr : fsa.transitions.buffer)
			id.transitions.buffer.emplace_back(tr.From(), LabelType{tr.Label(), tr.Label()}, tr.To());
		return id;
	}
	[[nodiscard]] static LetterTransducer identity(const ClassicalFSA& fsa)
		requires IsLetterType
	{
		return identity(ClassicalFSA{fsa});
	}
	static LetterTransducer createFromSymbolSet(const std::ranges::input_range auto& symbols)
		requires IsLetterType
	{
		LetterTransducer res;
		res.initial.insert(res.statesCnt++);
		for(const auto& pair : symbols)
		{
			if(pair.first != Constants::Epsilon) res.alphabetUnion(pair.first);
			// if(pair.second != Constants::Epsilon) res.alphabetUnion(pair.second);
			res.transitions.buffer.emplace_back(0, pair, res.statesCnt);
			res.final.insert(res.statesCnt++);
		}
		return res;
	}
	[[nodiscard]] Transducer<false, Symbol_Word> realTime(std::unordered_set<Word>* outputsForEpsilon = nullptr) &&
		requires IsLetterType
	{
		Transducer<false, Symbol_Word> rtime;
		rtime.statesCnt = this->statesCnt;
		TransitionList<Word> closure = epsilonClosureTape1(), reversed = closure;
		closure.sort(this->statesCnt);
		reversed.reverse().sort(this->statesCnt);

		// final states and outputs for epsilon
		rtime.final = this->final;
		for(State st : this->initial)
			for(const auto& tr : closure(st))
				if(this->final.contains(tr.To()))
				{
					if(outputsForEpsilon)
						outputsForEpsilon->insert(tr.Label());
					rtime.final.insert(st);
				}

		// transitions
		for(const auto& tr : this->transitions.buffer)
			if(tr.Label().first != Constants::Epsilon)
				for(const auto& trPrev : reversed(tr.From()))
					for(const auto& trNext : closure(tr.To()))
					{
						Word lbl;
						lbl.reserve(trPrev.Label().size() + 1 + trNext.Label().size());
						lbl.append(trPrev.Label()).push_back(tr.Label().second);
						lbl.append(trNext.Label());
						rtime.transitions.buffer.emplace_back(trPrev.To(), Symbol_Word{tr.Label().first, std::move(lbl)}, trNext.To());
					}

		rtime.initial = std::move(this->initial);
		rtime.alphabet = std::move(this->alphabet);
		rtime.alphabetOrder = std::move(this->alphabetOrder);
		return rtime;
	}
	[[nodiscard]] Transducer<false, Symbol_Word> realTime(std::unordered_set<Word>* outputsForEpsilon = nullptr) const&
		requires IsLetterType
	{
		return Transducer{*this}.realTime(outputsForEpsilon);
	}
	bool epsilonInDom()
		requires IsLetterType
	{
		this->transitions.sort(this->statesCnt);
		bool finalReachable = false;
		auto markFinal = [&final = std::as_const(this->final), &finalReachable](State st) { if(final.contains(st)) finalReachable = true; };
		auto isEpsTransitionTape1 = [](const Transition<LabelType>& tr) { return tr.Label().first == Constants::Epsilon; };
		for(auto it = this->initial.begin(); !finalReachable && it != this->initial.end(); ++it)
			this->BFS(*it, markFinal, isEpsTransitionTape1);
		return finalReachable;
	}
};

using LetterTransducer = Transducer<>::LetterTransducer;

#endif
