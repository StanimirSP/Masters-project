#ifndef CLASSICAL_BIMACHINE
#define CLASSICAL_BIMACHINE

#include <vector>
#include <cstdint>
#include <queue>
#include "twostepBimachine.h"
#include "function.h"
#include "classicalFSA.h"
#include "monoidalFSA.h"
#include "utilities.h"

class BimachineWithFinalOutput
{
	ClassicalFSA left, right;
	std::vector<std::uint32_t> index_of_left_state, index_of_right_state, index_of_leftctx_state;
	Function<Word, State, USymbol, State> psi;
	std::vector<Word> iota;

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
				std::uint32_t rule = TwostepBimachine::minJ(right, batch, left.containsFinalOf[from.lctx], succ_right_state);
				return {
					TwostepBimachine::nu(right, left.containsFinalOf[next_lctx], right_state),
					(rule != Constants::InvalidRule ? *batch[rule].output_for_epsilon : Word{}) + letter
				};
			}
			else if(succ_right_state.g_inv.contains(phi_of_g_it->second)) // NonemptyMatchNotFinished(phi, (R, g))
				return right.calculate_g_of_mu(phi_of_g_it->second, letter, right_state);
		}
		else // phi((R, g)) is not defined, i.e. OutsideOfMatch(phi, (R, g))
			if(State st = TwostepBimachine::nu(right, left.containsFinalOf[from.lctx], succ_right_state); st == Constants::InvalidState) // not NonemptyMatchBegin(L, g) 
			{
				std::uint32_t rule = TwostepBimachine::minJ(right, batch, left.containsFinalOf[from.lctx], succ_right_state);
				return {
					TwostepBimachine::nu(right, left.containsFinalOf[next_lctx], right_state),
					(rule != Constants::InvalidRule ? *batch[rule].output_for_epsilon : Word{}) + letter
				};
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
			auto [st, output] = next_left_helper(*left.states[from], letter, leftctx, right, right_classes, *right_state_ptr, next.lctx, batch);
			if(st != Constants::InvalidState)
				next.phi.emplace(right_ind, st);
			if(!(output.size() == 1 && output[0] == letter))
				psi.emplace(output, index_of_left_state[from], letter, right_ind);
		}
		return next;
	}

public:
	BimachineWithFinalOutput(const std::vector<ContextualReplacementRuleRepresentation>& batch): BimachineWithFinalOutput(auto(batch)) {}
	BimachineWithFinalOutput(std::vector<ContextualReplacementRuleRepresentation>&& batch)
	{
		TSBM_LeftAutomaton leftctx{std::move(batch)};
		std::vector<std::uint32_t> index_of_leftctx_state;
		auto leftctx_classes = leftctx.init_index(index_of_leftctx_state);
		TSBM_RightAutomaton right{std::move(batch)};
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
		auto cmp_classes_lctx_then_phi = [&index_of_leftctx_state](const LeftState& a, const LeftState& b) {
			if(auto cmp_cl_lctx = index_of_leftctx_state[a.lctx] <=> index_of_leftctx_state[b.lctx]; cmp_cl_lctx != 0)
				return cmp_cl_lctx < 0;
			return a.phi < b.phi;
			};
		std::map<const LeftState*, std::uint32_t, IndirectlyCompare<decltype(cmp_classes_lctx_then_phi)>> map_left(cmp_classes_lctx_then_phi);
		for(State step = 0; !q.empty(); step++)
		{
			State curr_st_name = q.front();
			q.pop();
			const LeftState &curr_st = *left.states[curr_st_name];
			index_of_left_state.push_back(map_left.try_emplace(&curr_st, map_left.size()).first->second);
			this->index_of_leftctx_state.push_back(index_of_leftctx_state[curr_st.lctx]);

			for(Symbol letter : leftctx.DFA.alphabet)
			{
				auto [it, inserted] = left.stateNames.try_emplace(next_left(curr_st_name, letter, left, leftctx, right, right_classes, batch), left.stateNames.size());
				if(inserted)
				{
					left.states.push_back(&it->first);
					q.push(it->second);
				}
				left.transitions.buffer.emplace_back(step, letter, it->second);
			}
			left.transitions.startInd.push_back(left.transitions.buffer.size());

			std::uint32_t rule = TwostepBimachine::minJ(right, batch, leftctx.containsFinalOf[left.states[curr_st_name]->lctx], *right.A_R.states[*right.A_R.initial.begin()]);
			if(rule != Constants::InvalidRule)
			{
				std::uint32_t leftctx_ind = this->index_of_leftctx_state[curr_st_name];
				if(iota.size() <= leftctx_ind)
					iota.resize(leftctx_ind + 1);
				iota[leftctx_ind] = *batch[rule].output_for_epsilon;
			}
		}
		left.transitions.isSorted = true;
		left.alphabet = std::move(leftctx.DFA.alphabet);
		left.alphabetOrder = std::move(leftctx.DFA.alphabetOrder);
		this->left = std::move(left).getMFSA();
		this->right = std::move(right.A_R).getMFSA();
		psi.prepare({this->left.statesCnt - 1, std::numeric_limits<std::make_unsigned_t<Symbol>>::max(), right_classes.size() - 1}, true);

		//debug
		/*std::cerr << "size psi: " << psi.buf.size() << '\n';
		for(auto t : psi.buf)
			std::cerr << std::get<0>(t) << ' ' << std::get<1>(t) << ' ' << std::get<2>(t) << ' ' << std::get<3>(t) << '\n';*/
		/*std::cerr << "size iota: " << iota.size() << '\n';
		for(auto t : iota)
			std::cerr << t << '\n';*/
	}
	Word operator()(const Word& input) const
	{
		std::vector<State> right_path = right.findPath(std::ranges::reverse_view(input));
		auto right_path_rev_range = right_path | std::ranges::views::reverse | std::ranges::views::transform([this](State r) { return index_of_right_state[r]; });

		Word output;
		State curr_left_st = *left.initial.begin();
		for(auto right_path_rev_it = right_path_rev_range.begin(); Symbol s : input)
		{
			output += psi(index_of_left_state[curr_left_st], s, *++right_path_rev_it, {s});
			curr_left_st = left.successor(curr_left_st, s);
		}
		output += iota[this->index_of_leftctx_state[curr_left_st]];
		return output;
	}
};

#endif
