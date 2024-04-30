#ifndef TWOSTEPBIMACHINE_HPP
#define TWOSTEPBIMACHINE_HPP

#include <vector>
#include <tuple>
#include <utility>
#include <cstdint>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <set>
#include <ranges>
#include <stdexcept>
#include <limits>
#include <concepts>
#include <functional>
#include "classicalFSA.hpp"
#include "contextualReplacementRule.hpp"
#include "utilities.hpp"

#if __has_include(<boost/unordered/unordered_flat_map.hpp>)
#	include <boost/unordered/unordered_flat_map.hpp>
#	define LIBBOOST_UNORDERED_FLAT_MAP_AVAILABLE
#elifndef WARN_LIBBOOST_UNORDERED_FLAT_MAP_NOT_AVAILABLE
#	warning "<boost/unordered/unordered_flat_map.hpp> was not found. Falling back to std::unordered_map. Install libboost for improved performance."
#	define WARN_LIBBOOST_UNORDERED_FLAT_MAP_NOT_AVAILABLE
#endif

struct TSBM_LeftAutomaton
{
	ClassicalFSA DFA;
	std::vector<std::set<std::uint32_t>> containsFinalOf;
private:
	static std::unordered_map<State, std::size_t> mapFinalStates(const std::vector<ContextualReplacementRuleRepresentation>& batch)
	{
		State offset = 0;
		std::unordered_map<State, std::size_t> map;
		for(std::size_t i = 0; i < batch.size(); i++)
		{
			map[offset + *batch[i].left.final.begin()] = i;
			offset += batch[i].left.statesCnt;
		}
		return map;
	}
public:
	TSBM_LeftAutomaton(const std::vector<ContextualReplacementRuleRepresentation>& batch): TSBM_LeftAutomaton(auto(batch)) {}
	TSBM_LeftAutomaton(std::vector<ContextualReplacementRuleRepresentation>&& batch)
	{
		std::unordered_map<State, std::size_t> finalMap = mapFinalStates(batch);
		for(ContextualReplacementRuleRepresentation& crrr : batch)
			DFA = DFA.Union(std::move(crrr.left));

		std::map<std::set<State>, State> det_states = DFA.convertToDFSA_ret(true);
		containsFinalOf.resize(DFA.statesCnt);
		for(auto [subset, st_name] : det_states)
			if(DFA.final.contains(st_name))
				for(State st : subset)
					if(auto it = finalMap.find(st); it != finalMap.end())
						containsFinalOf[st_name].insert(it->second);
		DFA.final.clear(); // no longer needed

		//debug:
		/*DFA.print(std::cerr) << '\n';
		std::cerr << "containsFinalOf[state]=rules:\n";
		for(std::size_t i = 0; i < containsFinalOf.size(); i++)
		{
			std::cerr << "containsFinalOf[" << i << "] = { ";
			for(std::uint32_t r : containsFinalOf[i])
				std::cerr << r << ' ';
			std::cerr << "}\n";
		}*/
	}
	auto init_index(std::vector<std::uint32_t>& index_of_state) const
	{
		index_of_state.clear();
		index_of_state.reserve(containsFinalOf.size());
		using container_type = decltype(containsFinalOf)::value_type;
		std::map<const container_type*, std::uint32_t, IndirectlyCompare<>> map;
		for(std::size_t i = 0; i < containsFinalOf.size(); i++)
		{
			auto [it, inserted] = map.try_emplace(&containsFinalOf[i], map.size());
			index_of_state.push_back(it->second);
		}
		return map;
	}
	auto init_index(std::vector<std::uint32_t>& index_of_state, std::vector<std::vector<State>>& states_of_index) const
	{
		states_of_index.clear();
		index_of_state.clear();
		index_of_state.reserve(containsFinalOf.size());
		using container_type = decltype(containsFinalOf)::value_type;
		std::map<const container_type*, std::uint32_t, IndirectlyCompare<>> map;
		for(std::size_t i = 0; i < containsFinalOf.size(); i++)
		{
			auto [it, inserted] = map.try_emplace(&containsFinalOf[i], map.size());
			index_of_state.push_back(it->second);
			if(inserted)
				states_of_index.emplace_back();
			states_of_index[it->second].push_back(i);
		}
		return map;
	}
};

struct TSBM_RightAutomaton
{
	struct State_t
	{
		std::set<State> R;
		std::vector<State> g,
			g_st; // the subsequence of g, containing exactly the initial states
		std::map<State, std::size_t> g_inv; // the inverse function of g
		std::size_t finals_in_g_begin = 0;

