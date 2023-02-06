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
  while (aht_iterator_ != aht_.End()) {
    std::vector<Value> values;
    for (const auto & value : aht_iterator_.Key().group_bys_) {
      values.emplace_back(value);
    }
    for (const auto & value : aht_iterator_.Val().aggregates_) {
      values.emplace_back(value);
    }
    *tuple = Tuple(std::move(values), &plan_->OutputSchema());
    ++aht_iterator_;
    return true;
  }
  return false;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_.get(); }

}  // namespace bustub
