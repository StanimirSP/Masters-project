#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <chrono>
#include "regularExpression.hpp"
#include "ThompsonsConstruction.hpp"
#include "transducer.hpp"
#include "contextualReplacementRule.hpp"
#include "twostepBimachine.hpp"
#include "classicalBimachine.hpp"
#include "PorterStemmer.hpp"

std::string readFromFile(const std::filesystem::path& path, char delim = '\n')
{
	std::ifstream ifs(path);
	if(!ifs)
		throw std::runtime_error("could not open \"" + path.string() + "\" for reading");
	std::string str;
	std::getline(ifs, str, delim);
	return str;
}

// g++ -Wall -pedantic-errors -O3 -std=c++23 -fdiagnostics-color=always *.cpp

int main(int argc, char** argv) try
{
	using Resolution = std::chrono::milliseconds;
	std::vector<BimachineWithFinalOutput> bm;
	//std::vector<TwostepBimachine> bm;
	std::vector<ContextualReplacementRuleRepresentation> batch;
	{
		auto start = std::chrono::steady_clock::now();
		for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
		{
			auto start = std::chrono::steady_clock::now();
			for(std::size_t j = 0; j < PorterStemmer::steps[i].size(); j++)
				batch.emplace_back(PorterStemmer::steps[i][j], PorterStemmer::alphabet);
			auto end_rep = std::chrono::steady_clock::now();
			std::cerr << "\telapsed time for creating FSR at step " << i << ": " << std::chrono::duration_cast<Resolution>(end_rep - start) << "\n";
			bm.emplace_back(std::move(batch));
			batch.clear();
			auto end = std::chrono::steady_clock::now();
			std::cerr << "\telapsed time for constructing the bimachine only at step " << i << ": " << std::chrono::duration_cast<Resolution>(end - end_rep) << "\n";
			std::cerr << "\telapsed time for construction at step " << i << ": " << std::chrono::duration_cast<Resolution>(end - start) << "\n\n";
		}
		auto end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for construction: " << std::chrono::duration_cast<Resolution>(end - start) << "\n";
	}
	Word input;
	{
		auto start = std::chrono::steady_clock::now();
		input = readFromFile("/dev/stdin", '\0');
		auto end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for reading: " << std::chrono::duration_cast<Resolution>(end - start) << "\n";
	}
	{
		auto start = std::chrono::steady_clock::now();
		for(std::size_t i = 0; i < PorterStemmer::steps_cnt; i++)
		{
			auto start = std::chrono::steady_clock::now();
			input = bm[i](input);
			auto end = std::chrono::steady_clock::now();
			std::cerr << "\telapsed time for replacing at step " << i << ": " << std::chrono::duration_cast<Resolution>(end - start) << "\n";
		}
		auto end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for replacing: " << std::chrono::duration_cast<Resolution>(end - start) << "\n";
	}
	{
		auto start = std::chrono::steady_clock::now();
		std::cout << input;
		auto end = std::chrono::steady_clock::now();
		std::cerr << "elapsed time for printing: " << std::chrono::duration_cast<Resolution>(end - start) << "\n";
	}
}
catch(const std::exception& e)
{
	std::cerr << e.what() << '\n';
	return 1;
}
