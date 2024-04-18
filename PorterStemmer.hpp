#ifndef PORTERSTEMMER_HPP
#define PORTERSTEMMER_HPP

#include <vector>
#include <string>
#include <cstddef>
#include "constants.hpp"
#include "contextualReplacementRule.hpp"

namespace PorterStemmer
{
	inline const std::string alphabet = "abcdefghijklmnopqrstuvwxyz \r\n\t\v\x01\x02";
	inline const std::string letter = "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)";
	inline const std::string letter_not_s = "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|t|u|v|w|x|y|z)";
	inline const std::string whitespace = "( |\r|\n|\t|\v)";
	inline const std::string always_vowel = "(a|e|i|o|u)";
	inline const std::string vowel_or_y = "(a|e|i|o|u|y)";
	inline const std::string always_consonant = "(b|c|d|f|g|h|j|k|l|m|n|p|q|r|s|t|v|w|x|z)";
	inline const std::string consonant_or_y = "(b|c|d|f|g|h|j|k|l|m|n|p|q|r|s|t|v|w|x|y|z)";
	inline const std::string consonant_not_wxy = "(b|c|d|f|g|h|j|k|l|m|n|p|q|r|s|t|v|z)";

	inline const std::string contains_vowel = '(' + letter + '*' + always_vowel + letter + "*|" + letter + '*' + consonant_or_y + vowel_or_y + letter + "*)";
	inline const std::string C = '(' + consonant_or_y + always_consonant + "*)";	// no 'y' after a consonant is allowed in C because the it would be a vowel
	inline const std::string V = '(' + vowel_or_y + always_vowel + "*)";			// no 'y' after a vowel is allowed in V because the it would be a consonant
	inline const std::string opt_C = '(' + C + "|_)";
	inline const std::string opt_V = '(' + V + "|_)";
	inline const std::string VC = '(' + V + C + ')';
	inline const std::string V_starting_non_y = '(' + always_vowel + always_vowel + "*)";

	inline const std::string m_gt_0 = "((" + C + VC + '|' + V_starting_non_y + C + ')' + VC + '*' + opt_V + ')';
	inline const std::string m_gt_1 = "((" + C + VC + '|' + V_starting_non_y + C + ')' + VC + VC + '*' + opt_V + ')';
	inline const std::string lctx{Constants::Epsilon};
	inline const std::string rctx{'\x02'};