		void clear()
		{
			R.clear();
			g.clear();
			g_st.clear();
			g_inv.clear();
			finals_in_g_begin = 0;
		}
		std::weak_ordering operator<=>(const State_t& rhs) const
		{
			// g_st, g_inv and finals_in_g_begin are helper members which are uniquely determined by g; they don't have to be compared
			if(auto cmp = R <=> rhs.R; cmp != 0) return cmp;
			return g <=> rhs.g;
		}

		friend std::ostream& operator<<(std::ostream& os, const State_t& state)
		{
			os << "\nR: ";
			for(State st : state.R)
				os << st << ' ';
			os << "\ng: ";
			for(State st : state.g)
				os << st << ' ';
			os << "\ng_st: ";
			for(State st : state.g_st)
				os << st << ' ';
			os << "\ng_inv: ";
			for(auto [st, ind] : state.g_inv)
				os << "g(" << ind << ")=" << st << ' ';
			return os << "\nfinals_in_g_begin: " << state.finals_in_g_begin << '\n';
		}
	};

	Internal::FSA<State_t> A_R;
	Transducer<false, Symbol_Word> A_T;
	TransitionList<Symbol_Word> Delta_T;
	std::vector<State> final_center_of_type; // final_center_of_type[r] is the name of the final state of 'batch[r].center_rt' in the union of all 'batch[i].center_rt'
	std::unordered_map<State, std::uint32_t> type_of_init_center, // maps initial states of the union of all 'batch[i].center_rt' to the number of the corresponding replacement rule
		type_of_final_center, // maps final states of the union of all 'batch[i].center_rt' to the number of the corresponding replacement rule
		type_of_final_right_rev; // maps final states of the union of all reversed 'batch[i].right' to the number of the corresponding replacement rule
private:
	Transducer<false, Symbol_Word> construct_A_T(std::vector<ContextualReplacementRuleRepresentation>& batch)
	{
		Transducer<false, Symbol_Word> A_T;
		State offset = 0;
		final_center_of_type.reserve(batch.size());
		for(std::size_t i = 0; i < batch.size(); i++)
		{
			final_center_of_type.push_back(offset + *batch[i].center_rt.final.begin());
			type_of_init_center[offset + *batch[i].center_rt.initial.begin()] = i;
			type_of_final_center[final_center_of_type[i]] = i;
			offset += batch[i].center_rt.statesCnt;
			A_T = A_T.Union(batch[i].center_rt.reverse());
		}
		return A_T;
	}
	ClassicalFSA construct_A_rho(std::vector<ContextualReplacementRuleRepresentation>& batch)
	{
		ClassicalFSA A_rho;
		State offset = 0;
		for(std::size_t i = 0; i < batch.size(); i++)
		{
			A_rho = A_rho.Union(batch[i].right.reverse());
			type_of_final_right_rev[offset + *batch[i].right.final.begin()] = i;
			offset += batch[i].right.statesCnt;
		}
		return A_rho;
	}
	State_t computeInitial_A_R(const ClassicalFSA& A_rho) const
	{
		State_t init;
		for(State st : A_rho.initial)
		{
			init.R.insert(st);
			if(auto it = type_of_final_right_rev.find(st); it != type_of_final_right_rev.end())
				init.g.push_back(final_center_of_type[it->second]);
		}
		std::ranges::sort(init.g);
		for(std::size_t i = 0; i < init.g.size(); i++)
			init.g_inv.emplace_hint(init.g_inv.end(), init.g[i], i);
		return init;
	}
	void addSuccessor_g(State_t& next, State toAdd)
	{
		if(next.g_inv.try_emplace(toAdd, next.g.size()).second)
		{
			next.g.push_back(toAdd);
			next.finals_in_g_begin = next.g.size();
			if(type_of_init_center.contains(toAdd))
				next.g_st.push_back(toAdd);
		}
	}
	void addSuccessor_R(State_t& next, State toAdd)
	{
		if(next.R.insert(toAdd).second)
			if(auto it = type_of_final_right_rev.find(toAdd); it != type_of_final_right_rev.end())
				next.g.push_back(final_center_of_type[it->second]);
	}
	void fixFinal_g(State_t& next)
	{
		std::sort(next.g.begin() + next.finals_in_g_begin, next.g.end()); // sorts the final states in next.g by their type
		for(std::size_t i = next.finals_in_g_begin; i < next.g.size(); i++)
			next.g_inv.emplace(next.g[i], i);	// add the final states in next.g in next.g_inv
	}
public:
	TSBM_RightAutomaton(const std::vector<ContextualReplacementRuleRepresentation>& batch): TSBM_RightAutomaton(auto(batch)) {}
	TSBM_RightAutomaton(std::vector<ContextualReplacementRuleRepresentation>&& batch)
	{
		ClassicalFSA A_rho = construct_A_rho(batch);
		A_T = construct_A_T(batch);
		A_rho.transitions.sort(A_rho.statesCnt);
		A_T.transitions.sortByTo(A_T.statesCnt); // may reduce the side of the constructed automaton
		A_T.transitions.sort(A_T.statesCnt);

		A_R.stateNames.emplace(computeInitial_A_R(A_rho), 0);
		A_R.states.push_back(&A_R.stateNames.begin()->first);
		A_R.initial.insert(0);
		A_R.transitions.startInd.push_back(0);
		std::queue<const State_t*> q;
		q.push(A_R.states.back());

		std::unordered_map<Symbol, State_t> nextStates;
		for(State step = 0; !q.empty(); step++)
		{
			const State_t& currState = *q.front();
			q.pop();

			for(State st : currState.g)
				for(const auto& tr : A_T.transitions(st))
					addSuccessor_g(nextStates[tr.Label().first], tr.To());

			for(State st : currState.R)
				for(const auto& tr : A_rho.transitions(st))
					addSuccessor_R(nextStates[tr.Label()], tr.To());

			for(USymbol letter : A_rho.alphabet)
			{
				State_t& next = nextStates[letter];
				fixFinal_g(next);

				auto [it, inserted] = A_R.stateNames.try_emplace(std::move(next), A_R.stateNames.size());
				next.clear();
				if(inserted)
				{
					A_R.states.push_back(&it->first);
					q.push(A_R.states.back());
				}
				A_R.transitions.buffer.emplace_back(step, letter, it->second);
			}
			A_R.transitions.startInd.push_back(A_R.transitions.buffer.size());
		}
		A_R.transitions.isSorted = true;

		A_R.alphabet = std::move(A_rho.alphabet);
		A_R.alphabetOrder = std::move(A_rho.alphabetOrder);
		A_T.reverse(); // restore the original direction

		// debug:
		/*A_rho.print(std::cerr << "\nA_rho:\n") << '\n';
		A_T.print(std::cerr << "\nA_T:\n") << '\n';
		A_R.print(std::cerr << "\nA_R:\n") << '\n';
		std::cerr << "final_center_of_type (type, state): ";
		for(std::size_t i = 0; i < final_center_of_type.size(); i++)
			std::cerr << '(' << i << ", " << final_center_of_type[i] << ") ";
		std::cerr << "\ntype_of_init_center: ";
		for(auto [st, type] : type_of_init_center)
			std::cerr << "type(" << st << ")=" << type << " ";
		std::cerr << "\ntype_of_final_center: ";
		for(auto [st, type] : type_of_final_center)
			std::cerr << "type(" << st << ")=" << type << " ";
		std::cerr << "\ntype_of_final_right_rev: ";
		for(auto [st, type] : type_of_final_right_rev)
			std::cerr << "type(" << st << ")=" << type << " ";
		std::cerr << "\n";*/
	}
	auto init_index(std::vector<std::uint32_t>& index_of_state, std::vector<std::vector<State>>& states_of_index) const
	{
		states_of_index.clear();
		index_of_state.clear();
		index_of_state.resize(A_R.stateNames.size());
		auto cmp_only_g = [](const State_t& a, const State_t& b) { return a.g < b.g; };
		std::map<const State_t*, std::uint32_t, IndirectlyCompare<decltype(cmp_only_g)>> map(cmp_only_g);
		for(const auto& [st, st_name] : A_R.stateNames)
		{
			auto [it, inserted] = map.try_emplace(&st, map.size());
			index_of_state[st_name] = it->second;
			if(inserted)
				states_of_index.emplace_back();
			states_of_index[it->second].push_back(st_name);
		}
		return map;
	}

