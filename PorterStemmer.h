#ifndef PORTERSTEMMER_H
#define PORTERSTEMMER_H

#include <vector>
#include <string>
#include <cstddef>
#include "contextualReplacementRule.h"

namespace PorterStemmer
{
	inline const std::string alphabet = "abcdefghijklmnopqrstuvwxyz \r\n\t\v\x01\x02";
	inline const std::string letter = "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)";
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
	inline const std::string m_eq_1 = '(' + whitespace + '(' + C + VC + '|' + V_starting_non_y + C + ')' + opt_V + ')';
	//inline const std::string V_starting_y = 'y' + always_vowel + '*';
	inline const std::string rctx = "\x02";

	using namespace std::string_literals;
	inline constexpr std::size_t steps_cnt = 10;
	inline const std::vector<ContextualReplacementRule> steps[steps_cnt] = {
		{ // 0
			{"[_,\x02]"s, letter + letter + letter, whitespace}, // only allow words of lenght > 2 to be modified
		},
		{ // 1a
			{"[sses,ss]"s, "_"s, rctx},						// SSES -> SS
			{"[ies,i]"s, "_"s, rctx},						// IES  -> I
			{"[ss,ss]"s, "_"s, rctx},						// SS   -> SS
			{"[s,_]"s, "_"s/*letter + letter*/, rctx},		// S    ->
		},
		{ // 1b -- '\x01' is used as a marker for whether step 1b' should take place
			{"[eed,ee]"s, m_gt_0, rctx},							// (m>0) EED -> EE
			{"[eed,eed]"s, "_"s, rctx}, 							// suppresses the rules below if the word ends in 'eed' but the condition (m>0) is not satisfied
			{"[ed,\x01]"s, /*whitespace +*/ contains_vowel, rctx},		// (*v*) ED  ->
			{"[ing,\x01]"s, /*whitespace +*/ contains_vowel, rctx},		// (*v*) ING ->
		},
		{ // 1b'
			{"[at\x01,ate]"s, "_"s, rctx},																					// AT -> ATE
			{"[bl\x01,ble]"s, "_"s, rctx},																					// BL -> BLE
			{"[iz\x01,ize]"s, "_"s, rctx},																					// IZ -> IZE
			{"([bb,b]|[cc,c]|[dd,d]|[ff,f]|[gg,g]|[hh,h]|[jj,j]|[kk,k]|[mm,m]|[nn,n]"
			 "|[pp,p]|[qq,q]|[rr,r]|[tt,t]|[vv,v]|[ww,w]|[xx,x])[\x01,_]"s, "_"s, rctx},									// (*d and not (*L or *S or *Z)) -> single letter
			{"[\x01,e]"s, whitespace + C + vowel_or_y + consonant_not_wxy, rctx},	// (m=1 and *o) -> E
			{"[\x01,_]"s, "_"s, rctx},																						// if none of the above can be applied, deletes marker '\x01'
		},
		{ // 1c
			{"[y,i]"s, /*letter + vowel_or_y + '|' + contains_vowel + letter*/contains_vowel, rctx},			// (*v*) Y -> I
		},
		{ // 2
			{"[ational,ate]"s, m_gt_0, rctx},	// (m>0) ATIONAL ->  ATE
			{"[ational,ational]"s, "_"s, rctx},
			{"[tional,tion]"s, m_gt_0, rctx},	// (m>0) TIONAL  ->  TION
			{"[tional,tional]"s, "_"s, rctx},
			{"[enci,ence]"s, m_gt_0, rctx},		// (m>0) ENCI    ->  ENCE
			{"[enci,enci]"s, "_"s, rctx},
			{"[anci,ance]"s, m_gt_0, rctx},		// (m>0) ANCI    ->  ANCE
			{"[anci,anci]"s, "_"s, rctx},
			{"[izer,ize]"s, m_gt_0, rctx},		// (m>0) IZER    ->  IZE
			{"[izer,izer]"s, "_"s, rctx},
		//	{"[abli,able]"s, m_gt_0, rctx},		// (m>0) ABLI    ->  ABLE
		//	{"[abli,abli]"s, "_"s, rctx},
			{"[bli,ble]"s, m_gt_0, rctx},		// (m>0) BLI    ->  BLE		// replaces the above rule
			{"[bli,bli]"s, "_"s, rctx},
			{"[alli,al]"s, m_gt_0, rctx},		// (m>0) ALLI    ->  AL
			{"[alli,alli]"s, "_"s, rctx},
			{"[entli,ent]"s, m_gt_0, rctx},		// (m>0) ENTLI   ->  ENT
			{"[entli,entli]"s, "_"s, rctx},
			{"[eli,e]"s, m_gt_0, rctx},			// (m>0) ELI     ->  E
			{"[eli,eli]"s, "_"s, rctx},
			{"[ousli,ous]"s, m_gt_0, rctx},		// (m>0) OUSLI   ->  OUS
			{"[ousli,ousli]"s, "_"s, rctx},
			{"[ization,ize]"s, m_gt_0, rctx},	// (m>0) IZATION ->  IZE
			{"[ization,ization]"s, "_"s, rctx},
			{"[ation,ate]"s, m_gt_0, rctx},		// (m>0) ATION   ->  ATE
			{"[ation,ation]"s, "_"s, rctx},
			{"[ator,ate]"s, m_gt_0, rctx},		// (m>0) ATOR    ->  ATE
			{"[ator,ator]"s, "_"s, rctx},
			{"[alism,al]"s, m_gt_0, rctx},		// (m>0) ALISM   ->  AL
			{"[alism,alism]"s, "_"s, rctx},
			{"[iveness,ive]"s, m_gt_0, rctx},	// (m>0) IVENESS ->  IVE
			{"[iveness,iveness]"s, "_"s, rctx},
			{"[fulness,ful]"s, m_gt_0, rctx},	// (m>0) FULNESS ->  FUL
			{"[fulness,fulness]"s, "_"s, rctx},
			{"[ousness,ous]"s, m_gt_0, rctx},	// (m>0) OUSNESS ->  OUS
			{"[ousness,ousness]"s, "_"s, rctx},
			{"[aliti,al]"s, m_gt_0, rctx},		// (m>0) ALITI   ->  AL
			{"[aliti,aliti]"s, "_"s, rctx},
			{"[iviti,ive]"s, m_gt_0, rctx},		// (m>0) IVITI   ->  IVE
			{"[iviti,iviti]"s, "_"s, rctx},
			{"[biliti,ble]"s, m_gt_0, rctx},	// (m>0) BILITI  ->  BLE
			{"[biliti,biliti]"s, "_"s, rctx},
			{"[logi,log]"s, m_gt_0, rctx},		// (m>0) LOGI    ->  LOG	// new rule
			{"[logi,logi]"s, "_"s, rctx},
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
			{"[al,al]"s, "_"s, rctx},
			{"[ance,_]"s, m_gt_1, rctx},		// (m>1) ANCE  ->
			{"[ance,ance]"s, "_"s, rctx},
			{"[ence,_]"s, m_gt_1, rctx},		// (m>1) ENCE  ->
			{"[ence,ence]"s, "_"s, rctx},
			{"[er,_]"s, m_gt_1, rctx},			// (m>1) ER    ->
			{"[er,er]"s, "_"s, rctx},
			{"[ic,_]"s, m_gt_1, rctx},			// (m>1) IC    ->
			{"[ic,ic]"s, "_"s, rctx},
			{"[able,_]"s, m_gt_1, rctx},		// (m>1) ABLE  ->
			{"[able,able]"s, "_"s, rctx},
			{"[ible,_]"s, m_gt_1, rctx},		// (m>1) IBLE  ->
			{"[ible,ible]"s, "_"s, rctx},
			{"[ant,_]"s, m_gt_1, rctx},			// (m>1) ANT   ->
			{"[ant,ant]"s, "_"s, rctx},
			{"[ement,_]"s, m_gt_1, rctx},		// (m>1) EMENT ->
			{"[ement,ement]"s, "_"s, rctx},
			{"[ment,_]"s, m_gt_1, rctx},		// (m>1) MENT  ->
			{"[ment,ment]"s, "_"s, rctx},
			{"[ent,_]"s, m_gt_1, rctx},			// (m>1) ENT   ->
			{"[ent,ent]"s, "_"s, rctx},
			{"[ion,_]"s, "((" + C + VC + '|' + V_starting_non_y + C + ')' + VC + '*' + V + opt_C + "(s|t))", rctx},		// (m>1 and (*S or *T)) ION ->
			{"[ion,ion]"s, "_"s, rctx},
			{"[ou,_]"s, m_gt_1, rctx},			// (m>1) OU    ->
			{"[ou,ou]"s, "_"s, rctx},
			{"[ism,_]"s, m_gt_1, rctx},			// (m>1) ISM   ->
			{"[ism,ism]"s, "_"s, rctx},
			{"[ate,_]"s, m_gt_1, rctx},			// (m>1) ATE   ->
			{"[ate,ate]"s, "_"s, rctx},
			{"[iti,_]"s, m_gt_1, rctx},			// (m>1) ITI   ->
			{"[iti,iti]"s, "_"s, rctx},
			{"[ous,_]"s, m_gt_1, rctx},			// (m>1) OUS   ->
			{"[ous,ous]"s, "_"s, rctx},
			{"[ive,_]"s, m_gt_1, rctx},			// (m>1) IVE   ->
			{"[ive,ive]"s, "_"s, rctx},
			{"[ize,_]"s, m_gt_1, rctx},			// (m>1) IZE   ->
			{"[ize,ize]"s, "_"s, rctx},
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
			{"[\x02,_]"s, "_"s, whitespace}, // deletes the marker '\x02'
		},
	};
}

#endif
