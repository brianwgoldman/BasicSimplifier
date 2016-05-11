/*
 * Problem.cpp
 *
 *  Created on: May 9, 2016
 *      Author: goldman
 */

#include "Problem.h"

using std::cout;
using std::endl;


void print(const vector<int>& clause, std::ostream& out) {
  for (const auto l : clause) {
    out << l << " ";
  }
  out << 0 << endl;
}


size_t number_of_signs_different(const vector<int>& first, const vector<int>& second) {
  assert(first.size() == second.size());
  size_t result=0;
  const size_t limit = first.size();
  for (size_t i=0; i < limit; i++) {
    result += (first[i] != second[i]);
    // TODO Consider something that stops after 2 differences
    assert(abs(first[i]) == abs(second[i]));
  }
  return result;
}


void Problem::add_clause(const vector<int>& org_clause) {
  auto clause = global_knowledge.simplify(org_clause, global_knowledge.is_unsat);
  if (clause.size() == 0) {
    if (global_knowledge.is_unsat) {
      //print(global_knowledge.assigned);

      cout << "UNSAT!" << endl;
      ::print(org_clause);
      global_knowledge.is_unsat = false;
      global_knowledge.print();
      assert(false);
    }
    return;
  }
  if (clause.size() == 1) {
    //cout << "Singleton: " << clause[0] << endl;
    //::print(org_clause);
    //global_knowledge.is_unsat = false;
    //global_knowledge.print();
    // Assign the variable and update affected clauses
    update_variables(global_knowledge.add(abs(clause[0]), clause[0] > 0));
    return;
  }
  // lowest to highest by variable
  sort(clause.begin(), clause.end(), [](const int l1, const int l2) {
    return abs(l1) < abs(l2);
  });
  // filter out duplicate clauses
  if (clause_to_index.count(clause)) {
    return;
  }
  vector<size_t> clause_key;
  for (const auto l : clause) {
    clause_key.push_back(abs(l));
  }
  bool clause_can_be_removed = false;
  vector<vector<int>> created_clauses;
  // For each clause with exactly the same variables
  for (const auto i : variable_set_to_clause_indices[clause_key]) {
    size_t differences = number_of_signs_different(clause, clauses[i]);
    // Only unique clauses should get this far
    assert(differences > 0);
    // If you found an alias
    if (differences == 2 and clause.size() == 2) {
      //cout << "Alias found" << endl;
      //::print(clause);
      //::print(clauses[i]);
      bool negated = (clause[0] > 0) == (clause[1] > 0);
      //cout << "Negated: " << negated << endl;
      TwoConsistency rewrite(abs(clause[0]), abs(clause[1]), negated);
      update_variables(global_knowledge.add(rewrite));
      return;
    }
    if (differences == 1) {
      //cout << "Simplification found" << endl;
      // Even though you found one, there may be more, so it still gets added
      clause_can_be_removed = true;
      can_be_removed[i] = true;
      created_clauses.emplace_back();
      for (size_t j=0; j < clause.size(); j++) {
        if (clause[j] == clauses[i][j]) {
          created_clauses.back().push_back(clause[j]);
        }
      }
      if (created_clauses.size() > 1) {
        ::print(clause);
        ::print(clauses[i]);
        ::print(created_clauses.back());
      }
    }
  }
  size_t position = clauses.size();
  // If you got this far it means the clause should be kept, at least for now
  can_be_removed.push_back(clause_can_be_removed);
  clause_to_index[clause] = position;
  clauses.push_back(clause);
  variable_set_to_clause_indices[clause_key].push_back(position);

  for (const auto l : clause) {
    size_t variable = abs(l);
    if (variable >= variable_to_clause_index.size()) {
      variable_to_clause_index.resize(variable + 1);
    }
    variable_to_clause_index[variable].push_back(position);
  }
  for (const auto clause : created_clauses) {
    add_clause(clause);
  }
}

void Problem::update_variables(const unordered_set<size_t>& variables) {
  unordered_set<size_t> affected_clauses;
  for (const auto v : variables) {
    if (v >= variable_to_clause_index.size()) {
      continue;
    }
    const auto& block = variable_to_clause_index[v];
    affected_clauses.insert(block.begin(), block.end());
  }
  for (const auto i : affected_clauses) {
    can_be_removed[i] = true;
    // Adding the clause again will show how it looks after knowledge is applied.
    add_clause(clauses[i]);
  }
}

void Problem::print(std::ostream& out) const {
  size_t max_variable = 0;
  size_t real_clauses = 0;
  for (size_t i=0; i < clauses.size(); i++) {
    if (not can_be_removed[i]) {
      real_clauses++;
      size_t clause_max = abs(clauses[i].back());
      if (clause_max > max_variable) {
        max_variable = clause_max;
      }
    }
  }
  out << "p cnf " << max_variable << " " << real_clauses << endl;
  for (size_t i=0; i < clauses.size(); i++) {
    if (can_be_removed[i]) {
      continue;
    }
    ::print(clauses[i], out);
  }
}

void Problem::sanity_check() const {
  assert(clauses.size() == can_be_removed.size());
  assert(clauses.size() == clause_to_index.size());
  assert(variable_set_to_clause_indices.size() <= clauses.size());
  for (const auto pair : clause_to_index) {
    assert(pair.first == clauses[pair.second]);
  }
  for (size_t v=0; v < variable_to_clause_index.size(); v++) {
    for (const auto i : variable_to_clause_index[v]) {
      bool contains = false;
      for (const auto l : clauses[i]) {
        if (abs(l) == v) {
          contains = true;
          break;
        }
      }
      assert(contains);
    }
  }
}