	// used by TwostepBimachine
	std::vector<Word>& calculate_mu(std::vector<std::size_t>& buf_mu,
									std::vector<Word>& buf_outputs,
									State q,
									const TSBM_RightAutomaton::State_t& right_state) const
	{
		std::ranges::fill(buf_mu, std::numeric_limits<std::size_t>::max());
		for(const auto& tr : A_T.transitions(q))
		{
			std::uint32_t letter_ind = A_R.alphabetOrder.find(tr.Label().first)->second; // tr.Label().first must be in the alphabet, no need to check
			if(auto it = right_state.g_inv.find(tr.To()); it != right_state.g_inv.end())
			{
				std::size_t ind_in_g = it->second;
				if(buf_mu[letter_ind] > ind_in_g)
				{
					buf_mu[letter_ind] = ind_in_g;
					buf_outputs[letter_ind] = tr.Label().second;
				}
			}
		}
		return buf_outputs;
	}

	// used by BimachineWithFinalOutput
	std::pair<State, Word> calculate_g_of_mu(State q, Symbol letter, const TSBM_RightAutomaton::State_t& right_state) const
	{
		std::size_t mu = std::numeric_limits<std::size_t>::max();
		Word output;
		for(const auto& tr : std::ranges::equal_range(A_T.transitions(q), letter, {}, [](const Transition<Symbol_Word>& tr) { return tr.Label().first; }))
			if(auto it = right_state.g_inv.find(tr.To()); it != right_state.g_inv.end())
			{
				std::size_t ind_in_g = it->second;
				if(mu > ind_in_g)
				{
					mu = ind_in_g;
					output = tr.Label().second;
				}
			}
		return {right_state.g[mu], std::move(output)};
	}
	const State_t& successor(const State_t& from, Symbol with) const
	{
		auto from_it = A_R.stateNames.find(from);
		if(from_it == A_R.stateNames.end())
			throw std::out_of_range("cannot get successor: the state 'from' not found");
		auto letterIndexIterator = A_R.alphabetOrder.find(with);
		if(letterIndexIterator == A_R.alphabetOrder.end())
			throw std::runtime_error("cannot get successor: '" + std::string{with} + "' is not in the alphabet");
		return *A_R.states[(A_R.transitions(from_it->second).begin() + letterIndexIterator->second)->To()];
	}
};

