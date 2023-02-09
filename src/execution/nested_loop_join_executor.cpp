//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include "binder/table_ref/bound_join_ref.h"
#include "common/exception.h"
#include "type/value_factory.h"
#include "common/logger.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)) {
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2022 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();
  RID _;
  left_omit_ = left_executor_->Next(&left_tuple_, &_);
  matched_ = false;
}

auto NestedLoopJoinExecutor::GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value> {
  std::vector<Value> ret;
  ret.reserve(output_schema->GetColumnCount());
  for (const Column &col : output_schema->GetColumns()) {
    Value val = tuple->GetValue(output_schema, output_schema->GetColIdx(col.GetName()));
    ret.push_back(val);
  }
  return ret;
}

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple right_tuple;
  while (left_omit_) {
    if (!right_executor_->Next(&right_tuple, rid)) {
      right_executor_->Init();
      if (plan_->GetJoinType() == JoinType::LEFT && !matched_) {
        std::vector<Value> left_values = GetValuesFromTuple(&left_tuple_, &left_executor_->GetOutputSchema());
        for (const Column &col : right_executor_->GetOutputSchema().GetColumns()) {
          left_values.emplace_back(ValueFactory::GetNullValueByType(col.GetType()));
        }
        *tuple = Tuple(std::move(left_values), &GetOutputSchema());
        matched_ = true;
        return true;
      }
      left_omit_ = left_executor_->Next(&left_tuple_, rid);
      matched_ = false;
      continue;
    }
    assert(left_omit_);
    auto value = plan_->Predicate().EvaluateJoin(&left_tuple_, left_executor_->GetOutputSchema(), &right_tuple,
                                                 right_executor_->GetOutputSchema());

    if (!value.IsNull() && value.GetAs<bool>()) {
      std::vector<Value> left_values = GetValuesFromTuple(&left_tuple_, &left_executor_->GetOutputSchema());
      std::vector<Value> right_values = GetValuesFromTuple(&right_tuple, &right_executor_->GetOutputSchema());
      left_values.insert(left_values.end(), right_values.begin(), right_values.end());
      *tuple = Tuple(std::move(left_values), &GetOutputSchema());
      matched_ = true;
      return true;
    }
  }
  return false;
}

}  // namespace bustub
