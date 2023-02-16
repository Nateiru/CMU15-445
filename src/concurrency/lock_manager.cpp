//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lock_manager.cpp
//
// Identification: src/concurrency/lock_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "concurrency/lock_manager.h"

#include "common/config.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"

namespace bustub {

void LockManager::BookKeeping(bool insert, Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID rid) {
  txn->LockTxn();
  // 说明对表加锁
  if (rid.GetPageId() == INVALID_PAGE_ID) {
    if (insert) {
      switch (lock_mode) {
      case LockMode::SHARED:
        txn->GetSharedTableLockSet()->insert(oid);
        break;
      case LockMode::EXCLUSIVE:
        txn->GetExclusiveTableLockSet()->insert(oid);
        break;
      case LockMode::INTENTION_SHARED:
        txn->GetIntentionSharedTableLockSet()->insert(oid);
        break;
      case LockMode::INTENTION_EXCLUSIVE:
        txn->GetIntentionExclusiveTableLockSet()->insert(oid);
        break;
      case LockMode::SHARED_INTENTION_EXCLUSIVE:
        txn->GetSharedIntentionExclusiveTableLockSet()->insert(oid);
        break;
      default:
        assert(0);
        break;
      }
    }
    else {
      switch (lock_mode) {
      case LockMode::SHARED:
        txn->GetSharedTableLockSet()->erase(oid);
        break;
      case LockMode::EXCLUSIVE:
        txn->GetExclusiveTableLockSet()->erase(oid);
        break;
      case LockMode::INTENTION_SHARED:
        txn->GetIntentionSharedTableLockSet()->erase(oid);
        break;
      case LockMode::INTENTION_EXCLUSIVE:
        txn->GetIntentionExclusiveTableLockSet()->erase(oid);
        break;
      case LockMode::SHARED_INTENTION_EXCLUSIVE:
        txn->GetSharedIntentionExclusiveTableLockSet()->erase(oid);
        break;
      default:
        assert(0);
        break;
      }
    }
  }
  else { // 对行加锁
    assert(lock_mode == LockMode::SHARED || lock_mode == LockMode::EXCLUSIVE);
    if (insert) {
      switch (lock_mode) {
      case LockMode::SHARED:
        txn->GetSharedLockSet()->insert(rid);
        if (!txn->GetSharedRowLockSet()->count(oid)) {
          txn->GetSharedRowLockSet()->emplace(oid, std::unordered_set<RID>());
        }
        txn->GetSharedRowLockSet()->at(oid).insert(rid);
        break;
      case LockMode::EXCLUSIVE:
        txn->GetExclusiveLockSet()->insert(rid);
        if (!txn->GetExclusiveRowLockSet()->count(oid)) {
          txn->GetExclusiveRowLockSet()->emplace(oid, std::unordered_set<RID>());
        }
        txn->GetExclusiveRowLockSet()->at(oid).insert(rid);
        break;
      default:
        assert(0);
        break;
      }
    }
    else {
      switch (lock_mode) {
      case LockMode::SHARED:
        txn->GetSharedLockSet()->erase(rid);
        assert(txn->GetSharedRowLockSet()->count(oid) != 0U);
        txn->GetSharedRowLockSet()->at(oid).erase(rid);
        break;
      case LockMode::EXCLUSIVE:
        txn->GetExclusiveLockSet()->erase(rid);
        assert(txn->GetExclusiveLockSet()->count(rid) != 0U);
        txn->GetExclusiveRowLockSet()->at(oid).erase(rid);
        break;
      default:
        assert(0);
        break;
      }
    }
  }
  txn->UnlockTxn();
}

auto LockManager::CheckUpgrade(const LockMode &hold, const LockMode &want) -> bool {
  if (hold == LockMode::INTENTION_SHARED) {
    return want == LockMode::SHARED              || \
           want == LockMode::EXCLUSIVE           || \
           want == LockMode::INTENTION_EXCLUSIVE || \
           want == LockMode::SHARED_INTENTION_EXCLUSIVE;
  }
  if (hold == LockMode::SHARED) {
    return want == LockMode::EXCLUSIVE           || \
           want == LockMode::SHARED_INTENTION_EXCLUSIVE; 
  }
  if (hold == LockMode::INTENTION_EXCLUSIVE) {
    return want == LockMode::EXCLUSIVE           || \
           want == LockMode::SHARED_INTENTION_EXCLUSIVE;
  }
  if (hold == LockMode::SHARED_INTENTION_EXCLUSIVE) {
    return want == LockMode::EXCLUSIVE;
  }
  return false;
}

auto LockManager::CheckCompatibility(const LockMode &hold, const LockMode &want) -> bool {
  if (hold == LockMode::INTENTION_SHARED) {
    return want == LockMode::INTENTION_SHARED    || \
           want == LockMode::INTENTION_EXCLUSIVE || \
           want == LockMode::SHARED              || \
           want == LockMode::SHARED_INTENTION_EXCLUSIVE;
  }
  if (hold == LockMode::INTENTION_EXCLUSIVE) {
    return want == LockMode::INTENTION_SHARED    || \
           want == LockMode::INTENTION_EXCLUSIVE; 
  }
  if (hold == LockMode::SHARED) {
    return want == LockMode::INTENTION_SHARED    || \
           want == LockMode::SHARED;
  }
  if (hold == LockMode::SHARED_INTENTION_EXCLUSIVE) {
    return want == LockMode::INTENTION_SHARED;
  }
  return false;
}

auto LockManager::GrantLock(Transaction *txn, std::shared_ptr<LockManager::LockRequestQueue> &lock_request_queue) -> bool {
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }
  LockRequest *lock_request{nullptr};
  for (auto request : lock_request_queue->request_queue_) {
    if (txn->GetTransactionId() == request->txn_id_) {
      lock_request = request;
      break;
    }
  }
  assert(lock_request != nullptr);
  int flag = -1;
  for (const auto & request : lock_request_queue->request_queue_) {
    if (request->granted_ ) {
      flag = 1;
      if (!CheckCompatibility(request->lock_mode_, lock_request->lock_mode_)) {
        return false;
      }
    }
  }
  // 说明全部兼容
  if (flag != -1) {
    lock_request->granted_ = true;
    // std::cout << "OK Compatibility!!!" <<std::endl;
    return true;
  }
  // 否则说明不存在已经 granted 的请求
  // 升级优先级最高
  if (lock_request_queue->upgrading_ != INVALID_TXN_ID) {
    if (lock_request_queue->upgrading_ == txn->GetTransactionId()) {
      lock_request->granted_ = true;
      return true;
    }
    return false;
  }
  // FIFO
  // 当前请求是第一个 waiting 状态的请求 
  // 前面还存在其他 waiting 请求，则要判断当前请求是否前面的 waiting 请求兼容。
  bool matched = true;
  for (const auto & request : lock_request_queue->request_queue_) {
    if (request->txn_id_ == txn->GetTransactionId()) {
      assert(request->granted_ == false);
      if (matched) {
        lock_request->granted_ = true;
        std::cout <<txn->GetTransactionId() << " Get Lock" <<std::endl;
        return true;  
      }
      return false;
    }
    if (!request->granted_ ) {
      matched &= CheckCompatibility(request->lock_mode_, lock_request->lock_mode_);
    }
  }
  assert(0);
  return false;
}

