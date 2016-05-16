/*
 * Problem.cpp
 *
 *  Created on: May 9, 2016
 *      Author: goldman
 */

#include "Problem.h"

using std::cout;
using std::endl;
using std::swap;

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

vector<size_t> Problem::create_key(const vector<int>& clause) const {
  // Assumes the clause is already sorted
  vector<size_t> clause_key;
  for (const auto l : clause) {
    clause_key.push_back(abs(l));
  }
  return clause_key;
}

bool is_superset(const vector<int>& sub, const vector<int>& super) {
  if (sub.size() > super.size()) {
    return false;
  }
  size_t super_it = 0;
  const size_t super_size = super.size();
  for (const auto l : sub) {
    auto variable = abs(l);
    while (super_it < super_size and abs(super[super_it]) < variable) {
      super_it++;
    }
    if (super_it >= super_size or l != super[super_it]) {
      return false;
    }
  }
  return true;
}

void Problem::add_clause(const vector<int>& org_clause, bool remove_subsets) {
  if (global_knowledge.is_unsat) {
    cout << "Add clause called while already unsat" << endl;
  }
  auto clause = global_knowledge.simplify(org_clause, global_knowledge.is_unsat);
  if (clause.size() == 0) {
    // Don't do anything with empty clauses.
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
  auto clause_key = create_key(clause);
  // at this point the clause is assured to be added, so just add it
  size_t position = direct_add_clause(clause, clause_key);
  if (clause.size() == 1) {
    // Assign the variable and update affected clauses
    update_variables(global_knowledge.add(abs(clause[0]), clause[0] > 0));
    return;
  }

  vector<vector<int>> created_clauses;
  // For each clause with exactly the same variables
  for (const auto i : variable_set_to_clause_indices[clause_key]) {
    if (i == position) {
      continue;
    }
    size_t differences = number_of_signs_different(clause, clauses[i]);
    // Only unique clauses should get this far
    assert(differences > 0);
    // If you found an alias
    if (differences == 2 and clause.size() == 2) {
      subsume(i, position);
      subsume(position, position);
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
      subsume(i, position);
      subsume(position, position);
      created_clauses.emplace_back();
      for (size_t j=0; j < clause.size(); j++) {
        if (clause[j] == clauses[i][j]) {
          created_clauses.back().push_back(clause[j]);
        }
      }
    }
  }
  if (remove_subsets) {
    for (const auto i : variable_to_clause_index[abs(clause[0])]) {
      if (i != position and subsumed_by[i] >= clauses.size()) {
        if (is_superset(clause, clauses[i])) {
          subsume(i, position);
        }
      }
    }
  }
  for (const auto to_add : created_clauses) {
    if (global_knowledge.is_unsat) {
      break;
    }
    add_clause(to_add);
  }
}

size_t Problem::direct_add_clause(const vector<int>& clause, const vector<size_t>& key) {
  // Assumes its already sorted
  const size_t position = clauses.size();
  // If you got this far it means the clause should be kept, at least for now
  subsumed_by.push_back(-1);
  subsumes.emplace_back();
  clause_to_index[clause] = position;
  clauses.push_back(clause);
  variable_set_to_clause_indices[key].insert(position);

  for (const auto l : clause) {
    size_t variable = abs(l);
    if (variable >= variable_to_clause_index.size()) {
      variable_to_clause_index.resize(variable + 1);
    }
    variable_to_clause_index[variable].insert(position);
  }
  return position;
}

void Problem::update_variables(const unordered_set<size_t>& variables) {
  assert(clauses.size() > 0);
  unordered_set<size_t> affected_clauses;
  for (const auto v : variables) {
    if (v >= variable_to_clause_index.size()) {
      continue;
    }
    const auto& block = variable_to_clause_index[v];
    affected_clauses.insert(block.begin(), block.end());
  }
  for (const auto i : affected_clauses) {
    if (subsumed_by[i] < clauses.size()) {
      // Ignore clauses that can be removed
      continue;
    }
    if (global_knowledge.is_unsat) {
      return;

    }
    subsume(i, clauses.size() - 1);
    // Adding the clause again will show how it looks after knowledge is applied.
    add_clause(clauses[i]);
  }
}

bool Problem::subsume(const size_t i, const size_t j) {
  if (subsumed_by[i] > j) {
    subsumed_by[i] = j;
    subsumes[j].push_back(i);
    return true;
  }
  return false;
}

void Problem::create_backup() {
  backups.emplace_back();
  backups.back().clause_size = clauses.size();
  swap(backups.back().known, global_knowledge);
}

void Problem::revert_to_backup() {
  if (backups.size() == 0) {
    cout << "Attempted to revert to backup when no backup exists" << endl;
    assert(false);
  }
  const size_t old_size = backups.back().clause_size;
  for (size_t i= old_size; i < clauses.size(); i++) {
    // Reset subsumes_by clauses
    for (const auto j : subsumes[i]) {
      assert(subsumed_by[j] == i);
      subsumed_by[j] = -1;
    }
    // Remove clauses from variable_to_clause_index
    for (const auto l : clauses[i]) {
      size_t variable = abs(l);
      size_t removed = variable_to_clause_index[variable].erase(i);
      if (removed != 1) {
        cout << "Removed: " << removed << " when removing " << i << " from " << variable << endl;
        ::print(clauses[i]);
        assert(false);
      }
      assert(removed == 1);
    }
    auto key = create_key(clauses[i]);
    // Remove clause from variable_set_to_clause_indices
    auto it = variable_set_to_clause_indices.find(key);
    size_t removed = it->second.erase(i);
    if (it->second.empty()) {
      variable_set_to_clause_indices.erase(it);
    }
    assert(removed == 1);
    // Remove clause from clause_to_index
    removed = clause_to_index.erase(clauses[i]);
    assert(removed == 1);
  }
  subsumed_by.resize(old_size);
  subsumes.resize(old_size);
  clauses.resize(old_size);

  // Revert the knowledge and clear the backup
  swap(backups.back().known, global_knowledge);
  backups.pop_back();

}

void Problem::assume_and_learn(size_t variable) {
  // TODO Verify all of the knowledge based things get preserved
  create_backup();
  // First assume it positive
  vector<int> fake(1, variable);
  add_clause(fake);
  if (global_knowledge.is_unsat) {
    cout << "Contradiction learning" << endl;
    revert_to_backup();
    fake[0] = -fake[0];
    add_clause(fake);
    return;
  }
  // Build the set of the clauses created by this assumption
  unordered_set<vector<int>> created;
  vector<vector<int>> created_singles;
  for (size_t i=backups.back().clause_size; i < clauses.size(); i++) {
    created.insert(clauses[i]);
    if (clauses[i].size() == 1) {
      created_singles.push_back(clauses[i]);
    }
  }
  // make the opposite assumption
  revert_to_backup();

  create_backup();
  fake[0] = - fake[0];
  add_clause(fake);
  if (global_knowledge.is_unsat) {
    cout << "Contradiction learning" << endl;
    revert_to_backup();
    fake[0] = -fake[0];
    add_clause(fake);
    return;
  }

  vector<vector<int>> second_singles;
  vector<vector<int>> to_add;
  for (size_t i=backups.back().clause_size; i < clauses.size(); i++) {
    // If this clause was created going both directions
    if (created.count(clauses[i])) {
      to_add.push_back(clauses[i]);
    }
    if (clauses[i].size() == 1) {
      second_singles.push_back(clauses[i]);
    }
  }
  if (to_add.size() > 0) {
    cout << "Created both ways" << endl;
    for (const auto c : to_add) {
      ::print(c);
    }
  }
  revert_to_backup();
  // TODO Consider handling single matching
  /*
  cout << " Doing " << created_singles.size() << "x" << second_singles.size() << endl;
  for (const auto second : second_singles) {
    // Pair of all singles
    for (const auto& single : created_singles) {
      if (abs(single[0]) != abs(second[0])) {
        to_add.push_back({single[0], second[0]});
        // Prevent memory issues
        if(to_add.size() > clauses.size()) {
          cout << "Cut off" << endl;
          break;
        }
      }
    }
  }
  //*/
  /*
  for (const auto single : created_singles) {
    if (abs(single[0]) == abs(fake[0])) {
      continue;
    }
    to_add.push_back({single[0], fake[0]});
  }
  for (const auto single : second_singles) {
    if (abs(single[0]) == abs(fake[0])) {
      continue;
    }
    to_add.push_back({single[0], -fake[0]});
  }
  //*/
  for (const auto clause : to_add) {
    add_clause(clause, true);
  }
  if (to_add.size() > 0) {
    cout << "Added: " << to_add.size() << " clauses" << endl << endl << endl;
    cout << clauses.size() << endl;
  }
}

void Problem::print(std::ostream& out) const {
  size_t max_variable = 0;
  size_t real_clauses = 0;
  for (size_t i=0; i < clauses.size(); i++) {
    if (subsumed_by[i] >= clauses.size()) {
      real_clauses++;
      size_t clause_max = abs(clauses[i].back());
      if (clause_max > max_variable) {
        max_variable = clause_max;
      }
    }
  }
  out << "c Assigned: " << global_knowledge.assigned.size()
      << " Rewritten: " << global_knowledge.rewrites.size() << endl;
  out << "p cnf " << max_variable << " " << real_clauses << endl;
  for (size_t i=0; i < clauses.size(); i++) {
    if (subsumed_by[i] < clauses.size()) {
      continue;
    }
    ::print(clauses[i], out);
  }
}

bool Problem::is_removable(const size_t variable) const {
  return false;
}

void Problem::sanity_check() const {
  assert(clauses.size() == subsumed_by.size());
  assert(clauses.size() == clause_to_index.size());
  assert(clauses.size() == subsumes.size());
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
