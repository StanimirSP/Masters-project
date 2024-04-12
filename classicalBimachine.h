#ifndef CLASSICAL_BIMACHINE
#define CLASSICAL_BIMACHINE

#include <vector>
#include <cstdint>
#include <utility>
#include <map>
#include <unordered_map>
#include "twostepBimachine.h"
#include "function.h"
#include "classicalFSA.h"
#include "monoidalFSA.h"
#include "utilities.h"

#if __has_include(<boost/unordered/unordered_flat_map.hpp>) // && __has_include(<boost/functional/hash.hpp>)
#	include <boost/unordered/unordered_flat_map.hpp>
//#	include <boost/functional/hash.hpp>
#	define LIBBOOST_UNORDERED_FLAT_MAP_AVAILABLE
#endif

namespace std
{
	/*template<>
	struct hash<pair<USymbol, State>>
	{
		std::size_t operator()(const pair<USymbol, State>& p) const
		{
			auto seed = std::hash<USymbol>{}(p.first);
			return seed ^= std::hash<State>{}(p.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};*/
}

class BimachineWithFinalOutput
{
	ClassicalFSA left, right;
	Function<Word, State, USymbol, State> psi;
	//std::vector<std::unordered_map<std::pair<USymbol, State>, Word>> psi;
	//boost::unordered_flat_map<std::tuple<State, USymbol, State>, Word> psi;
	std::unordered_map<State, Word> iota;