void LockManager::DeleteInQueue(Transaction *txn, std::shared_ptr<LockManager::LockRequestQueue> &lock_request_queue) {
  assert(txn->GetState() == TransactionState::ABORTED);
  auto &request_queue = lock_request_queue->request_queue_;
  for (auto it = request_queue.begin(); it != request_queue.end(); ++it) {
    auto request = *it;
    if (request->txn_id_ == txn->GetTransactionId()) {
      request_queue.erase(it);
      delete request;
      break;
    }
  }
  if (lock_request_queue->upgrading_ == txn->GetTransactionId()) {
    lock_request_queue->upgrading_ = INVALID_TXN_ID;
  }
  lock_request_queue->cv_.notify_all();
}
auto LockManager::LockTable(Transaction *txn, LockMode lock_mode, const table_oid_t &oid) -> bool {
  /** 1. 检查 txn 的状态。*/

  // 检查当前事务是否被终止
  if (txn->GetState() == TransactionState::ABORTED) {
    std::cout << "transaction has been aborted!" << std::endl;
    // BUSTUB_ENSURE(0, "transaction has been aborted!");
    return false;
  }
  // 事务的隔离级别是 READ_UNCOMITTED，只能加 X 和 IX 锁
  if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
    if (!(lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::INTENTION_EXCLUSIVE)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      return false;
    }
    // 事务状态为 SHRINKING 时不能上锁
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  // 事务的隔离级别是 READ_COMITTED
  if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
    // 事务状态为 SHRINKING 时 只允许 S 和 IS 锁
    if (txn->GetState() == TransactionState::SHRINKING && 
      !(lock_mode == LockMode::SHARED || lock_mode == LockMode::INTENTION_SHARED)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  // 事务的隔离级别是 Repeatable Read
  if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    // 事务状态为SHRINKING时不能上锁
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  /** 2. 获取 table 对应的 lock request queue。*/

  table_lock_map_latch_.lock();
  // std::cout << "Begin Fetch Table Latch: " << oid << std::endl;
  // 若 queue 不存在则创建。
  if (!table_lock_map_.count(oid)) {
    table_lock_map_[oid] = std::make_shared<LockRequestQueue>();
  }
  auto &lock_request_queue = table_lock_map_[oid];
  // 获取 queue 后 立即可以释放 table
  // std::cout << "End Fetch Table Latch: " << oid << std::endl;
  table_lock_map_latch_.unlock();
  // lock_request_queue->latch_.lock();
  std::unique_lock<std::mutex> lock(lock_request_queue->latch_);
  // std::cout << "++++++++++++++++++++++++++"<< std::endl;
  auto &request_queue = lock_request_queue->request_queue_;
  /** 3. 检查此锁请求是否为一次锁升级。*/
  for (auto & request : request_queue) {
    if (request->txn_id_ == txn->GetTransactionId()) {
      // std::cout << "Opps! Upgrade!!!"<<std::endl;
      BUSTUB_ENSURE(request->granted_ == true, "try requesting another lock means that it has been granted a lock!");
      // 判断当前资源上是否有另一个事务正在尝试升级
      if (lock_request_queue->upgrading_ != INVALID_TXN_ID) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
        return false;
      }
      // 判断锁的类型是否相同 相同直接返回
      if (request->lock_mode_ == lock_mode) {
        return true;
      }
      // 尝试升级
      // 1. 判断是否可以升级
      if (!CheckUpgrade(request->lock_mode_, lock_mode)) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
        return false; 
      }
      // 2. 释放当前锁
      BookKeeping(false, txn, request->lock_mode_, request->oid_);
      request->granted_ = false;
      request->lock_mode_ = lock_mode;
      request->oid_ = oid;
      lock_request_queue->upgrading_ = request->txn_id_;
      lock_request_queue->cv_.notify_all();
      // 3. 等待直到新锁被授予。
      while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
        lock_request_queue->cv_.wait(lock);
      }
      if (txn->GetState() == TransactionState::ABORTED) {
        DeleteInQueue(txn, lock_request_queue);
        return false;
      }
      txn->SetState(TransactionState::GROWING);
      BookKeeping(true, txn, lock_mode, oid);
      return true;
    }
  }
  
  /** 4. 将锁请求加入请求队列（此时无锁升级）。*/
  
  lock_request_queue->request_queue_.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid));
  std::cout << "Success Emplace_back: " << txn->GetTransactionId() << std::endl;
  /** 5. 尝试获取锁。*/
  while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
    lock_request_queue->cv_.wait(lock);
  }
  if (txn->GetState() == TransactionState::ABORTED) {
    DeleteInQueue(txn, lock_request_queue);
    return false;
  }
  std::cout << "Success Lock: " << oid << std::endl;
  txn->SetState(TransactionState::GROWING);
  BookKeeping(true, txn, lock_mode, oid);
  return true;
}


