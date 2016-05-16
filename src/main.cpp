#include <iostream>
using std::cout;
using std::endl;
using std::string;
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <sstream>
using std::istringstream;
#include <vector>
using std::vector;
#include <cassert>
#include <algorithm>
using std::pair;
#include <unordered_set>
using std::unordered_set;


#include <chrono>


#include "Problem.h"

vector<vector<int>> read_dimacs(const string filename) {
  ifstream in(filename);
  string line, word;
  int literal;
  vector<vector<int>> clauses;
  while (getline(in, line)) {
    istringstream iss(line);
    if (line.empty() or line[0] == 'c') {
      continue;
    }
    else if (line[0] == 'p') {
      // Process the header
      iss >> word;
      assert(word == "p");
      iss >> word;
      assert(word == "cnf");
      continue;
    }
    // If you got this far, its to process a cnf
    vector<int> clause;
    while (iss >> literal) {
      if (literal != 0) {
        // If the literal is negated, having a true makes this clause false.
        clause.push_back(literal);
      }
    }
    assert(clause.size() > 0);
    clauses.push_back(clause);
  }
  return clauses;
}

int main(int argc, char * argv[]) {
  if (argc < 3) {
    cout << "Requires an input file and output file" << endl;
    return 1;
  }
  auto clauses = read_dimacs(argv[1]);
  cout << "Load complete" << endl;
  auto start_time = std::chrono::steady_clock::now();
  double duration = 0;
  Problem problem;
  for (const auto& clause : clauses) {
    problem.add_clause(clause);
  }
  cout << "Clauses added: " << problem.clauses.size() << endl;
  problem.global_knowledge.print_short();
  const size_t total_variables = problem.variable_to_clause_index.size();

  unordered_map<size_t, size_t> frequency;
  for (size_t v=1; v < total_variables; v++) {
    int positive=0, negative=0;
    for (const auto i : problem.variable_to_clause_index[v]) {
      if (problem.subsumed_by[i] >= problem.clauses.size()) {
        for (const auto l : problem.clauses[i]) {
          size_t var = abs(l);
          if (var == v) {
            if (l > 0) {
              positive++;
            } else {
              negative++;
            }
          }
        }
      }
    }
    frequency[v] = abs(positive-negative);
  }
  vector<vector<size_t>> bins;
  for (size_t v=1; v < total_variables; v++) {
    auto count = frequency[v];
    if (count >= bins.size()) {
      bins.resize(count + 1);
    }
    bins[count].push_back(v);
  }
  while (bins.size() > 0) {
    size_t variable = bins.back().back();
    bins.back().pop_back();
    while (bins.size() > 0 and bins.back().size() == 0) {
      bins.pop_back();
    }
    if (duration > 60) {
      cout << "Took too long" << endl;
      break;
    }
    problem.global_knowledge.print_short();
    problem.assume_and_learn(variable);
    duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
  }
  problem.sanity_check();
  problem.global_knowledge.print_short();
  assert(problem.backups.size() == 0);
  ofstream output(argv[2]);
  problem.print(output);
}