class TwostepBimachine
{
	ClassicalFSA left, right;
#ifdef LIBBOOST_UNORDERED_FLAT_MAP_AVAILABLE
	boost::unordered_flat_map<std::tuple<State, USymbol, State>, State> delta;
	boost::unordered_flat_map<std::tuple<State, USymbol, State>, Word> psi_delta;
	boost::unordered_flat_map<std::tuple<State, State>, State> tau;
	boost::unordered_flat_map<std::tuple<State, State>, Word> psi_tau;
#else
	std::unordered_map<std::tuple<State, Symbol, State>, State, hash_tuple::hash<std::tuple<State, Symbol, State>>> delta;
	std::unordered_map<std::tuple<State, Symbol, State>, Word, hash_tuple::hash<std::tuple<State, Symbol, State>>> psi_delta;
	std::unordered_map<std::tuple<State, State>, State, hash_tuple::hash<std::tuple<State, State>>> tau;
	std::unordered_map<std::tuple<State, State>, Word, hash_tuple::hash<std::tuple<State, State>>> psi_tau;
#endif
	State q_err;
	std::unordered_set<State> final_center;

	void construct_functions(const TSBM_LeftAutomaton& left, const TSBM_RightAutomaton& right,
							 const auto& left_classes, const auto& right_classes,
							 const std::vector<ContextualReplacementRuleRepresentation>& batch)
	{
		// defined outside of the loops to avoid reallocations
		std::vector<std::size_t> mu(right.A_R.alphabet.size());
		std::vector<Word> outputs(right.A_R.alphabet.size());

		for(const auto& [right_state_ptr, right_ind] : right_classes)
		{
			for(State q = 0; q < right.A_T.statesCnt; q++)
			{
				right.calculate_mu(mu, outputs, q, *right_state_ptr);
				for(std::size_t letter_ind = 0; letter_ind < mu.size(); letter_ind++)
					if(mu[letter_ind] != std::numeric_limits<std::size_t>::max())
					{
						delta[{q, right.A_R.alphabet[letter_ind], right_ind}] = right_state_ptr->g[mu[letter_ind]];
						if(!(outputs[letter_ind].size() == 1 && outputs[letter_ind][0] == right.A_R.alphabet[letter_ind])) // do not insert elements which represent identity on the letter to optimize psi_delta for size
							psi_delta[{q, right.A_R.alphabet[letter_ind], right_ind}] = outputs[letter_ind];
					}
			}

			for(const auto& [rules_left_ctx_ok_ptr, left_ind] : left_classes)
			{
				if(State init = nu(right, *rules_left_ctx_ok_ptr, *right_state_ptr); init != Constants::InvalidState)
					tau[{left_ind, right_ind}] = init;
				if(std::uint32_t rule = minJ(right, batch, *rules_left_ctx_ok_ptr, *right_state_ptr);
					rule != Constants::InvalidRule &&
					!batch[rule].output_for_epsilon->empty() // do not insert elements which represent empty output to optimize psi_tau for size
				)
					psi_tau[{left_ind, right_ind}] = *batch[rule].output_for_epsilon;
			}
		}
	}
	std::pair<std::size_t, std::size_t> find_colors(std::vector<State>& color_of_left, std::vector<State>& color_of_right,
													const std::vector<std::vector<State>>& left_states_of_index,
													const std::vector<std::vector<State>>& right_states_of_index,
													const std::vector<std::uint32_t>& index_of_left_state,
													const std::vector<std::uint32_t>& index_of_right_state) const
	{
		using delta_profile_t = std::set<std::tuple<State, Symbol, State>>; // set of (q, a, delta(q, a, R))
		using psi_delta_profile_t = std::set<std::tuple<State, Symbol, Word>>;
		using tau_profile_t = std::set<std::tuple<State, State>>; // set of (L, tau(L, R)) or set of (R, tau(L, R))
		using psi_tau_profile_t = std::set<std::tuple<State, Word>>;
		using left_profile_t = std::tuple<tau_profile_t, psi_tau_profile_t>;
		using right_profile_t = std::tuple<tau_profile_t, psi_tau_profile_t, delta_profile_t, psi_delta_profile_t>;
		std::vector<left_profile_t> left_profile(left_states_of_index.size());
		std::vector<right_profile_t> right_profile(right_states_of_index.size());

		for(const auto& [args, ret] : delta)
		{
			const auto& [q, a, right_index] = args;
			std::get<2>(right_profile[right_index]).emplace(q, a, ret);
		}

		for(const auto& [args, ret] : psi_delta)
		{
			const auto& [q, a, right_index] = args;
			std::get<3>(right_profile[right_index]).emplace(q, a, ret);
		}

		for(const auto& [args, ret] : tau)
		{
			const auto& [left_index, right_index] = args;
			std::get<0>(right_profile[right_index]).emplace(left_index, ret);
			std::get<0>(left_profile[left_index]).emplace(right_index, ret);
		}

		for(const auto& [args, ret] : psi_tau)
		{
			const auto& [left_index, right_index] = args;
			std::get<1>(right_profile[right_index]).emplace(left_index, ret);
			std::get<1>(left_profile[left_index]).emplace(right_index, ret);
		}

		return {
			find_colors_helper(color_of_left, left_profile, index_of_left_state),
			find_colors_helper(color_of_right, right_profile, index_of_right_state)
		};
	}
	void update_functions(const std::vector<State>& color_of_left, const std::vector<State>& color_of_right,
						  const std::vector<std::vector<State>>& left_states_of_index,
						  const std::vector<std::vector<State>>& right_states_of_index)
	{
		auto update_deltalike = [&right_states_of_index, &color_of_right]<class Function>(Function & fun)
		{
			Function updated;
			for(const auto& [args, ret] : fun)
			{
				const auto& [q, a, right_index] = args;
				for(State R : right_states_of_index[right_index])
					updated[{q, a, color_of_right[R]}] = ret;
			}
			fun = std::move(updated);
		};
		update_deltalike(delta);
		update_deltalike(psi_delta);

		auto update_taulike = [&right_states_of_index, &color_of_right, &left_states_of_index, &color_of_left]<class Function>(Function & fun)
		{
			Function updated;
			for(const auto& [args, ret] : fun)
			{
				const auto& [left_index, right_index] = args;
				for(auto [L, R] : std::views::cartesian_product(left_states_of_index[left_index], right_states_of_index[right_index]))
					updated[{color_of_left[L], color_of_right[R]}] = ret;
			}
			fun = std::move(updated);
		};
		update_taulike(tau);
		update_taulike(psi_tau);
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
	static State nu(const TSBM_RightAutomaton & right, const std::ranges::forward_range auto & rules_left_ctx_ok, const TSBM_RightAutomaton::State_t & right_state)
	{
		for(State init : right_state.g_st)
		{
			std::uint32_t rule = right.type_of_init_center.find(init)->second; // (init, type(init)) must be in type_of_init_center, no need to check
			if(rules_left_ctx_ok.contains(rule))
				return init;
		}
		return Constants::InvalidState;
	}
	static std::uint32_t minJ(const TSBM_RightAutomaton& right,
							  const std::vector<ContextualReplacementRuleRepresentation>& batch,
							  const std::ranges::forward_range auto& rules_left_ctx_ok,
							  const TSBM_RightAutomaton::State_t& right_state)
	{
		for(State fin : right_state.g | std::views::drop(right_state.finals_in_g_begin))
		{
			std::uint32_t rule = right.type_of_final_center.find(fin)->second; // (fin, type(fin)) must be in type_of_final_center, no need to check
			if(batch[rule].output_for_epsilon && rules_left_ctx_ok.contains(rule))
				return rule; // final states in right_state.g are sorted by rule in ascending order
		}
		return Constants::InvalidRule;
	}
	TwostepBimachine(const std::vector<ContextualReplacementRuleRepresentation>& batch): TwostepBimachine(auto(batch)) {}
	TwostepBimachine(std::vector<ContextualReplacementRuleRepresentation>&& batch)
	{
		std::vector<std::uint32_t> index_of_left_state, index_of_right_state;
		std::vector<std::vector<State>> left_states_of_index, right_states_of_index;
		{
			TSBM_LeftAutomaton left{std::move(batch)};
			TSBM_RightAutomaton right{std::move(batch)};
			auto left_classes = left.init_index(index_of_left_state, left_states_of_index);
			auto right_classes = right.init_index(index_of_right_state, right_states_of_index);

			q_err = right.A_T.statesCnt;
			right.A_T.transitions.sort(right.A_T.statesCnt); // needed for calling calculate_mu
			construct_functions(left, right, left_classes, right_classes, batch);
			for(const auto& [st, _] : right.type_of_final_center)
				final_center.insert(st);
			this->left = std::move(left.DFA);
			this->right = std::move(right.A_R).getMFSA();
		}
		pseudo_minimize(left_states_of_index, right_states_of_index, index_of_left_state, index_of_right_state);

		//debug
		//std::cerr << "\t\tleft states: " << this->left.statesCnt << '\n';
		//std::cerr << "\t\tleft transitions: " << this->left.transitions.buffer.size() << '\n';
		//std::cerr << "\t\tright states: " << this->right.statesCnt << '\n';
		//std::cerr << "\t\tright transitions: " << this->right.transitions.buffer.size() << '\n';
		//std::cerr << "\t\tsize delta: " << delta.size() << '\n';
		//std::cerr << "\t\tsize psi_delta: " << psi_delta.size() << '\n';
		//std::cerr << "\t\tsize tau: " << tau.size() << '\n';
		//std::cerr << "\t\tsize psi_tau: " << psi_tau.size() << '\n';
		//std::cerr << "\t\tsize final_center: " << final_center.size() << '\n';
		/*this->left.print(std::cerr << "left:\n") << '\n';
		this->right.print(std::cerr << "right:\n") << '\n';*/
	}
	Word operator()(const Word& input) const
	{
		std::vector<State> left_path = left.findPath(input), right_path = right.findPath(std::views::reverse(input));
		auto left_path_it = left_path.begin();
		auto right_path_rev_it = right_path.rbegin();

		Word output;
		State curr/* = q_err*/;
		curr = value_or(tau, {*left_path_it, *right_path_rev_it}, q_err);
		if(curr == q_err)
			if(auto it = psi_tau.find({*left_path_it++, *right_path_rev_it++}); it != psi_tau.end())
				output += it->second;
		for(Symbol s : input)
		{
			State next = value_or(delta, {curr, s, *right_path_rev_it}, q_err);
			output += value_or(psi_delta, {curr, s, *right_path_rev_it}, {s});
			if(next == q_err || final_center.contains(next))
			{
				curr = value_or(tau, {*left_path_it, *right_path_rev_it}, q_err);
				if(curr == q_err)
					if(auto it = psi_tau.find({*left_path_it, *right_path_rev_it}); it != psi_tau.end())
						output += it->second;
			}
			else
				curr = next;
			++left_path_it;
			++right_path_rev_it;
		}
		return output;
	}
};

#endif
