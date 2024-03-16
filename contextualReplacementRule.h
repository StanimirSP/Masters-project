#ifndef CONTEXTUALREPLACEMENTRULE_H
#define CONTEXTUALREPLACEMENTRULE_H

#include "regularExpression.h"
#include "classicalFSA.h"
#include "transducer.h"
#include "ThompsonsConstruction.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <optional>

struct ContextualReplacementRule
{
	RegularExpression<WordPair> center;
	RegularExpression<SymbolOrEpsilon> lctx, rctx;

	friend std::istream& operator>>(std::istream& is, ContextualReplacementRule& crr)
	{
		std::string regex;
		is >> std::quoted(regex);
		crr.center = regex;
		is >> std::quoted(regex);
		crr.lctx = regex;
		is >> std::quoted(regex);
		crr.rctx = regex;
		return is;
	}
};

struct ContextualReplacementRuleRepresentation
{
	std::optional<Word> output_for_epsilon;
	Transducer<false, Symbol_Word> center_rt;
	ClassicalFSA left, right;

	ContextualReplacementRuleRepresentation(const ContextualReplacementRule& crr, const std::string& alphabet)
	{
		ClassicalFSA all = ClassicalFSA::createFromSymbolSet(alphabet).KleeneStar();
		left = all.Concatenation(regexToMFSA(crr.lctx, alphabet)).pseudoMinimize().toRightSimple();
		right = regexToMFSA(crr.rctx, alphabet).Concatenation(all).pseudoMinimize().toLeftSimple();
		std::unordered_set<Word> outputsForEpsilon;
		center_rt = Transducer(regexToMFSA(crr.center, alphabet)).expand().realTime(&outputsForEpsilon).pseudoMinimize().toSimple();
		if(outputsForEpsilon.size() > 1)
			throw std::logic_error("crr.center does not represent a function");
		if(!outputsForEpsilon.empty())
		{
			output_for_epsilon = *outputsForEpsilon.begin();
			normalize(*output_for_epsilon);
		}
	}
};

#endif
