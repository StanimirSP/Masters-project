#ifndef CLASSICAL_BIMACHINE
#define CLASSICAL_BIMACHINE

#include "twostepBimachine.h"
#include "function.h"
#include "classicalFSA.h"
#include "monoidalFSA.h"
#include <vector>
#include <cstdint>
#include <queue>

class BimachineWithFinalOutput
{
	ClassicalFSA left, right;
	std::vector<std::uint32_t> index_of_left_state, index_of_right_state;
	Function<Word, State, USymbol, State> psi;

	struct LeftState
	{
		State lctx;
		std::map<std::uint32_t, State> phi;

		auto operator<=>(const LeftState&) const = default;
	};

	LeftState initial_left(const TSBM_LeftAutomaton& left, const TSBM_RightAutomaton& right, auto right_classes)
	{
		LeftState init{*left.DFA.initial.begin()};
		for(const auto& [right_state_ptr, right_ind] : right_classes)
			if(State st = TwostepBimachine::nu(right, left.containsFinalOf[init.lctx], *right_state_ptr); st != Constants::InvalidState)
				init.phi.emplace(right_ind, st);
		return init;
	}
	std::pair<State, Word> next_left_helper(const LeftState& from,
											Symbol letter,
											const TSBM_LeftAutomaton& left,
											const TSBM_RightAutomaton& right,
											const auto& right_classes,
											const TSBM_RightAutomaton::State_t& right_state,
											std::uint32_t right_ind,
											State next_lctx,
											const std::vector<ContextualReplacementRuleRepresentation>& batch) const
	{
		// right_state is (R', g')
		const TSBM_RightAutomaton::State_t& succ_right_state = right.successor(right_state, letter); // succ_right_state is (R, g)
		auto phi_of_g_it = from.phi.find(right_classes.find(&succ_right_state)->second);
		if(phi_of_g_it != from.phi.end()) // phi((R, g)) is defined
		{
			if(right.type_of_final_center.contains(phi_of_g_it->second)) // NonemptyMatchFinished(phi, (R, g))
			{
				if(State st = TwostepBimachine::nu(right, left.containsFinalOf[from.lctx], succ_right_state); st != Constants::InvalidState) // NonemptyMatchBegin(L, g)
					return right.calculate_g_of_mu(st, letter, right_state);
				// not NonemptyMatchBegin(L, g)
				if(State st = TwostepBimachine::nu(right, left.containsFinalOf[next_lctx], right_state); st != Constants::InvalidState) // NonemptyMatchBegin(L', g')
				{
					if(std::uint32_t rule = TwostepBimachine::minJ(right, batch, left.containsFinalOf[from.lctx], succ_right_state); rule != Constants::InvalidRule) // EmptyMatchBegin(L, R)
						return {st, *batch[rule].output_for_epsilon + letter};
					return {st, {letter}};
				}
			}
			else if(succ_right_state.g_inv.contains(phi_of_g_it->second)) // NonemptyMatchNotFinished(phi, (R, g))
				return right.calculate_g_of_mu(phi_of_g_it->second, letter, right_state);
		}
		else // phi((R, g)) is not defined, i.e. OutsideOfMatch(phi, (R, g))
			if(State st = TwostepBimachine::nu(right, left.containsFinalOf[next_lctx], right_state); st != Constants::InvalidState) // NonemptyMatchBegin(L', g')
			{
				if(std::uint32_t rule = TwostepBimachine::minJ(right, batch, left.containsFinalOf[from.lctx], succ_right_state); rule != Constants::InvalidRule) // EmptyMatchBegin(L, R)
					return {st, *batch[rule].output_for_epsilon + letter};
				return {st, {letter}};
			}
		return {Constants::InvalidState, {letter}};

	}
	LeftState next_left(State from,
						Symbol letter,
						const Internal::FSA<LeftState>& left,
						const TSBM_LeftAutomaton& leftctx,
						const TSBM_RightAutomaton& right,
						const auto& right_classes,
						const std::vector<ContextualReplacementRuleRepresentation>& batch)
	{
		LeftState next{leftctx.DFA.successor(left.states[from]->lctx, letter)};
		for(const auto& [right_state_ptr, right_ind] : right_classes)
		{
			auto [st, output] = next_left_helper(*left.states[from], letter, leftctx, right, right_classes, *right_state_ptr, right_ind, next.lctx, batch);
			if(st != Constants::InvalidState)
				next.phi.emplace(right_ind, st);
			if(!(output.size() == 1 && output[0] == letter))
				psi.emplace(output, /*index_of_left_state[from.lctx]*/ from, letter, right_ind); // not correct, index_of_left_state must be different
		}
		return next;
	}
public:
	BimachineWithFinalOutput(const std::vector<ContextualReplacementRuleRepresentation>& batch_representation): BimachineWithFinalOutput(auto(batch_representation)) {}
	BimachineWithFinalOutput(std::vector<ContextualReplacementRuleRepresentation>&& batch_representation)
	{
		TSBM_LeftAutomaton leftctx{std::move(batch_representation)};
		std::vector<std::uint32_t> index_of_leftctx_state;
		auto leftctx_classes = leftctx.init_index(index_of_leftctx_state);
		TSBM_RightAutomaton right{std::move(batch_representation)};
		auto right_classes = right.init_index(index_of_right_state);
		right.A_T.transitions.sort();

		Internal::FSA<LeftState> left;
		left.stateNames.emplace(initial_left(leftctx, right, right_classes), 0);
		left.states.push_back(&left.stateNames.begin()->first);
		left.initial.insert(0);
		left.transitions.startInd.push_back(0);
		std::queue<State> q;
		q.push(0);

		LeftState nextLeftStates[std::numeric_limits<USymbol>::max() + 1]; // not good if USymbol is later changed to a larger type
																		   // then this may cause stack overflow
		for(State step = 0; !q.empty(); step++)
		{
			State currState = q.front();
			q.pop();

			for(Symbol letter : leftctx.DFA.alphabet)
			{
				auto [it, inserted] = left.stateNames.try_emplace(next_left(currState, letter, left, leftctx, right, right_classes, batch_representation), left.stateNames.size());
				if(inserted)
				{
					left.states.push_back(&it->first);
					q.push(it->second);
				}
				left.transitions.buffer.emplace_back(step, letter, it->second);
			}
			left.transitions.startInd.push_back(left.transitions.buffer.size());
		}
		left.transitions.isSorted = true;
		left.alphabet = std::move(leftctx.DFA.alphabet);
		left.alphabetOrder = std::move(leftctx.DFA.alphabetOrder);
		this->left = std::move(left).getMFSA();
		this->right = std::move(right.A_R).getMFSA();
		psi.prepare({this->left.statesCnt - 1, std::numeric_limits<std::make_unsigned_t<Symbol>>::max(), right_classes.size() - 1});
	}
	Word operator()(const Word& input) const
	{
		std::vector<State> /*left_path = left.findPath(input),*/ right_path = right.findPath(std::ranges::reverse_view(input));
		//std::ranges::transform(left_path, left_path.begin(), [this](State l) { return index_of_left_state[l]; });
		std::ranges::transform(right_path, right_path.begin(), [this](State r) { return index_of_right_state[r]; });

		Word output;
		State currLeftSt = *left.initial.begin();
		for(auto revPathIt = right_path.rbegin(); Symbol s : input)
		{
			output += psi(currLeftSt, s, *++revPathIt, {s});
			currLeftSt = left.successor(currLeftSt, s);
		}
		return output;
	}
};

#endif
