//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  table_info_ = GetExecutorContext()->GetCatalog()->GetTable(plan_->TableOid());
  table_indexes_ = GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info_->name_);
  used_ = false;
  child_executor_->Init();
  /** 加表锁 IX */
  LockManager *lock_mgr = GetExecutorContext()->GetLockManager();
  Transaction *txn = GetExecutorContext()->GetTransaction();
  table_oid_t oid = plan_->TableOid();
  if (lock_mgr != nullptr) {
    // 已经加过能升级的锁
    if (!txn->IsTableIntentionExclusiveLocked(oid) && !txn->IsTableExclusiveLocked(oid) &&
        !txn->IsTableSharedIntentionExclusiveLocked(oid)) {
      try {
        lock_mgr->LockTable(txn, LockManager::LockMode::INTENTION_EXCLUSIVE, oid);
      } catch (TransactionAbortException &e) {
        throw ExecutionException(e.GetInfo());
      }
    }
  }
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  if (used_) {
    return false;
  }
  used_ = true;
  int cnt = 0;
  while (child_executor_->Next(tuple, rid)) {
    /** Before Delete 加行锁 X */
    LockManager *lock_mgr = GetExecutorContext()->GetLockManager();
    Transaction *txn = GetExecutorContext()->GetTransaction();
    table_oid_t oid = plan_->TableOid();
    if (lock_mgr != nullptr) {
      if (!txn->IsRowExclusiveLocked(oid, *rid)) {
        try {
          lock_mgr->LockRow(txn, LockManager::LockMode::EXCLUSIVE, oid, *rid);
        } catch (TransactionAbortException &e) {
          throw ExecutionException(e.GetInfo());
        }
      }
    }
    table_info_->table_->MarkDelete(*rid, GetExecutorContext()->GetTransaction());
    for (auto index_info : table_indexes_) {
      const auto key =
          tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->DeleteEntry(key, *rid, GetExecutorContext()->GetTransaction());
      txn->GetIndexWriteSet()->emplace_back(*rid, oid, WType::DELETE, *tuple, index_info->index_oid_,
                                            GetExecutorContext()->GetCatalog());
    }
    ++cnt;
  }
  Schema output_schema{std::vector<Column>{Column{"__bustub_internal.delete_rows", TypeId::INTEGER}}};
  *tuple = Tuple{std::vector<Value>{Value(TypeId::INTEGER, cnt)}, &output_schema};
  return true;
}

}  // namespace bustub