auto LockManager::UnlockTable(Transaction *txn, const table_oid_t &oid) -> bool {
  // row 锁必须全部释放
  if ((txn->GetSharedRowLockSet()->count(oid) && !txn->GetSharedRowLockSet()->at(oid).empty()) || \
      (txn->GetExclusiveRowLockSet()->count(oid) && !txn->GetExclusiveRowLockSet()->at(oid).empty())) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::TABLE_UNLOCKED_BEFORE_UNLOCKING_ROWS);
    return false;
  }
  table_lock_map_latch_.lock();
  // 若 queue 不存在。
  if (!table_lock_map_.count(oid)) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
    return false;
  }
  // std::cout << "Begin Unlock " <<std::endl;
  auto &lock_request_queue = table_lock_map_[oid];
  // 获取 queue 后 立即可以释放 table
  table_lock_map_latch_.unlock();
  std::unique_lock<std::mutex> lock(lock_request_queue->latch_);
  auto &request_queue = lock_request_queue->request_queue_;
  // std::cout << "queue size: " << request_queue.size() << std::endl; 
  for (auto it = request_queue.begin(); it != request_queue.end(); ++it) {
    auto request = *it;
    // // std::cout << request->txn_id_ <<std::endl;
    if (request->txn_id_ == txn->GetTransactionId() && request->granted_) {
      // 释放锁
      request_queue.erase(it);
      // 状态变化
      if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        if (txn->GetState() == TransactionState::GROWING && request->lock_mode_ == LockMode::EXCLUSIVE) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
        if (txn->GetState() == TransactionState::GROWING && request->lock_mode_ == LockMode::EXCLUSIVE) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
        if (txn->GetState() == TransactionState::GROWING && 
           (request->lock_mode_ == LockMode::SHARED || request->lock_mode_ == LockMode::EXCLUSIVE)) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      // std::cout << "Unlock: " << oid << std::endl;
      // Bookkeeping
      BookKeeping(false, txn, request->lock_mode_, oid);
      delete request;
      lock_request_queue->cv_.notify_all();
      return true;
    }
  }

  return false;
}