	using namespace std::string_literals;
	inline const std::vector<ContextualReplacementRule> steps[] = {
		{ // 1a
			{"[sses,ss\x02]"s, lctx, whitespace},						// SSES -> SS
			{"[ies,i\x02]"s, lctx, whitespace},							// IES  -> I
			{"[ss,ss\x02]"s, letter, whitespace},						// SS   -> SS
			{"[s,\x02]"s, letter + letter, whitespace},					// S    ->
			{"[_,\x02]"s, letter + letter + letter_not_s, whitespace},	// only allow words of length > 2 to be modified
		},
		{ // 1b -- '\x01' is used as a marker for whether step 1b' should take place
			{"[eed,ee]"s, m_gt_0, rctx},				// (m>0) EED -> EE
			{"[eed,eed]"s, lctx, rctx}, 				// suppresses the rules below if the word ends in 'eed' but the condition (m>0) is not satisfied
			{"[ed,\x01]"s, contains_vowel, rctx},		// (*v*) ED  ->
			{"[ing,\x01]"s, contains_vowel, rctx},		// (*v*) ING ->
		},
		{ // 1b'
			{"[at\x01,ate]"s, lctx, rctx},													// AT -> ATE
			{"[bl\x01,ble]"s, lctx, rctx},													// BL -> BLE
			{"[iz\x01,ize]"s, lctx, rctx},													// IZ -> IZE
			{"([bb,b]|[cc,c]|[dd,d]|[ff,f]|[gg,g]|[hh,h]|[jj,j]|[kk,k]|[mm,m]|[nn,n]"
			 "|[pp,p]|[qq,q]|[rr,r]|[tt,t]|[vv,v]|[ww,w]|[xx,x])[\x01,_]"s, lctx, rctx},	// (*d and not (*L or *S or *Z)) -> single letter
			{"[\x01,e]"s, whitespace + C + vowel_or_y + consonant_not_wxy, rctx},			// (m=1 and *o) -> E
			{"[\x01,_]"s, lctx, rctx},														// if none of the above can be applied, deletes marker '\x01'
		},
		{ // 1c
			{"[y,i]"s, contains_vowel, rctx},	// (*v*) Y -> I
		},
		{ // 2
			{"[ational,ate]"s, m_gt_0, rctx},	// (m>0) ATIONAL ->  ATE
			{"[ational,ational]"s, lctx, rctx},
			{"[tional,tion]"s, m_gt_0, rctx},	// (m>0) TIONAL  ->  TION
			{"[tional,tional]"s, lctx, rctx},
			{"[enci,ence]"s, m_gt_0, rctx},		// (m>0) ENCI    ->  ENCE
			{"[enci,enci]"s, lctx, rctx},
			{"[anci,ance]"s, m_gt_0, rctx},		// (m>0) ANCI    ->  ANCE
			{"[anci,anci]"s, lctx, rctx},
			{"[izer,ize]"s, m_gt_0, rctx},		// (m>0) IZER    ->  IZE
			{"[izer,izer]"s, lctx, rctx},
		//	{"[abli,able]"s, m_gt_0, rctx},		// (m>0) ABLI    ->  ABLE
		//	{"[abli,abli]"s, lctx, rctx},
			{"[bli,ble]"s, m_gt_0, rctx},		// (m>0) BLI    ->  BLE		// replaces the above rule
			{"[bli,bli]"s, lctx, rctx},
			{"[alli,al]"s, m_gt_0, rctx},		// (m>0) ALLI    ->  AL
			{"[alli,alli]"s, lctx, rctx},
			{"[entli,ent]"s, m_gt_0, rctx},		// (m>0) ENTLI   ->  ENT
			{"[entli,entli]"s, lctx, rctx},
			{"[eli,e]"s, m_gt_0, rctx},			// (m>0) ELI     ->  E
			{"[eli,eli]"s, lctx, rctx},
			{"[ousli,ous]"s, m_gt_0, rctx},		// (m>0) OUSLI   ->  OUS
			{"[ousli,ousli]"s, lctx, rctx},
			{"[ization,ize]"s, m_gt_0, rctx},	// (m>0) IZATION ->  IZE
			{"[ization,ization]"s, lctx, rctx},
			{"[ation,ate]"s, m_gt_0, rctx},		// (m>0) ATION   ->  ATE
			{"[ation,ation]"s, lctx, rctx},
			{"[ator,ate]"s, m_gt_0, rctx},		// (m>0) ATOR    ->  ATE
			{"[ator,ator]"s, lctx, rctx},
			{"[alism,al]"s, m_gt_0, rctx},		// (m>0) ALISM   ->  AL
			{"[alism,alism]"s, lctx, rctx},
			{"[iveness,ive]"s, m_gt_0, rctx},	// (m>0) IVENESS ->  IVE
			{"[iveness,iveness]"s, lctx, rctx},
			{"[fulness,ful]"s, m_gt_0, rctx},	// (m>0) FULNESS ->  FUL
			{"[fulness,fulness]"s, lctx, rctx},
			{"[ousness,ous]"s, m_gt_0, rctx},	// (m>0) OUSNESS ->  OUS
			{"[ousness,ousness]"s, lctx, rctx},
			{"[aliti,al]"s, m_gt_0, rctx},		// (m>0) ALITI   ->  AL
			{"[aliti,aliti]"s, lctx, rctx},
			{"[iviti,ive]"s, m_gt_0, rctx},		// (m>0) IVITI   ->  IVE
			{"[iviti,iviti]"s, lctx, rctx},
			{"[biliti,ble]"s, m_gt_0, rctx},	// (m>0) BILITI  ->  BLE
			{"[biliti,biliti]"s, lctx, rctx},
			{"[logi,log]"s, m_gt_0, rctx},		// (m>0) LOGI    ->  LOG	// new rule
			{"[logi,logi]"s, lctx, rctx},
		},
		{ // 3
			{"[icate,ic]"s, m_gt_0, rctx},		// (m>0) ICATE ->  IC
			{"[ative,_]"s, m_gt_0, rctx},		// (m>0) ATIVE ->
			{"[alize,al]"s, m_gt_0, rctx},		// (m>0) ALIZE ->  AL
			{"[iciti,ic]"s, m_gt_0, rctx},		// (m>0) ICITI ->  IC
			{"[ical,ic]"s, m_gt_0, rctx},		// (m>0) ICAL  ->  IC
			{"[ful,_]"s, m_gt_0, rctx},			// (m>0) FUL   ->
			{"[ness,_]"s, m_gt_0, rctx},		// (m>0) NESS  ->
		},
		{ // 4
			{"[al,_]"s, m_gt_1, rctx},			// (m>1) AL    ->
			{"[al,al]"s, lctx, rctx},
			{"[ance,_]"s, m_gt_1, rctx},		// (m>1) ANCE  ->
			{"[ance,ance]"s, lctx, rctx},
			{"[ence,_]"s, m_gt_1, rctx},		// (m>1) ENCE  ->
			{"[ence,ence]"s, lctx, rctx},
			{"[er,_]"s, m_gt_1, rctx},			// (m>1) ER    ->
			{"[er,er]"s, lctx, rctx},
			{"[ic,_]"s, m_gt_1, rctx},			// (m>1) IC    ->
			{"[ic,ic]"s, lctx, rctx},
			{"[able,_]"s, m_gt_1, rctx},		// (m>1) ABLE  ->
			{"[able,able]"s, lctx, rctx},
			{"[ible,_]"s, m_gt_1, rctx},		// (m>1) IBLE  ->
			{"[ible,ible]"s, lctx, rctx},
			{"[ant,_]"s, m_gt_1, rctx},			// (m>1) ANT   ->
			{"[ant,ant]"s, lctx, rctx},
			{"[ement,_]"s, m_gt_1, rctx},		// (m>1) EMENT ->
			{"[ement,ement]"s, lctx, rctx},
			{"[ment,_]"s, m_gt_1, rctx},		// (m>1) MENT  ->
			{"[ment,ment]"s, lctx, rctx},
			{"[ent,_]"s, m_gt_1, rctx},			// (m>1) ENT   ->
			{"[ent,ent]"s, lctx, rctx},
			{"[ion,_]"s, "((" + C + VC + '|' + V_starting_non_y + C + ')' + VC + '*' + V + opt_C + "(s|t))", rctx},		// (m>1 and (*S or *T)) ION ->
			{"[ion,ion]"s, lctx, rctx},
			{"[ou,_]"s, m_gt_1, rctx},			// (m>1) OU    ->
			{"[ou,ou]"s, lctx, rctx},
			{"[ism,_]"s, m_gt_1, rctx},			// (m>1) ISM   ->
			{"[ism,ism]"s, lctx, rctx},
			{"[ate,_]"s, m_gt_1, rctx},			// (m>1) ATE   ->
			{"[ate,ate]"s, lctx, rctx},
			{"[iti,_]"s, m_gt_1, rctx},			// (m>1) ITI   ->
			{"[iti,iti]"s, lctx, rctx},
			{"[ous,_]"s, m_gt_1, rctx},			// (m>1) OUS   ->
			{"[ous,ous]"s, lctx, rctx},
			{"[ive,_]"s, m_gt_1, rctx},			// (m>1) IVE   ->
			{"[ive,ive]"s, lctx, rctx},
			{"[ize,_]"s, m_gt_1, rctx},			// (m>1) IZE   ->
			{"[ize,ize]"s, lctx, rctx},
		},
		{ // 5a
			{"[e,_]"s, m_gt_1, rctx},											// (m>1) E     ->
			{"[e,_]"s, whitespace + "((" +
				'(' + always_vowel + always_vowel + "*)" + C + opt_V + ")|(" +
				C + VC + V + ")|(" +
				C + V + always_vowel + C + opt_V + ")|(" +
				C + V + "(w|x|y)" + ")|(" +
				C + V + C + always_consonant + opt_V +
				"))", rctx
			},																	// (m=1 and not *o) E ->
		},
		{ // 5b
			{"[l\x02,_]"s, "((" + C + VC + '|' + V_starting_non_y + C + ')' + VC + '*' + V + opt_C + "l)", whitespace},		// (m > 1 and *d and *L) -> single letter;
																															// also deletes the marker '\x02'
			{"[\x02,_]"s, lctx, whitespace}, 																				// deletes the marker '\x02'
		},
	};
	inline constexpr std::size_t steps_cnt = sizeof(steps) / sizeof(*steps);
}

#endif
