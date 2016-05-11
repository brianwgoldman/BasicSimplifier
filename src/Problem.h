/*
 * Problem.h
 *
 *  Created on: May 9, 2016
 *      Author: goldman
 */

#ifndef PROBLEM_H_
#define PROBLEM_H_
#include "Knowledge.h"

#include <vector>
using std::vector;
using std::unordered_map;


// TODO Actually clean up useless clauses
class Problem {
 public:
  void add_clause(const vector<int>& org_clause);
  void print(std::ostream& out=std::cout) const;
  void sanity_check() const;
 public:
  vector<vector<int>> clauses;
  bool subsume(const size_t i, const size_t j);
  vector<size_t> subsumed_by;
  vector<vector<size_t>> variable_to_clause_index;
  Knowledge global_knowledge;
  unordered_map<vector<int>, size_t> clause_to_index;
  unordered_map<vector<size_t>, vector<size_t>> variable_set_to_clause_indices;
  void update_variables(const unordered_set<size_t>& variables);
  size_t direct_add_clause(const vector<int>& clause, const vector<size_t>& key);
};



template<class T, class U>
void print(const unordered_map<T, U>& map, std::ostream& out=std::cout) {
  vector<std::pair<T, U>> sortable(map.begin(), map.end());
  sort(sortable.begin(), sortable.end());
  for (const auto pair : sortable) {
    out << pair.first << "=" << pair.second << ", ";
  }
  out << std::endl;
}


#endif /* PROBLEM_H_ */
