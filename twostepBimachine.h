#ifndef TWOSTEPBIMACHINE_H
#define TWOSTEPBIMACHINE_H

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
#include "classicalFSA.h"
#include "contextualReplacementRule.h"
#include "function.h"
#include "utilities.h"

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
		DFA.print(std::cerr) << '\n';
		std::cerr << "containsFinalOf[state]=rules:\n";
		for(std::size_t i = 0; i < containsFinalOf.size(); i++)
		{
			std::cerr << "containsFinalOf[" << i << "] = { ";
			for(std::uint32_t r : containsFinalOf[i])
				std::cerr << r << ' ';
			std::cerr << "}\n";
		}
	}
	auto init_index(std::vector<std::uint32_t>& index_of_left_state) const
	{
		using container_type = decltype(containsFinalOf)::value_type;
		//auto cmp = [](const container_type* a, const container_type* b) { return *a < *b; };
		std::map<const container_type*, std::uint32_t, IndirectlyCompare<>> map;
		index_of_left_state.reserve(containsFinalOf.size());
		for(std::size_t i = 0; i < containsFinalOf.size(); i++)
			index_of_left_state.push_back(map.try_emplace(&containsFinalOf[i], map.size()).first->second);
		return map;
	}
	/*State successor(State from, Symbol with) const
	{
		if(from >= DFA.statesCnt)
			throw std::out_of_range("cannot get successor: state 'from' is out of range");
		auto letterIndexIterator = DFA.alphabetOrder.find(with);
		if(letterIndexIterator == DFA.alphabetOrder.end())
			throw std::runtime_error("cannot get successor: '" + std::string{with} + "' is not in the alphabet");
		return (DFA.transitions(from).begin() + letterIndexIterator->second)->To();
	}*/
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
		A_rho.transitions.sort();
		A_T.transitions.sortByTo();
		sortByLabel(A_T.transitions); // needed for calculate_mu
		A_T.transitions.sort();

		A_R.stateNames.emplace(computeInitial_A_R(A_rho), 0);
		A_R.states.push_back(&A_R.stateNames.begin()->first);
		A_R.initial.insert(0);
		A_R.transitions.startInd.push_back(0);
		std::queue<const State_t*> q;
		q.push(A_R.states.back());

		State_t nextStates[std::numeric_limits<USymbol>::max() + 1]; // not good if USymbol is later changed to a larger type
																	 // then this may cause stack overflow
		for(State step = 0; !q.empty(); step++)
		{
			const State_t& currState = *q.front();
			q.pop();

			for(State st : currState.g)
				for(const auto& tr : A_T.transitions(st))
					addSuccessor_g(nextStates[static_cast<USymbol>(tr.Label().first)], tr.To());

			for(State st : currState.R)
				for(const auto& tr : A_rho.transitions(st))
					addSuccessor_R(nextStates[static_cast<USymbol>(tr.Label())], tr.To());

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
		A_rho.print(std::cerr << "\nA_rho:\n") << '\n';
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
		std::cerr << "\n";
	}
	auto init_index(std::vector<std::uint32_t>& index_of_right_state) const
	{
		auto cmp_only_g = [](const State_t& a, const State_t& b) { return a.g < b.g; };
		std::map<const State_t*, std::uint32_t, IndirectlyCompare<decltype(cmp_only_g)>> map(cmp_only_g);
		index_of_right_state.resize(A_R.stateNames.size());
		for(const auto& [st, st_name] : A_R.stateNames)
			index_of_right_state[st_name] = map.try_emplace(&st, map.size()).first->second;
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
			try
			{
				std::size_t ind_in_g = right_state.g_inv.at(tr.To());
				if(buf_mu[letter_ind] > ind_in_g)
				{
					buf_mu[letter_ind] = ind_in_g;
					buf_outputs[letter_ind] = tr.Label().second;
				}
			}
			catch(const std::out_of_range&) {} // if tr.To() is not in Rng(g), do nothing
		}
		return buf_outputs;
	}

	// used by BimachineWithFinalOutput
	std::pair<State, Word> calculate_g_of_mu(State q, Symbol letter, const TSBM_RightAutomaton::State_t& right_state) const
	{
		std::size_t mu = std::numeric_limits<std::size_t>::max();
		Word output;
		for(const auto& tr : std::ranges::equal_range(A_T.transitions(q), letter, {}, [](const Transition<Symbol_Word>& tr) { return tr.Label().first; })) try
		{
			std::size_t ind_in_g = right_state.g_inv.at(tr.To());
			if(mu > ind_in_g)
			{
				mu = ind_in_g;
				output = tr.Label().second;
			}
		}
		catch(const std::out_of_range&) {} // if tr.To() is not in Rng(g), do nothing
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
	std::vector<std::uint32_t> index_of_left_state, index_of_right_state;
	Function<State, State, USymbol, State> delta;
	Function<Word, State, USymbol, State> psi_delta;
	Function<State, State, State> tau;
	Function<Word, State, State> psi_tau;
	State q_err;
	std::unordered_map<State, std::uint32_t> type_of_final_center;
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
		TSBM_LeftAutomaton left{std::move(batch)};
		auto left_classes = left.init_index(index_of_left_state);
		TSBM_RightAutomaton right{std::move(batch)};
		auto right_classes = right.init_index(index_of_right_state);

		// defined outside of the loops to avoid reallocations
		std::vector<std::size_t> mu(right.A_R.alphabet.size());
		std::vector<Word> outputs(right.A_R.alphabet.size());

		q_err = right.A_T.statesCnt;
		right.A_T.transitions.sort();
		for(const auto& [right_state_ptr, right_ind] : right_classes)
		{
			for(State q = 0; q < right.A_T.statesCnt; q++)
			{
				right.calculate_mu(mu, outputs, q, *right_state_ptr);
				for(std::size_t letter_ind = 0; letter_ind < mu.size(); letter_ind++)
					if(mu[letter_ind] != std::numeric_limits<std::size_t>::max())
					{
						delta.emplace(right_state_ptr->g[mu[letter_ind]], q, right.A_R.alphabet[letter_ind], right_ind);
						if(!(outputs[letter_ind].size() == 1 && outputs[letter_ind][0] == right.A_R.alphabet[letter_ind]))
							psi_delta.emplace(outputs[letter_ind], q, right.A_R.alphabet[letter_ind], right_ind);
					}
			}

			for(const auto& [rules_left_ctx_ok_ptr, left_ind] : left_classes)
			{
				if(State init = nu(right, *rules_left_ctx_ok_ptr, *right_state_ptr); init != Constants::InvalidState)
					tau.emplace(init, left_ind, right_ind);
				if(std::uint32_t rule = minJ(right, batch, *rules_left_ctx_ok_ptr, *right_state_ptr);
					rule != Constants::InvalidRule && !batch[rule].output_for_epsilon->empty()
				)
					psi_tau.emplace(*batch[rule].output_for_epsilon, left_ind, right_ind);
			}
		}

		this->left = std::move(left.DFA);
		this->right = std::move(right.A_R).getMFSA();
		tau.prepare({left_classes.size() - 1, right_classes.size() - 1});
		psi_tau.prepare({left_classes.size() - 1, right_classes.size() - 1});
		delta.prepare({q_err, std::numeric_limits<std::make_unsigned_t<Symbol>>::max(), right_classes.size() - 1});
		psi_delta.prepare({q_err, std::numeric_limits<std::make_unsigned_t<Symbol>>::max(), right_classes.size() - 1});
		type_of_final_center = std::move(right.type_of_final_center);

		//debug
		/*for(auto t : psi_tau.buf)
			std::cerr << std::get<0>(t) << ' ' << std::get<1>(t) << ' ' << std::get<2>(t) << '\n';*/
	}
	Word operator()(const Word& input) const
	{
		std::vector<State> left_path = left.findPath(input), right_path = right.findPath(std::ranges::views::reverse(input));
		auto left_path_range = left_path | std::ranges::views::transform([this](State l) { return index_of_left_state[l]; });
		auto left_path_it = left_path_range.begin();
		auto right_path_rev_range = right_path | std::ranges::views::reverse | std::ranges::views::transform([this](State r) { return index_of_right_state[r]; });
		auto right_path_rev_it = right_path_rev_range.begin();

		Word output;
		State curr/* = q_err*/;
		//std::cerr << "path:\n" << q_err << '\n';
		curr = tau(*left_path_it, *right_path_rev_it, q_err);
		output += psi_tau(*left_path_it++, *right_path_rev_it++, {});
		//std::cerr << curr << '\n';
		for(Symbol s : input)
		{
			State next = delta(curr, s, *right_path_rev_it, q_err);
			//std::cerr << next << '\n';
			output += psi_delta(curr, s, *right_path_rev_it, {s});
			if(next == q_err || type_of_final_center.contains(next))
			{
				curr = tau(*left_path_it, *right_path_rev_it, q_err);
				if(curr == q_err)
					output += psi_tau(*left_path_it, *right_path_rev_it, {});
			}
			else
				curr = next;
			++left_path_it;
			++right_path_rev_it;
			//std::cerr << curr << '\n';
		}
		return output;
	}
};

#endif
