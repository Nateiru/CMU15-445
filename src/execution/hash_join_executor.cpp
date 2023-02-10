//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include "type/value_factory.h"

// Note for 2022 Fall: You don't need to implement HashJoinExecutor to pass all tests. You ONLY need to implement it
// if you want to get faster in leaderboard tests.

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx), plan_(plan), left_child_(std::move(left_child)), right_child_(std::move(right_child)){

  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2022 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void HashJoinExecutor::Init() {
  left_child_->Init();
  right_child_->Init();
  hht_.Clear();
  Value hash_key;
  std::vector<Tuple>().swap(ret_tuples_);
  bool reordered = GetOutputSchema().GetColumn(0).GetName() == right_child_->GetOutputSchema().GetColumn(0).GetName();

  Tuple tuple;
  RID rid;
  // bulid hashmap using left data
  while (left_child_->Next(&tuple, &rid)) {
    Value value = plan_->LeftJoinKeyExpression().Evaluate(&tuple, left_child_->GetOutputSchema());
    hht_.Insert(value, tuple);
  }
  // Probe
  while (right_child_->Next(&tuple, &rid)) {

    hash_key = plan_->RightJoinKeyExpression().Evaluate(&tuple, right_child_->GetOutputSchema()); 
    if (hht_.Count(hash_key) == 0) {
      continue;
    } 
    assert(hht_.Count(hash_key) > 0);

    auto left_tuples = hht_.Scan(hash_key);
    auto right_values = GetValuesFromTuple(&tuple, &right_child_->GetOutputSchema());
    for (const auto & left_tuple : left_tuples) {
      auto left_values = GetValuesFromTuple(&left_tuple, &left_child_->GetOutputSchema()); 
      if (reordered) {
        assert(plan_->GetJoinType() == JoinType::INNER);
        size_t right_size = right_values.size();
        right_values.insert(right_values.end(), left_values.begin(), left_values.end());
        ret_tuples_.emplace_back(Tuple(right_values, &GetOutputSchema()));
        right_values.erase(right_values.begin() + right_size, right_values.end());
      }else {
        left_values.insert(left_values.end(), right_values.begin(), right_values.end());
        ret_tuples_.emplace_back(Tuple(left_values, &GetOutputSchema()));
      }
    }
  }
  // LeftJoin: Unmatched
  if (plan_->GetJoinType() == JoinType::LEFT) {
    assert(!reordered);
    auto left_tuples = hht_.UnMatched();
    if (left_tuples.empty()) {
      return;
    }
    std::vector<Value> right_values;
    for (const Column &col : right_child_->GetOutputSchema().GetColumns()) {
      right_values.emplace_back(ValueFactory::GetNullValueByType(col.GetType()));
    }
    for (const auto & left_tuple : left_tuples) {
      auto left_values = GetValuesFromTuple(&left_tuple, &left_child_->GetOutputSchema()); 
      left_values.insert(left_values.end(), right_values.begin(), right_values.end());
      ret_tuples_.emplace_back(Tuple(left_values, &GetOutputSchema()));
    }
  }
}
auto HashJoinExecutor::GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value> {
  std::vector<Value> ret;
  ret.reserve(output_schema->GetColumnCount());
  for (const Column &col : output_schema->GetColumns()) {
    Value val = tuple->GetValue(output_schema, output_schema->GetColIdx(col.GetName()));
    ret.push_back(val);
  }
  return ret;
}

auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (!ret_tuples_.empty()) {
    *tuple = ret_tuples_.back();
    ret_tuples_.pop_back();
    *rid = tuple->GetRid();
    return true;
  } 
  return false;
}

}  // namespace bustub