auto LockManager::CheckTableLock(Transaction *txn, LockMode lock_mode, const table_oid_t &oid) -> bool {
  if (lock_mode == LockMode::SHARED) {
    return txn->IsTableExclusiveLocked(oid) || 
           txn->IsTableIntentionExclusiveLocked(oid) ||
           txn->IsTableIntentionSharedLocked(oid) ||
           txn->IsTableSharedIntentionExclusiveLocked(oid) ||
           txn->IsTableSharedLocked(oid);
  }
  assert(lock_mode == LockMode::EXCLUSIVE);
  return txn->IsTableExclusiveLocked(oid) || 
         txn->IsTableIntentionExclusiveLocked(oid) ||
         txn->IsTableSharedIntentionExclusiveLocked(oid); 
}

auto LockManager::LockRow(Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID &rid) -> bool {

  assert(rid.GetPageId() != INVALID_PAGE_ID);
  /** 1. 检查 txn 的状态。*/

  // 检查当前事务是否被终止
  if (txn->GetState() == TransactionState::ABORTED) {
    // BUSTUB_ENSURE(0, "transaction has been aborted!");
    return false;
  }

  // 事务的隔离级别是 READ_UNCOMITTED，只能加 X 和 IX 锁
  if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
    if (!(lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::INTENTION_EXCLUSIVE)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      return false;
    }
    // 事务状态为 SHRINKING 时不能上锁
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  // 事务的隔离级别是 READ_COMITTED
  if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
    // 事务状态为 SHRINKING 时 只允许 S 和 IS 锁
    if (txn->GetState() == TransactionState::SHRINKING && 
      !(lock_mode == LockMode::SHARED || lock_mode == LockMode::INTENTION_SHARED)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  // 事务的隔离级别是 Repeatable Read
  if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    // 事务状态为SHRINKING时不能上锁
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      return false;
    }
  }
  // 行锁不能加意向锁 
  if (!(lock_mode == LockMode::SHARED || lock_mode == LockMode::EXCLUSIVE)) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_INTENTION_LOCK_ON_ROW);
    return false;
  }
  // 加行锁前必须先加表锁
  if (!CheckTableLock(txn, lock_mode, oid)) {
    return false;
  }

  /** 2. 获取 row 对应的 lock request queue。*/

  row_lock_map_latch_.lock();
  // 若 queue 不存在则创建。
  if (!row_lock_map_.count(rid)) {
    row_lock_map_[rid] = std::make_shared<LockRequestQueue>();
  }
  auto &lock_request_queue = row_lock_map_[rid];

  row_lock_map_latch_.unlock();

  std::unique_lock<std::mutex> lock(lock_request_queue->latch_);

  /** 3. 检查此锁请求是否为一次锁升级。*/
  for (auto & request : lock_request_queue->request_queue_) {
    if (request->txn_id_ == txn->GetTransactionId()) {

      BUSTUB_ENSURE(request->granted_ == true, "try requesting another lock means that it has been granted a lock!");
      // 判断当前资源上是否有另一个事务正在尝试升级
      if (lock_request_queue->upgrading_ != INVALID_TXN_ID) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
        return false;
      }
      // 判断锁的类型是否相同 相同直接返回
      if (request->lock_mode_ == lock_mode) {
        return true;
      }
      // 尝试升级
      // 1. 判断是否可以升级
      if (!CheckUpgrade(request->lock_mode_, lock_mode)) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
        return false; 
      }
      // 行锁只能由 S 升级到 X
      assert(request->lock_mode_ == LockMode::SHARED && lock_mode == LockMode::EXCLUSIVE);
      // 2. 释放当前锁
      BookKeeping(false, txn, request->lock_mode_, request->oid_, rid);
      request->granted_ = false;
      request->lock_mode_ = lock_mode;
      request->oid_ = oid;
      lock_request_queue->upgrading_ = request->txn_id_;
      lock_request_queue->cv_.notify_all();
      // 3. 等待直到新锁被授予。
      while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
        lock_request_queue->cv_.wait(lock);
      }
      if (txn->GetState() == TransactionState::ABORTED) {
        return false;
      }
      txn->SetState(TransactionState::GROWING);
      BookKeeping(true, txn, lock_mode, oid, rid);
      return true;
    }
  }
  
  /** 4. 将锁请求加入请求队列（此时无锁升级）。*/
  
  lock_request_queue->request_queue_.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid, rid));

  /** 5. 尝试获取锁。*/
  while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
    lock_request_queue->cv_.wait(lock);
  }
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }
  txn->SetState(TransactionState::GROWING);
  BookKeeping(true, txn, lock_mode, oid, rid);
  return true;
}

