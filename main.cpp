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
		auto start = std::chrono::steady_clock::now();
		const std::string alphabet = "\n !\"'()*+,-./0123456789:;=?ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz";
		std::vector<ContextualReplacementRule> rules{{"[a,a]|[ability,ytiliba]|[able,elba]|[about,tuoba]|[above,evoba]|[accept,tpecca]|[according,gnidrocca]|[account,tnuocca]|[across,ssorca]|[act,tca]|[action,noitca]|[activity,ytivitca]|[actually,yllautca]|[add,dda]|[address,sserdda]|[administration,noitartsinimda]|[admit,timda]|[adult,tluda]|[affect,tceffa]|[after,retfa]|[again,niaga]|[against,tsniaga]|[age,ega]|[agency,ycnega]|[agent,tnega]|[ago,oga]|[agree,eerga]|[agreement,tnemeerga]|[ahead,daeha]|[air,ria]|[all,lla]|[allow,wolla]|[almost,tsomla]|[alone,enola]|[along,gnola]|[already,ydaerla]|[also,osla]|[although,hguohtla]|[always,syawla]|[American,naciremA]|[among,gnoma]|[amount,tnuoma]|[analysis,sisylana]|[and,dna]|[animal,lamina]|[another,rehtona]|[answer,rewsna]|[any,yna]|[anyone,enoyna]|[anything,gnihtyna]|[appear,raeppa]|[apply,ylppa]|[approach,hcaorppa]|[area,aera]|[argue,eugra]|[arm,mra]|[around,dnuora]|[arrive,evirra]|[art,tra]|[article,elcitra]|[artist,tsitra]|[as,sa]|[ask,ksa]|[assume,emussa]|[at,ta]|[attack,kcatta]|[attention,noitnetta]|[attorney,yenrotta]|[audience,ecneidua]|[author,rohtua]|[authority,ytirohtua]|[available,elbaliava]|[avoid,diova]|[away,yawa]|[baby,ybab]|[back,kcab]|[bad,dab]|[bag,gab]|[ball,llab]|[bank,knab]|[bar,rab]|[base,esab]|[be,eb]|[beat,taeb]|[beautiful,lufituaeb]|[because,esuaceb]|[become,emoceb]|[bed,deb]|[before,erofeb]|[begin,nigeb]|[behavior,roivaheb]|[behind,dniheb]|[believe,eveileb]|[benefit,tifeneb]|[best,tseb]|[better,retteb]|[between,neewteb]|[beyond,dnoyeb]|[big,gib]|[bill,llib]|[billion,noillib]|[bit,tib]|[black,kcalb]|[blood,doolb]|[blue,eulb]|[board,draob]|[body,ydob]|[book,koob]|[born,nrob]|[both,htob]|[box,xob]|[boy,yob]|[break,kaerb]|[bring,gnirb]|[brother,rehtorb]|[budget,tegdub]|[build,dliub]|[building,gnidliub]|[business,ssenisub]|[but,tub]|[buy,yub]|[by,yb]|[call,llac]|[camera,aremac]|[campaign,ngiapmac]|[can,nac]|[cancer,recnac]|[candidate,etadidnac]|[capital,latipac]|[car,rac]|[card,drac]|[care,erac]|[career,reerac]|[carry,yrrac]|[case,esac]|[catch,hctac]|[cause,esuac]|[cell,llec]|[center,retnec]|[central,lartnec]|[century,yrutnec]|[certain,niatrec]|[certainly,ylniatrec]|[chair,riahc]|[challenge,egnellahc]|[chance,ecnahc]|[change,egnahc]|[character,retcarahc]|[charge,egrahc]|[check,kcehc]|[child,dlihc]|[choice,eciohc]|[choose,esoohc]|[church,hcruhc]|[citizen,nezitic]|[city,ytic]|[civil,livic]|[claim,mialc]|[class,ssalc]|[clear,raelc]|[clearly,ylraelc]|[close,esolc]|[coach,hcaoc]|[cold,dloc]|[collection,noitcelloc]|[college,egelloc]|[color,roloc]|[come,emoc]|[commercial,laicremmoc]|[common,nommoc]|[community,ytinummoc]|[company,ynapmoc]|[compare,erapmoc]|[computer,retupmoc]|[concern,nrecnoc]|[condition,noitidnoc]|[conference,ecnerefnoc]|[Congress,ssergnoC]|[consider,redisnoc]|[consumer,remusnoc]|[contain,niatnoc]|[continue,eunitnoc]|[control,lortnoc]|[cost,tsoc]|[could,dluoc]|[country,yrtnuoc]|[couple,elpuoc]|[course,esruoc]|[court,truoc]|[cover,revoc]|[create,etaerc]|[crime,emirc]|[cultural,larutluc]|[culture,erutluc]|[cup,puc]|[current,tnerruc]|[customer,remotsuc]|[cut,tuc]|[dark,krad]|[data,atad]|[daughter,rethguad]|[day,yad]|[dead,daed]|[deal,laed]|[death,htaed]|[debate,etabed]|[decade,edaced]|[decide,ediced]|[decision,noisiced]|[deep,peed]|[defense,esnefed]"s, "_"s, "_"s},
		};
		std::vector<ContextualReplacementRuleRepresentation> batch;
		for(std::size_t i = 0; i < rules.size(); i++)
		{
			batch.emplace_back(rules[i], alphabet);
		}
		TwostepBimachine tsbm(batch);
		auto end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for construction: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";

		start = std::chrono::steady_clock::now();
		Word input = readFromFile("/dev/stdin", '\0');
		std::cout << tsbm(input) << std::endl;
		end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for replacing: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
	}*/
	{
		//std::vector<BimachineWithFinalOutput> bm;
		std::vector<TwostepBimachine> bm;
		std::vector<ContextualReplacementRuleRepresentation> batch;
		{
			auto start = std::chrono::steady_clock::now();
			for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
			{
				auto start = std::chrono::steady_clock::now();
				for(std::size_t j = 0; j < PorterStemmer::steps[i].size(); j++)
					batch.emplace_back(PorterStemmer::steps[i][j], PorterStemmer::alphabet);
				bm.emplace_back(std::move(batch));
				batch.clear();
				auto end = std::chrono::steady_clock::now();
				std::cerr << "\telapsed time for construction of step " << i << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
			}
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for construction: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
		}
		Word input;
		{
			auto start = std::chrono::steady_clock::now();
			input = readFromFile("/dev/stdin", '\0');
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for reading: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
		}
		{
			auto start = std::chrono::steady_clock::now();
			for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
			{
				auto start = std::chrono::steady_clock::now();
				input = bm[i](input);
				// std::cerr << "{ ";
				// for(int c : input)
				// 	std::cerr << std::hex << c << ' ';
				// std::cerr << "}\n";
				auto end = std::chrono::steady_clock::now();
				std::cerr << "\telapsed time for replacing at step " << i << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
			}
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for replacing: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
		}
		{
			auto start = std::chrono::steady_clock::now();
			std::cout << input;
			auto end = std::chrono::steady_clock::now();
			std::cerr << "elapsed time for printing: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
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
			batch[i].center_rt.trim().print(std::cerr << "middle:\n");
			//batch[i].left.trim().print(std::cerr << "left:\n");
			//batch[i].right.trim().print(std::cerr << "right:\n");
			//std::cerr << "output for epsilon: " << batch[i].output_for_epsilon.value_or("none!") << "\n\n";
		}
		//TSBM_LeftAutomaton left(std::move(batch));
		//TSBM_RightAutomaton right(std::move(batch));
		TwostepBimachine tsbm(batch);
		std::cout << tsbm("aaaaaabaaab") << std::endl;
		std::cout << "-----------------------\n";

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