	struct LeftState
	{
		State lctx;
		std::map<std::uint32_t /* State */, State> phi;

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
						const std::vector<ContextualReplacementRuleRepresentation>& batch,
						const std::vector<std::uint32_t>& index_of_left_state)
	{
		LeftState next{leftctx.DFA.successor(left.states[from]->lctx, letter)};
		for(const auto& [right_state_ptr, right_ind] : right_classes)
		{
			auto [st, output] = next_left_helper(*left.states[from], letter, leftctx, right, right_classes, *right_state_ptr, next.lctx, batch);
			if(st != Constants::InvalidState)
				next.phi.emplace(right_ind, st);
			if(!(output.size() == 1 && output[0] == letter))
			{
				psi.emplace(output, index_of_left_state[from] /* from */, letter, right_ind);
				//psi[{index_of_left_state[from], letter, right_ind}] = output;
			}
		}
		return next;
	}
	std::pair<std::size_t, std::size_t> find_colors(std::vector<State>& color_of_left, std::vector<State>& color_of_right,
													const std::vector<std::vector<State>>& left_states_of_index,
													const std::vector<std::vector<State>>& right_states_of_index,
													const std::vector<std::uint32_t>& index_of_left_state,
													const std::vector<std::uint32_t>& index_of_right_state) const
	{
		using psi_profile_t = std::set<std::tuple<State, Symbol, Word>>;
		using left_profile_t = std::tuple<psi_profile_t, Word>;
		using right_profile_t = psi_profile_t;
		std::vector<left_profile_t> left_profile(left_states_of_index.size());
		std::vector<right_profile_t> right_profile(right_states_of_index.size());
		for(const auto& [ret, left_index, a, right_index] : psi.data())
		//for(const auto& [args, ret] : psi)
		{
			//const auto& [left_index, a, right_index] = args;
			std::get<0>(left_profile[left_index]).emplace(right_index, a, ret);
			right_profile[right_index].emplace(left_index, a, ret);
		}
		for(const auto& [left_index, ret] : iota)
			std::get<1>(left_profile[left_index]) = ret;
		return {
			find_colors_helper(color_of_left, left_profile, index_of_left_state),
			find_colors_helper(color_of_right, right_profile, index_of_right_state)
		};
	}
	void update_functions(const std::vector<State>& color_of_left, const std::vector<State>& color_of_right,
						  const std::vector<std::vector<State>>& left_states_of_index,
						  const std::vector<std::vector<State>>& right_states_of_index)
	{
		{
			decltype(psi) updated;
			for(const auto& [ret, left_index, a, right_index] : psi.data())
			//for(const auto& [args, ret] : psi)
			{
				//const auto& [left_index, a, right_index] = args;
				for(auto [L, R] : std::views::cartesian_product(left_states_of_index[left_index], right_states_of_index[right_index]))
					updated.emplace(ret, color_of_left[L], a, color_of_right[R]);
					//updated[{color_of_left[L], a, color_of_right[R]}] = ret;
			}
			psi = std::move(updated);
		}
		{
			decltype(iota) updated;
			for(const auto& [left_index, ret] : iota)
				for(State L : left_states_of_index[left_index])
					updated[color_of_left[L]] = ret;
			iota = std::move(updated);
		}
	}
	void pseudo_minimize(const std::vector<std::vector<State>>& left_states_of_index,
						 const std::vector<std::vector<State>>& right_states_of_index,
						 const std::vector<std::uint32_t>& index_of_left_state,
						 const std::vector<std::uint32_t>& index_of_right_state)
	{
		std::vector<State> color_of_left, color_of_right;
		auto [colors_left_cnt, colors_right_cnt] = find_colors(color_of_left, color_of_right, left_states_of_index, right_states_of_index, index_of_left_state, index_of_right_state);
		left.coloredPseudoMinimize(colors_left_cnt, color_of_left, left.findPseudoAlphabet());
		right.coloredPseudoMinimize(colors_right_cnt, color_of_right, right.findPseudoAlphabet());
		left.transitions.sort(left.statesCnt); // needed for calling findPath; coloredPseudoMinimize is optimized to leave transitions sorted by Label() according to alphabetOrder as a side effect
		right.transitions.sort(right.statesCnt); // same as above but for the right automaton
		update_functions(color_of_left, color_of_right, left_states_of_index, right_states_of_index);
	}
public:
	BimachineWithFinalOutput(const std::vector<ContextualReplacementRuleRepresentation>& batch): BimachineWithFinalOutput(auto(batch)) {}
	BimachineWithFinalOutput(std::vector<ContextualReplacementRuleRepresentation>&& batch)
	{
		std::vector<std::uint32_t> index_of_left_state, index_of_right_state, index_of_leftctx_state;
		std::vector<std::vector<State>> left_states_of_index, right_states_of_index;
		TSBM_LeftAutomaton leftctx{std::move(batch)};
		TSBM_RightAutomaton right{std::move(batch)};
		leftctx.init_index(index_of_leftctx_state);
		auto right_classes = right.init_index(index_of_right_state, right_states_of_index);

		// needed for calculate_g_of_mu
		sortByLabelDomain(right.A_T.transitions);
		right.A_T.transitions.sort(right.A_T.statesCnt);

		Internal::FSA<LeftState> left;
		left.stateNames.emplace(initial_left(leftctx, right, right_classes), 0);
		left.states.push_back(&left.stateNames.begin()->first);
		left.initial.insert(0);
		left.transitions.startInd.push_back(0);

		LeftState nextLeftStates[std::numeric_limits<USymbol>::max() + 1]; // not good if USymbol is later changed to a larger type
																		   // then this may cause stack overflow
		auto cmp_classes_lctx_then_phi = [&index_of_leftctx_state](const LeftState& a, const LeftState& b) {
			if(auto cmp_cl_lctx = index_of_leftctx_state[a.lctx] <=> index_of_leftctx_state[b.lctx]; cmp_cl_lctx != 0)
				return cmp_cl_lctx < 0;
			return a.phi < b.phi;
			};
		std::map<const LeftState*, std::uint32_t, IndirectlyCompare<decltype(cmp_classes_lctx_then_phi)>> map_left(cmp_classes_lctx_then_phi);
		for(State curr_st_name = 0; curr_st_name < left.states.size(); curr_st_name++)
		{
			const LeftState &curr_st = *left.states[curr_st_name];
			auto [it, inserted] = map_left.try_emplace(&curr_st, map_left.size());
			index_of_left_state.push_back(it->second);
			if(inserted)
				left_states_of_index.emplace_back();
			left_states_of_index[it->second].push_back(curr_st_name);

			for(Symbol letter : leftctx.DFA.alphabet)
			{
				auto [it, inserted] = left.stateNames.try_emplace(next_left(curr_st_name, letter, left, leftctx, right, right_classes, batch, index_of_left_state), left.stateNames.size());
				if(inserted)
					left.states.push_back(&it->first);
				left.transitions.buffer.emplace_back(curr_st_name, letter, it->second);
			}
			left.transitions.startInd.push_back(left.transitions.buffer.size());

			if(std::uint32_t rule = TwostepBimachine::minJ(right, batch, leftctx.containsFinalOf[left.states[curr_st_name]->lctx], *right.A_R.states[*right.A_R.initial.begin()]);
				rule != Constants::InvalidRule && !batch[rule].output_for_epsilon->empty()
			)
				iota[index_of_left_state[curr_st_name]] = *batch[rule].output_for_epsilon;
		}
		left.transitions.isSorted = true;
		left.alphabet = std::move(leftctx.DFA.alphabet);
		left.alphabetOrder = std::move(leftctx.DFA.alphabetOrder);
		this->left = std::move(left).getMFSA();
		this->right = std::move(right.A_R).getMFSA();

		pseudo_minimize(left_states_of_index, right_states_of_index, index_of_left_state, index_of_right_state);
		psi.prepare({this->left.statesCnt - 1, std::numeric_limits<std::make_unsigned_t<Symbol>>::max(), this->right.statesCnt - 1}, true);

		//std::cerr << "size psi: " << psi.data().size() << '\n';

		//debug
		/*this->left.print(std::cerr << "left:\n") << '\n';
		this->right.print(std::cerr << "right:\n") << '\n';
		std::cerr << "size psi: " << psi.data().size() << '\n';
		std::cerr << psi << '\n';
		std::cerr << "size iota: " << iota.size() << '\n';
		for(auto&& [st, output] : iota)
			std::cerr << st << " -> " << output << '\n';*/
	}
	Word operator()(const Word& input) const
	{
		std::vector<State> right_path = right.findPath(std::ranges::reverse_view(input));
		auto right_path_rev_range = right_path | std::views::reverse;

		Word output;
		State curr_left_st = *left.initial.begin();
		for(auto right_path_rev_it = right_path_rev_range.begin(); Symbol s : input)
		{
			/*if(auto it = psi.find({curr_left_st, s, *++right_path_rev_it}); it != psi.end())
				output += it->second;
			else
				output += s;*/
			output += psi(curr_left_st, s, *++right_path_rev_it, {s});
			curr_left_st = left.successor(curr_left_st, s);
		}
		if(auto it = iota.find(curr_left_st); it != iota.end())
			output += it->second;
		return output;
	}
};

#endif
