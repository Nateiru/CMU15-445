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

  /** 加表锁 IS */
  LockManager *lock_mgr = GetExecutorContext()->GetLockManager();
  Transaction *txn = GetExecutorContext()->GetTransaction();
  table_oid_t oid = plan_->GetTableOid();
  if (lock_mgr != nullptr) {
    if (txn->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED) {
      // 已经加过能升级的锁
      if (!txn->IsTableIntentionSharedLocked(oid) && !txn->IsTableSharedLocked(oid) &&
          !txn->IsTableIntentionExclusiveLocked(oid) && !txn->IsTableExclusiveLocked(oid) &&
          !txn->IsTableSharedIntentionExclusiveLocked(oid)) {
        try {
          lock_mgr->LockTable(txn, LockManager::LockMode::INTENTION_SHARED, oid);
        } catch (TransactionAbortException &e) {
          throw ExecutionException(e.GetInfo());
        }
      }
    }
  }
}
// 按照 output_schema 提取 tuple 中的某些列作为 Values
// 可能是 Projection 操作
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

    /** 加行锁 S*/
    LockManager *lock_mgr = GetExecutorContext()->GetLockManager();
    Transaction *txn = GetExecutorContext()->GetTransaction();
    table_oid_t oid = plan_->GetTableOid();
    bool is_lock = false;
    if (lock_mgr != nullptr) {
      if (txn->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED) {
        if (!txn->IsRowSharedLocked(oid, original_rid) && !txn->IsRowExclusiveLocked(oid, original_rid)) {
          try {
            lock_mgr->LockRow(txn, LockManager::LockMode::SHARED, oid, original_rid);
          } catch (TransactionAbortException &e) {
            throw ExecutionException(e.GetInfo());
          }
          is_lock = true;
        }
      }
    }
    auto values = GetValuesFromTuple(&(*table_iter_), output_schema);
    /** 解锁 */
    if (lock_mgr != nullptr) {
      // COMMITTED 读锁不影响状态 立即释放
      if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
        if (is_lock && txn->IsRowSharedLocked(oid, original_rid) && !txn->IsRowExclusiveLocked(oid, original_rid)) {
          try {
            lock_mgr->UnlockRow(txn, oid, original_rid);
          } catch (TransactionAbortException &e) {
            throw ExecutionException(e.GetInfo());
          }
        }
      }
    }
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
  }
  return false;
}

}  // namespace bustub