auto LockManager::UnlockRow(Transaction *txn, const table_oid_t &oid, const RID &rid) -> bool {
  assert(rid.GetPageId() != INVALID_PAGE_ID);
  row_lock_map_latch_.lock();
  // 若 queue 不存在。
  if (!row_lock_map_.count(rid)) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
    return false;
  }
  auto &lock_request_queue = row_lock_map_[rid];
  // 获取 queue 后 立即可以释放 table
  row_lock_map_latch_.unlock();
  std::unique_lock<std::mutex> lock(lock_request_queue->latch_);
  auto &request_queue = lock_request_queue->request_queue_;

  for (auto it = request_queue.begin(); it != request_queue.end(); ++it) {
    auto request = *it;

    if (request->txn_id_ == txn->GetTransactionId() && request->granted_) {
      // 释放锁
      request_queue.erase(it);
      // 状态变化
      if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
        if (txn->GetState() == TransactionState::GROWING && request->lock_mode_ == LockMode::EXCLUSIVE) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
        if (txn->GetState() == TransactionState::GROWING && request->lock_mode_ == LockMode::EXCLUSIVE) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
        if (txn->GetState() == TransactionState::GROWING && 
           (request->lock_mode_ == LockMode::SHARED || request->lock_mode_ == LockMode::EXCLUSIVE)) {
          txn->SetState(TransactionState::SHRINKING);
        }
      }
      
      BookKeeping(false, txn, request->lock_mode_, oid, rid);
      delete request;
      lock_request_queue->cv_.notify_all();
      return true;
    }
  }
  return false;
}

void LockManager::AddEdge(txn_id_t t1, txn_id_t t2) {}

void LockManager::RemoveEdge(txn_id_t t1, txn_id_t t2) {}

auto LockManager::HasCycle(txn_id_t *txn_id) -> bool { return false; }

auto LockManager::GetEdgeList() -> std::vector<std::pair<txn_id_t, txn_id_t>> {
  std::vector<std::pair<txn_id_t, txn_id_t>> edges(0);
  return edges;
}

void LockManager::RunCycleDetection() {
  while (enable_cycle_detection_) {
    std::this_thread::sleep_for(cycle_detection_interval);
    {  // TODO(students): detect deadlock
    }
  }
}

}  // namespace bustu

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  