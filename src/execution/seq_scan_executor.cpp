//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void SeqScanExecutor::Init() {
  table_heap_ = GetExecutorContext()->GetCatalog()->GetTable(plan_->GetTableOid())->table_.get();
  table_iter_ = table_heap_->Begin(GetExecutorContext()->GetTransaction());
}
// 按照 output_schema 提取 tuple 中的某些列作为 Values
// 可能是 Projection操作
auto SeqScanExecutor::GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value> {
  std::vector<Value> ret;
  ret.reserve(output_schema->GetColumnCount());
  for (const Column &col : output_schema->GetColumns()) {
    Value val = tuple->GetValue(output_schema, output_schema->GetColIdx(col.GetName()));
    ret.push_back(val);
  }
  return ret;
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (table_iter_ != table_heap_->End()) {
    // 获取RIR
    RID original_rid = table_iter_->GetRid();
    const Schema *output_schema = &(plan_->OutputSchema());

    auto values = GetValuesFromTuple(&(*table_iter_), output_schema);

    ++table_iter_;
    // 不能直接把 TableHeapIterator 返回的 Tuple 作为最终结果输出
    // plan 中的 out_schema 可能仅仅是 TableHeapIterator 返回的 Tuple 的一个 projection (或者说返回 Tuple 的某些列)
    Tuple temp_tuple(std::move(values), output_schema);

    // // predicate检查所有的列比较合理
    // if (plan_->filter_predicate_ == nullptr ||
    // 		plan_->filter_predicate_->Evaluate(&temp_tuple, *output_schema).GetAs<bool>()) {
    *tuple = temp_tuple;
    *rid = original_rid;
    return true;
    // }
  }
  return false;
}

}  // namespace bustub
