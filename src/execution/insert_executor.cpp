//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"
#include "type/type_id.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void InsertExecutor::Init() {
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
/**
 * Dyy:
 * We lock the tuple after insertion. Since only after insertion can
 * we know the RID of that tuple. And we still need to lock the
 * tuple for isolation reason. We can't lock the tuple after the page
 * latch is released because as soon as the page latch released, other
 * txn is able to lock teh inserted tuple.
 * However, this may cause undetectable deadlock
 */

auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  if (used_) {
    return false;
  }
  used_ = true;
  int cnt = 0;
  while (child_executor_->Next(tuple, rid)) {
    table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
    /** After Insert 加行锁 X */
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

    for (auto index_info : table_indexes_) {
      const auto key =
          tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->InsertEntry(key, *rid, GetExecutorContext()->GetTransaction());
      txn->GetIndexWriteSet()->emplace_back(*rid, oid, WType::INSERT, *tuple, index_info->index_oid_,
                                            GetExecutorContext()->GetCatalog());
    }
    cnt++;
  }
  Schema ret_schema{std::vector<Column>{Column("__bustub_internal.insert_rows", TypeId::INTEGER)}};
  *tuple = Tuple{std::vector<Value>{Value(TypeId::INTEGER, cnt)}, &ret_schema};
  return true;
}

}  // namespace bustub
