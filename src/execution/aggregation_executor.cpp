//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_(std::move(child)),
      aht_{plan->GetAggregates(), plan->GetAggregateTypes()},
      aht_iterator_(aht_.Begin()){}

void AggregationExecutor::Init() {

  Tuple temp_tuple;
  RID temp_rid;
  child_->Init();
  while(child_->Next(&temp_tuple, &temp_rid)) {
    aht_.InsertCombine(MakeAggregateKey(&temp_tuple), MakeAggregateValue(&temp_tuple));
  }
  aht_iterator_ = aht_.Begin();
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  
  if (aht_.IsEmpty() && !used_) {
    used_ = true;
    if (!plan_->group_bys_.empty()) {
      return false;
    }
    *tuple = Tuple(std::move(aht_.GenerateInitialAggregateValue().aggregates_), &GetOutputSchema());
    return true;
  }
  while (aht_iterator_ != aht_.End()) {
    std::vector<Value> values;
    for (const auto & value : aht_iterator_.Key().group_bys_) {
      values.emplace_back(value);
    }
    for (const auto & value : aht_iterator_.Val().aggregates_) {
      values.emplace_back(value);
    }

    *tuple = Tuple(std::move(values), &GetOutputSchema());
    ++aht_iterator_;
    return true;
  }
  return false;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_.get(); }

}  // namespace bustub
