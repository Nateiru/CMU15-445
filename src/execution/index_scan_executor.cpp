//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
  auto index_info = GetExecutorContext()->GetCatalog()->GetIndex(plan_->GetIndexOid());
  auto tree = dynamic_cast<BPlusTreeIndexForOneIntegerColumn *>(index_info->index_.get());
  iter_ = tree->GetBeginIterator();
  iter_end_ = tree->GetEndIterator();
  table_heap_ = GetExecutorContext()->GetCatalog()->GetTable(index_info->table_name_)->table_.get();
}
// 按照 output_schema 提取 tuple 中的某些列作为 Values
// 可能是 Projection操作
auto IndexScanExecutor::GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value> {
  std::vector<Value> ret;
	ret.reserve(output_schema->GetColumnCount());
  for (const Column &col : output_schema->GetColumns()) {
    Value val = tuple->GetValue(output_schema, output_schema->GetColIdx(col.GetName()));
    ret.push_back(val);
  }
  return ret;
}
auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (iter_ != iter_end_) {
		*rid = (*iter_).second;
		const Schema *output_schema = &(plan_->OutputSchema());
    table_heap_->GetTuple(*rid, tuple, GetExecutorContext()->GetTransaction());

		auto values = GetValuesFromTuple(tuple, output_schema);
		++iter_;
    *tuple = std::move(Tuple(std::move(values), output_schema));
    return true;
  }
  return false;
}

}  // namespace bustub
