//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/table/tuple.h"
#include "common/util/hash_util.h"


namespace bustub {
struct HashJoinKey {
  Value value_;
  auto operator==(const HashJoinKey &other) const -> bool {
    return (value_.CompareEquals(other.value_) == CmpBool::CmpTrue);
  }
};

struct HashJoinValue {
  HashJoinValue () {
    std::vector<Tuple>().swap(tuples_);
  }
  std::vector<Tuple> tuples_;
};
}   // namespace bustub

namespace std{
  
/** Implements std::hash on HashJoinKey */
template <>
struct hash<bustub::HashJoinKey> {
  auto operator()(const bustub::HashJoinKey &hash_join_key) const -> std::size_t {
    size_t curr_hash = 0;
    if (!hash_join_key.value_.IsNull()) {
      curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&hash_join_key.value_));
    }
    return curr_hash;
  }
};

}  // namespace std

namespace bustub {

/**
 * A simplified hash table that has all the necessary functionality for hashjoin.
 */

class SimpleHashJoinHashTable {
 public:
  /**
   * Inserts a value into the hash table
   */
  void Insert(const Value &value, const Tuple &tuple) {
    HashJoinKey hash_join_key{value};
    if (ht_.count(hash_join_key) == 0) {
      ht_.insert({hash_join_key, HashJoinValue()});
    }
    ht_[hash_join_key].tuples_.emplace_back(tuple);
  }
  /**
   * clear the hash table
   */
  void Clear() {
    ht_.clear();
    matched_.clear();
  }
  /**
   * count the hash table in hash_join_key
   */
  auto Count(const Value &value) -> size_t {
    HashJoinKey hash_join_key{value};
    if (ht_.count(hash_join_key) == 0) {
      return 0;
    }
    return ht_[hash_join_key].tuples_.size();
  }
  /**
   * Scan the hash table in hash_join_key 
   */
  auto Scan(const Value &value) -> std::vector<Tuple> {
    HashJoinKey hash_join_key{value};
    std::vector<Tuple> ret;
    ret.reserve(ht_[hash_join_key].tuples_.size());
    for (auto it : ht_[hash_join_key].tuples_) {
      ret.emplace_back(it);
    }
    matched_[hash_join_key] = true;
    return ret;
  }
  /**
   * Unmatched the hash table in hash_join_key 
   */
  auto UnMatched() -> std::vector<Tuple> {
    std::vector<Tuple> ret;
    for (const auto &it : ht_) {
      if (!matched_.count(it.first)) {
        ret.insert(ret.begin(), it.second.tuples_.begin(), it.second.tuples_.end());
      }
    }
    return ret;
  }

 private:
  /** The hash table is values */
  std::unordered_map<HashJoinKey, HashJoinValue> ht_{};
  std::unordered_map<HashJoinKey, bool> matched_{};
};

/**
 * HashJoinExecutor executes a nested-loop JOIN on two tables.
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  /**
   * Construct a new HashJoinExecutor instance.
   * @param exec_ctx The executor context
   * @param plan The HashJoin join plan to be executed
   * @param left_child The child executor that produces tuples for the left side of join
   * @param right_child The child executor that produces tuples for the right side of join
   */
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                   std::unique_ptr<AbstractExecutor> &&left_child, std::unique_ptr<AbstractExecutor> &&right_child);

  /** Initialize the join */
  void Init() override;

  /**
   * Yield the next tuple from the join.
   * @param[out] tuple The next tuple produced by the join.
   * @param[out] rid The next tuple RID, not used by hash join.
   * @return `true` if a tuple was produced, `false` if there are no more tuples.
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return The output schema for the join */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

  auto GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value>;

 private:
  /** The NestedLoopJoin plan node to be executed. */
  const HashJoinPlanNode *plan_;
  std::unique_ptr<AbstractExecutor> left_child_;
  std::unique_ptr<AbstractExecutor> right_child_;
  SimpleHashJoinHashTable hht_;
  std::vector<Tuple> ret_tuples_;
};

}  // namespace bustub
