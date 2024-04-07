#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <chrono>
#include "regularExpression.h"
#include "ThompsonsConstruction.h"
#include "transducer.h"
#include "contextualReplacementRule.h"
#include "twostepBimachine.h"
#include "classicalBimachine.h"
#include "PorterStemmer.h"

std::string readFromFile(const std::filesystem::path& path, char delim = '\n')
{
	std::ifstream ifs(path);
	if(!ifs)
		throw std::runtime_error("could not open \"" + path.string() + "\" for reading");
	std::string str;
	std::getline(ifs, str, delim);
	return str;
}

// g++ -Wall -pedantic-errors -O2 -std=c++23 -fdiagnostics-color=always *.cpp

int main(int argc, char** argv) try
{
	using namespace std::string_literals;
	/*if(argc != 4)
		throw std::runtime_error("Usage: "s + (argv[0] ? argv[0] : "<program name>") + " <alphabet_file> <pattern_file> <text_file>");
	std::string alphabet = readFromFile(argv[1], Constants::Epsilon);
	Transducer<false, Symbol_Word> rtime;
	{
		std::cerr << "Constructing regular expression...\n";
		RegularExpression<WordPair> r(readFromFile(argv[2]));
		std::cerr << "Constructing letter transducer...\n";
		LetterTransducer lt = Transducer(regexToMFSA(std::move(r), alphabet)).expand();
		std::cerr << "Constructing transducer for LML...\n";
		auto rtime = ReplaceLML(lt, alphabet).realTime();
	}
	std::cerr << "Constructing bimachine...\n";
	Bimachine bm(std::move(rtime), alphabet);
	//std::cerr << bm << '\n';
	std::string text = readFromFile(argv[3], '\0');
	std::cerr << "Replacing text...\n";
	std::cout << bm(text) << std::endl;*/

	/*{
		//RegularExpression<WordPair> r("([aa,x]|[a,yy])([aa,x]|[a,yy])*");
		//RegularExpression<WordPair> r("[a,_][_,x]*"); // infinitely ambiguous
		//RegularExpression<WordPair> r("[a,x]|[a,y]|[a,z]|[x,x][a,t][a,t]*");
		//RegularExpression<WordPair> r("[ ,_][ ,_]*[_, ]|[a,A]|[b,B]|[c,C]|[d,D]|[e,E]|[f,F]|[new york university,01]|[new york,02]|[york,03]|[york university,044]");
		//RegularExpression<WordPair> r("[`,_][`,_]*[_,`]|[a,A]|[b,B]|[c,C]|[d,D]|[e,E]|[f,F]");
		//RegularExpression<WordPair> r("[`,_][`,_]*[_,`]|[a,A]|[b,B]|[d,D]");
		//RegularExpression<WordPair> r("[x,y][x,y]*[y,x]|[a,A]|[b,B]|[c,C]|[d,D]|[e,E]|[f,F]");
		std::string s;
		std::getline(std::cin, s);
		//RegularExpression<WordPair> r(s);
		std::string alphabet("abcfdefghijklmn`o pqrstuvw!yz.x0123456789");
		//std::string alphabet("abd`");
		//std::cerr << alphabet << '\n';
		LetterTransducer T3(Transducer(regexToMFSA(std::move(r), alphabet)).expand());
		//std::cout << T3.pseudoMinimize() << '\n';
		//using namespace std::string_view_literals;
		//auto R = Replace(T3, "abyx"sv);
		//R.trim().transitions.sort();
		//std::cout << R << '\n';
		auto lml = ReplaceLML(T3, alphabet);
		lml.pseudoMinimize();
		//lml.transitions.sort();
		//std::cout << lml << '\n';
		//std::cout << lml.pseudoMinimize() << '\n';
		auto rt = lml.realTime();
		//rt.transitions.sort();
		//std::cout << rt << '\n';
		Bimachine bm(std::move(rt), alphabet);

		std::cout << bm(s) << std::endl;
		//std::cout << bm("xaaaaxy") << std::endl;
		//std::cout << bm("sd") << std::endl;
	}*/
	/*{
		RegularExpression<SymbolOrEpsilon> r("a*bb*c"), r2("c*|aabbc*");
		std::cout << r.TokenizedRegex() << '\n';
		std::cout << r.TokenizedReversePolishNotation() << '\n';
		for(auto&& x : r.BaseTokens())
			std::cout << x << '\n';
		ClassicalFSA ca = regexToMFSA(std::move(r), "abc"), ca2 = regexToMFSA(std::move(r2), "abc");
		std::cout << ca << '\n';
		//std::cout << ca.convertToDFSA() << '\n';
		//std::cout << ca2.convertToDFSA() << '\n';
		std::cout << ca.intersect(ca2) << '\n';
	}*/
	/*{
		//RegularExpression<SymbolOrEpsilon> r("sses");
		RegularExpression<WordPair> r("[aa,X]");
		//RegularExpression<SymbolOrEpsilon> r("aa*");

		//ClassicalFSA T = regexToMFSA(std::move(r), "se");
		//T.convertToDFSA();
		std::unordered_set<Word> outputsForEpsilon;
		LetterTransducer T2(Transducer(regexToMFSA(std::move(r), PorterStemmer::alphabet)).expand());
		auto T = T2.realTime(&outputsForEpsilon);
		T.print(std::cerr) << '\n';
		T.pseudoMinimize();//.toSimple();
		T.print(std::cerr) << '\n';
		//T.pseudoMinimize().print(std::cerr) << '\n';
		//T.toSimple().print(std::cerr) << '\n';

	}*/
	/*{
		const std::string alphabet = "axb";
		std::vector<ContextualReplacementRule> rules{{"[ab,bbb]"s, "aa"s, "_"s},
													 {"[b,z]"s, "b"s, "a"s},
													 {"[a,_][a,_]*[_,x]|[_,y]"s, "aa"s, "_"s},
													 //{"[a,x]*"s, "aaa*"s, "b|a"s},
													 {"[_,c]"s, "_"s, "_"s},
		};
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
		{
			batch.emplace_back(rules[i], alphabet);
		//	std::cerr << "rule " << i << ": middle: \n";
		//	batch[i].center_rt.print(std::cerr);
		//	std::cerr << "output for epsilon: " << batch[i].output_for_epsilon.value_or("none!") << "\n\n";
		}

		//TSBM_LeftAutomaton left(std::move(batch));

		//TSBM_RightAutomaton right(std::move(batch));
		TwostepBimachine tsbm(batch);
		std::cout << tsbm("aa") << std::endl;
		std::cout << tsbm("aaaabba") << std::endl;
		std::cout << tsbm("aaaaba") << std::endl;
		std::cout << tsbm("abaaaaaaaabba") << std::endl;
		std::cout << tsbm("abaaaabaaaabba") << std::endl;
		std::cout << tsbm("abaabaaaabba") << std::endl;
		std::cout << tsbm("abaaabaaaabbaaaaaa") << std::endl;

		std::cout << "-----------------------\n";

		BimachineWithFinalOutput bmfo(batch);
		std::cout << bmfo("aa") << std::endl;
		std::cout << bmfo("aaaabba") << std::endl;
		std::cout << bmfo("aaaaba") << std::endl;
		std::cout << bmfo("abaaaaaaaabba") << std::endl;
		std::cout << bmfo("abaaaabaaaabba") << std::endl;
		std::cout << bmfo("abaabaaaabba") << std::endl;
		std::cout << bmfo("abaaabaaaabbaaaaaa") << std::endl;

		std::cout << "=========================\n";
	}*/
	/*{
		std::vector<ContextualReplacementRule> rules = PorterStemmer::steps[0];
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
		{
			batch.emplace_back(rules[i], PorterStemmer::alphabet);
			batch[i].center_rt.print(std::cerr);
		}

		TwostepBimachine tsbm(batch);
		std::cout << tsbm(" caresses ") << std::endl;
		std::cout << tsbm(" ponies ") << std::endl;
		std::cout << tsbm(" ties ") << std::endl;
		std::cout << tsbm(" caress ") << std::endl;
		std::cout << tsbm(" cats ") << std::endl;
		std::cout << tsbm(" abatements ") << std::endl;

		std::cout << "-----------------------\n";

		BimachineWithFinalOutput bmfo(batch);
		std::cout << bmfo(" caresses ") << std::endl;
		std::cout << bmfo(" ponies ") << std::endl;
		std::cout << bmfo(" ties ") << std::endl;
		std::cout << bmfo(" caress ") << std::endl;
		std::cout << bmfo(" cats ") << std::endl;
		std::cout << bmfo(" abatements ") << std::endl;

		std::cout << "=========================\n";
	}*/
	/*{
		std::vector<ContextualReplacementRule> rules = PorterStemmer::steps[1];
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
			batch.emplace_back(rules[i], PorterStemmer::alphabet);

		TwostepBimachine tsbm(batch);
		std::cout << tsbm(" feed ") << std::endl;
		std::cout << tsbm(" agreed ") << std::endl;

		std::cout << "-----------------------\n";

		BimachineWithFinalOutput bmfo(batch);
		std::cout << bmfo(" feed ") << std::endl;
		std::cout << bmfo(" agreed ") << std::endl;

		std::cout << "=========================\n";
	}
	{
		std::vector<ContextualReplacementRule> rules = PorterStemmer::steps[7];
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
			batch.emplace_back(rules[i], PorterStemmer::alphabet);

		TwostepBimachine tsbm(batch);
		std::cout << tsbm(" cease ") << std::endl;
		std::cout << tsbm(" csase ") << std::endl;

		std::cout << "-----------------------\n";

		BimachineWithFinalOutput bmfo(batch);
		std::cout << bmfo(" cease ") << std::endl;
		std::cout << bmfo(" csase ") << std::endl;

		std::cout << "=========================\n";
	}*/

	{
		//std::vector<BimachineWithFinalOutput> bm;
		std::vector<TwostepBimachine> bm;
		std::vector<ContextualReplacementRuleRepresentation> batch;
		{
			auto start = std::chrono::steady_clock::now();
			for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
			{
				for(std::size_t j = 0; j < PorterStemmer::steps[i].size(); j++)
					batch.emplace_back(PorterStemmer::steps[i][j], PorterStemmer::alphabet);
				bm.emplace_back(std::move(batch));
				batch.clear();
			}
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for construction: " << std::fixed << std::chrono::duration<double>(end - start).count() << " s\n";
		}
		Word input;
		{
			auto start = std::chrono::steady_clock::now();
			input = readFromFile("/dev/stdin", '\0');
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for reading: " << std::fixed << std::chrono::duration<double>(end - start).count() << " s\n";
		}
		{
			auto start = std::chrono::steady_clock::now();
			for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
			{
				input = bm[i](input);
				// std::cerr << "{ ";
				// for(int c : input)
				// 	std::cerr << std::hex << c << ' ';
				// std::cerr << "}\n";
			}
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for replacing: " << std::fixed << std::chrono::duration<double>(end - start).count() << " s\n";
		}
		{
			auto start = std::chrono::steady_clock::now();
			std::cout << input;
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for printing: " << std::fixed << std::chrono::duration<double>(end - start).count() << " s\n";
		}
	}

	/*{
		RegularExpression<SymbolOrEpsilon> r("(a|b)*a(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b|_)(a|b|_)(a|b|_)(a|b|_)(a|b|_)(a|b|_)");
		RegularExpression<SymbolOrEpsilon> r2("(a|b)*a(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)");

		ClassicalFSA T = regexToMFSA(std::move(r), "ab");
		ClassicalFSA T2 = regexToMFSA(std::move(r2), "ab");
		ClassicalFSA result = T.intersect(T2.complement());
		std::cerr << "Minimizing...\n";
		result.pseudoMinimize().print();
	}*/

	/*{
		const std::string alphabet = "ab";
		std::vector<ContextualReplacementRule> rules{
			{"[a,_]*[aa,A]|[ab,_]"s, "a"s, "aa|b"s},
		};
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
		{
			batch.emplace_back(rules[i], alphabet);
			//std::cerr << "rule " << i << ":\n";
			//batch[i].center_rt.trim().print(std::cerr << "middle:\n");
			//batch[i].left.trim().print(std::cerr << "left:\n");
			//batch[i].right.trim().print(std::cerr << "right:\n");
			//std::cerr << "output for epsilon: " << batch[i].output_for_epsilon.value_or("none!") << "\n\n";
		}
		//TSBM_LeftAutomaton left(std::move(batch));
		//TSBM_RightAutomaton right(std::move(batch));
		//TwostepBimachine tsbm(batch);
		//std::cout << tsbm("aaaaaabaaab") << std::endl;
		//std::cout << "-----------------------\n";

		BimachineWithFinalOutput bmfo(batch);
		std::cout << bmfo("aaaaaabaaab") << std::endl;
		std::cout << "=========================\n";
	}*/
}
catch(const std::exception& e)
{
	std::cerr << e.what() << '\n';
	return 1;
}
