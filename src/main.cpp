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
  if (argc < 2) {
    cout << "Requires an input file" << endl;
    return 1;
  }
  auto clauses = read_dimacs(argv[1]);
  cout << "Load complete" << endl;
  Problem problem;
  for (const auto& clause : clauses) {
    problem.add_clause(clause);
  }
  cout << "Clauses added: " << problem.clauses.size() << endl;
  problem.global_knowledge.print();
  problem.sanity_check();
  cout << problem.global_knowledge.assigned.size() << " " << problem.global_knowledge.rewrites.size() << endl;
  size_t real_variables = 0;
  for (const auto bin : problem.variable_to_clause_index) {
    for (const auto i : bin) {
      if (problem.subsumed_by[i] >= problem.clauses.size()) {
        real_variables++;
        break;
      }
    }
  }
  cout << "Real Variables: " << real_variables << endl;
  /*
  unordered_set<size_t> has_true, has_false;;
  for (size_t i=0; i < problem.clauses.size(); i++) {
    if (problem.can_be_removed[i]) {
      continue;
    }
    for (const auto l : problem.clauses[i]) {
      if (l > 0) {
        has_true.insert(abs(l));
      } else {
        has_false.insert(abs(l));
      }
    }
  }
  for (const auto v : has_true) {
    if (has_false.count(v) == 0) {
      cout << v << " is never false, but sometimes true" << endl;
    }
  }
  for (const auto v : has_false) {
    if (has_true.count(v) == 0) {
      cout << v << " is never true, but sometimes false" << endl;
    }
  }
  */
  ofstream output("tmp.cnf");
  problem.print(output);
  /*
  for (const auto c : problem.clauses) {
    bool important = false;
    for (const auto l : c) {
      size_t variable = abs(l);
      if (variable == 464 or variable == 393) {
        important = true;
      }
    }
    if (important and c.size() < 7) {
      print(c);
    }
  }
  //*/
  //problem.print();
}
