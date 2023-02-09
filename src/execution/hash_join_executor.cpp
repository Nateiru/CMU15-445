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
  hash_key_ = Value();
  Tuple tuple;
  RID rid;
  while (left_child_->Next(&tuple, &rid)) {
    Value value = plan_->LeftJoinKeyExpression().Evaluate(&tuple, left_child_->GetOutputSchema());
    hht_.Insert(value, tuple);
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
  while (true) {
    while (hht_.Count(hash_key_) == 0) {
      if (!right_child_->Next(tuple, rid)) {
        return false;
      }
      hash_key_ = plan_->RightJoinKeyExpression().Evaluate(tuple, right_child_->GetOutputSchema()); 
    }
    assert(hht_.Count(hash_key_) > 0);

    Tuple left_tuple = hht_.Pop(hash_key_);
    auto left_values = GetValuesFromTuple(&left_tuple, &left_child_->GetOutputSchema());
    auto right_values = GetValuesFromTuple(tuple, &right_child_->GetOutputSchema());
    left_values.insert(left_values.end(), right_values.begin(), right_values.end());
    *tuple = Tuple(std::move(left_values), &GetOutputSchema());
    return true;
  }

}

}  // namespace bustub
