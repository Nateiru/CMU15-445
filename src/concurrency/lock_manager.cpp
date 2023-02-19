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

auto LockManager::GetLockSet(Transaction *txn, LockMode lock_mode) const
    -> std::shared_ptr<std::unordered_set<table_oid_t>> {
  std::shared_ptr<std::unordered_set<table_oid_t>> lock_set;
  switch (lock_mode) {
    case LockMode::SHARED:
      lock_set = txn->GetSharedTableLockSet();
      break;
    case LockMode::EXCLUSIVE:
      lock_set = txn->GetExclusiveTableLockSet();
      break;
    case LockMode::INTENTION_SHARED:
      lock_set = txn->GetIntentionSharedTableLockSet();
      break;
    case LockMode::INTENTION_EXCLUSIVE:
      lock_set = txn->GetIntentionExclusiveTableLockSet();
      break;
    case LockMode::SHARED_INTENTION_EXCLUSIVE:
      lock_set = txn->GetSharedIntentionExclusiveTableLockSet();
      break;
  }
  return lock_set;
}

auto LockManager::GetRowLockSet(Transaction *txn, LockMode lock_mode) const
    -> std::shared_ptr<std::unordered_set<RID>> {
  std::shared_ptr<std::unordered_set<RID>> lock_set;
  switch (lock_mode) {
    case LockMode::SHARED:
      lock_set = txn->GetSharedLockSet();
      break;
    case LockMode::EXCLUSIVE:
      lock_set = txn->GetExclusiveLockSet();
      break;
    default:
      BUSTUB_ASSERT(false, "Row lock only support S and X");
      break;
  }
  return lock_set;
}

auto LockManager::GetTableLockMap(Transaction *txn, LockMode lock_mode) const
    -> std::shared_ptr<std::unordered_map<table_oid_t, std::unordered_set<RID>>> {
  std::shared_ptr<std::unordered_map<table_oid_t, std::unordered_set<RID>>> lock_map;
  switch (lock_mode) {
    case LockMode::SHARED:
      lock_map = txn->GetSharedRowLockSet();
      break;
    case LockMode::EXCLUSIVE:
      lock_map = txn->GetExclusiveRowLockSet();
      break;
    default:
      BUSTUB_ASSERT(false, "Row lock only support S and X");
      break;
  }
  return lock_map;
}

auto LockManager::CheckOneCompatibility(LockMode org_lock_mode, LockMode add_lock_mode) const -> bool {
  if (org_lock_mode == LockMode::EXCLUSIVE) {
    return false;
  }
  if (org_lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
    if (add_lock_mode != LockMode::INTENTION_SHARED) {
      return false;
    }
  }
  if (org_lock_mode == LockMode::SHARED) {
    if (add_lock_mode != LockMode::INTENTION_SHARED && add_lock_mode != LockMode::SHARED) {
      return false;
    }
  }
  if (org_lock_mode == LockMode::INTENTION_EXCLUSIVE) {
    if (add_lock_mode != LockMode::INTENTION_SHARED && add_lock_mode != LockMode::INTENTION_EXCLUSIVE) {
      return false;
    }
  }
  if (org_lock_mode == LockMode::INTENTION_SHARED) {
    if (add_lock_mode == LockMode::EXCLUSIVE) {
      return false;
    }
  }
  return true;
}

auto LockManager::CheckCompatibility(Transaction *txn, LockMode lock_mode,
                                     std::list<LockRequest *> &target_lrq_queue) const -> bool {
  if (txn->GetState() == TransactionState::ABORTED) {
    return true;
  }
  if (txn->GetTransactionId() == 2) {
    // LOG_DEBUG("txn2 check");
  }
  // IS IX S SIX S
  bool exist_locks[5] = {false, false, false, false, false};
  for (auto *lr : target_lrq_queue) {
    if (txn->GetTransactionId() == 2) {
      // LOG_DEBUG("txn2 visit node for txn%d", lr->txn_id_);
    }
    if (lr->txn_id_ == txn->GetTransactionId()) {
      return true;
    }
    // A lock request can get locked only if all request before it all get locked.
    // May have some lock requests before, but if this lock request is compatible with them, it can still get the lock.
    if (!lr->granted_) {
      // Check request before it can be given
      if (exist_locks[0]) {
        if (!CheckOneCompatibility(lr->lock_mode_, LockMode::INTENTION_SHARED)) {
          return false;
        }
      }
      if (exist_locks[1]) {
        if (!CheckOneCompatibility(lr->lock_mode_, LockMode::INTENTION_EXCLUSIVE)) {
          return false;
        }
      }
      if (exist_locks[2]) {
        if (!CheckOneCompatibility(lr->lock_mode_, LockMode::SHARED)) {
          return false;
        }
      }
      if (exist_locks[3]) {
        if (!CheckOneCompatibility(lr->lock_mode_, LockMode::SHARED_INTENTION_EXCLUSIVE)) {
          return false;
        }
      }
      if (exist_locks[4]) {
        if (!CheckOneCompatibility(lr->lock_mode_, LockMode::INTENTION_EXCLUSIVE)) {
          return false;
        }
      }
    }

    switch (lr->lock_mode_) {
      case LockMode::INTENTION_SHARED:
        exist_locks[0] = true;
        break;
      case LockMode::INTENTION_EXCLUSIVE:
        exist_locks[1] = true;
        break;
      case LockMode::SHARED:
        exist_locks[2] = true;
        break;
      case LockMode::SHARED_INTENTION_EXCLUSIVE:
        exist_locks[3] = true;
        break;
      case LockMode::EXCLUSIVE:
        exist_locks[4] = true;
        break;
    }

    if (!CheckOneCompatibility(lr->lock_mode_, lock_mode)) {
      return false;
    }
  }
  BUSTUB_ASSERT(false, "cannot find target lock request in the queue");
  return true;
}
// ======================================================

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

// auto LockManager::CheckOneCompatibility(LockMode hold, LockMode want) const -> bool {
//   if (hold == LockMode::INTENTION_SHARED) {
//     return want == LockMode::INTENTION_SHARED    || \
//            want == LockMode::INTENTION_EXCLUSIVE || \
//            want == LockMode::SHARED              || \
//            want == LockMode::SHARED_INTENTION_EXCLUSIVE;
//   }
//   if (hold == LockMode::INTENTION_EXCLUSIVE) {
//     return want == LockMode::INTENTION_SHARED    || \
//            want == LockMode::INTENTION_EXCLUSIVE; 
//   }
//   if (hold == LockMode::SHARED) {
//     return want == LockMode::INTENTION_SHARED    || \
//            want == LockMode::SHARED;
//   }
//   if (hold == LockMode::SHARED_INTENTION_EXCLUSIVE) {
//     return want == LockMode::INTENTION_SHARED;
//   }
//   return false;
// }


auto LockManager::GrantLock(Transaction *txn, std::shared_ptr<LockManager::LockRequestQueue> &lock_request_queue) -> bool {
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }
  /** 1. 找到目前将要候等事务*/
  auto target_lock_it = std::find_if(lock_request_queue->request_queue_.begin(), 
                                      lock_request_queue->request_queue_.end(),
                                      [&](const LockRequest *lr) {
                                        return lr->txn_id_ == txn->GetTransactionId();
                                      });
  assert(target_lock_it != lock_request_queue->request_queue_.end());
  auto target_lock_request = *target_lock_it;      
  /** 2. 升级的优先级最高 率先升级*/
  if (lock_request_queue->upgrading_ != INVALID_TXN_ID) {
    if (lock_request_queue->upgrading_ == txn->GetTransactionId() && 
        CheckCompatibility(txn, target_lock_request->lock_mode_, lock_request_queue->request_queue_)) {
      target_lock_request->granted_ = true;
      lock_request_queue->upgrading_ = INVALID_TXN_ID;
      return true;
    }
    return false;
  }
  /** 3. 无升级 FIFO */

  if (CheckCompatibility(txn, target_lock_request->lock_mode_, lock_request_queue->request_queue_)) {
    if (txn->GetState() == TransactionState::ABORTED) {
      return false;
    }
    target_lock_request->granted_ = true;
    return true;
  }
  return false;
}

void LockManager::DeleteInQueue(Transaction *txn, std::shared_ptr<LockManager::LockRequestQueue> &lock_request_queue) {
  // assert(txn->GetState() == TransactionState::ABORTED);
  auto remove_it = std::find_if(lock_request_queue->request_queue_.begin(), 
                                      lock_request_queue->request_queue_.end(),
                                      [&](const LockRequest *lr) {
                                        return lr->txn_id_ == txn->GetTransactionId();
                                      });
  assert(remove_it != lock_request_queue->request_queue_.end());
  auto remove_request = *remove_it;
  lock_request_queue->request_queue_.erase(remove_it);
  delete remove_request;
  if (lock_request_queue->upgrading_ == txn->GetTransactionId()) {
    lock_request_queue->upgrading_ = INVALID_TXN_ID;
  }
  lock_request_queue->cv_.notify_all();
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
// =========================================================================

// auto LockManager::LockTable(Transaction *txn, LockMode lock_mode, const table_oid_t &oid) -> bool {
//   /** 1. 检查 txn 的状态。*/

//   if (txn->GetState() == TransactionState::ABORTED) {
//     return false;
//   }
//   if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
//     if (txn->GetState() == TransactionState::SHRINKING) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
//     }
//   } else if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
//     if (txn->GetState() == TransactionState::SHRINKING) {
//       if (lock_mode != LockMode::INTENTION_SHARED && lock_mode != LockMode::SHARED) {
//         txn->SetState(TransactionState::ABORTED);
//         throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
//       }
//     }
//   } else if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
//     if (lock_mode != LockMode::INTENTION_EXCLUSIVE && lock_mode != LockMode::EXCLUSIVE) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
//     }
//     if (txn->GetState() == TransactionState::SHRINKING) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
//     }
//   }
//   bool is_update = false;
//   LockMode org_lock_mode;
//   if (txn->IsTableSharedLocked(oid)) {
//     if (lock_mode == LockMode::SHARED) {
//       return true;
//     }
//     if (lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
//       is_update = true;
//       org_lock_mode = LockMode::SHARED;
//     }
//     if (lock_mode == LockMode::INTENTION_SHARED || lock_mode == LockMode::INTENTION_EXCLUSIVE) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
//     }
//   } else if (txn->IsTableExclusiveLocked(oid)) {
//     if (lock_mode == LockMode::EXCLUSIVE) {
//       return true;
//     }
//     if (lock_mode != LockMode::EXCLUSIVE) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
//     }
//   } else if (txn->IsTableIntentionSharedLocked(oid)) {
//     if (lock_mode == LockMode::INTENTION_SHARED) {
//       return true;
//     }
//     if (lock_mode != LockMode::INTENTION_SHARED) {
//       is_update = true;
//       org_lock_mode = LockMode::INTENTION_SHARED;
//     }
//   } else if (txn->IsTableIntentionExclusiveLocked(oid)) {
//     if (lock_mode == LockMode::INTENTION_EXCLUSIVE) {
//       return true;
//     }
//     if (lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
//       is_update = true;
//       org_lock_mode = LockMode::INTENTION_EXCLUSIVE;
//     }
//     if (lock_mode == LockMode::SHARED || lock_mode == LockMode::INTENTION_SHARED) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
//     }
//   } else if (txn->IsTableSharedIntentionExclusiveLocked(oid)) {
//     if (lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
//       return true;
//     }
//     if (lock_mode == LockMode::EXCLUSIVE) {
//       is_update = true;
//       org_lock_mode = LockMode::SHARED_INTENTION_EXCLUSIVE;
//     }
//     if (lock_mode != LockMode::EXCLUSIVE && lock_mode != LockMode::SHARED_INTENTION_EXCLUSIVE) {
//       txn->SetState(TransactionState::ABORTED);
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
//     }
//   }
//   /** 2. 获取 table 对应的 lock request queue。*/

//   table_lock_map_latch_.lock();
//   // 若 queue 不存在则创建。
//   if (table_lock_map_.find(oid) == table_lock_map_.end()) {
//     table_lock_map_.emplace(oid, std::make_shared<LockRequestQueue>());
//   }
//   auto &lock_request_queue = table_lock_map_[oid];
//   // 获取 queue 后 立即可以释放 table
//   table_lock_map_latch_.unlock();
//   std::unique_lock<std::mutex> lock(lock_request_queue->latch_);
//   auto &request_queue = lock_request_queue->request_queue_;
//   /** 3. 检查此锁请求是否为一次锁升级。*/
//   // bool is_update = false;
//   for (auto it = request_queue.begin(); it != request_queue.end(); ++it) {
//     auto request = *it;
//     if (request->txn_id_ == txn->GetTransactionId()) {
//       // std::cout << "Opps! Upgrade: "<< request->txn_id_ << "\nhas: " << (int) request->granted_ << std::endl;
//       assert(is_update);
//       assert(org_lock_mode == request->lock_mode_);
//       BUSTUB_ENSURE(request->granted_ == true, "try requesting another lock means that it has been granted a lock!");
//       // 判断当前资源上是否有另一个事务正在尝试升级
//       if (lock_request_queue->upgrading_ != INVALID_TXN_ID) {
//         txn->SetState(TransactionState::ABORTED);
//         throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
//       }
//       // 判断锁的类型是否相同 相同直接返回
//       if (request->lock_mode_ == lock_mode) {
//         return true;
//       }
//       // 尝试升级
//       // 1. 判断是否可以升级
//       if (!CheckUpgrade(request->lock_mode_, lock_mode)) {
//         txn->SetState(TransactionState::ABORTED);
//         throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
//       }
//       // 2. 释放当前锁
//       BookKeeping(false, txn, request->lock_mode_, request->oid_);
//       request_queue.erase(it);
//       delete request;
//       lock_request_queue->upgrading_ = txn->GetTransactionId();
//       // lock_request_queue->cv_.notify_all();
//       // 3. 插入新的位置
//       auto insert_pos = std::find_if(request_queue.begin(), request_queue.end(),
//                                      [&](const LockRequest *lr) { return !lr->granted_; });
//       auto new_request = new LockRequest(txn->GetTransactionId(), lock_mode, oid); 
//       auto new_it = request_queue.insert(insert_pos, new_request); 
//       // 4. 等待直到新锁被授予。
//       while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
//         lock_request_queue->cv_.wait(lock);
//       }
//       // lock_request_queue->cv_.wait(lock, [&]() { return CheckCompatibility(txn, lock_mode, request_queue); });
//       if (is_update) {
//         lock_request_queue->upgrading_ = INVALID_PAGE_ID;
//       }
//       if (txn->GetState() == TransactionState::ABORTED) {
//         // DeleteInQueue(txn, lock_request_queue);
//         // if (lock_request_queue->upgrading_ == txn->GetTransactionId()) {
//         //   lock_request_queue->upgrading_ = INVALID_TXN_ID;
//         // }
//         request_queue.erase(new_it);
//         delete new_request;
//         lock_request_queue->cv_.notify_all();
//         return false;
//       }
//       new_request->granted_ = true;
//       txn->SetState(TransactionState::GROWING);
//       // BookKeeping(true, txn, lock_mode, oid);
//       std::shared_ptr<std::unordered_set<table_oid_t>> lock_set = GetLockSet(txn, lock_mode);
//       lock_set->emplace(oid);
//       return true;
//     }
//   }
//   // std::cout << "Lock Table" <<std::endl; 
//   /** 4. 将锁请求加入请求队列（此时无锁升级）。*/
//   decltype(request_queue.begin()) new_it; 
//   auto new_request = request_queue.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid));
//   new_it = --request_queue.end();
//   assert(request_queue.back()->lock_mode_ == lock_mode);
//   /** 5. 尝试获取锁。*/
//   while ((txn->GetState() != TransactionState::ABORTED) && !GrantLock(txn, lock_request_queue)) {
//     lock_request_queue->cv_.wait(lock);
//   }
//   // lock_request_queue->cv_.wait(lock, [&]() { return CheckCompatibility(txn, lock_mode, request_queue); });
//   if (txn->GetState() == TransactionState::ABORTED) {
//     // DeleteInQueue(txn, lock_request_queue);
//     // if (lock_request_queue->upgrading_ == txn->GetTransactionId()) {
//     //   lock_request_queue->upgrading_ = INVALID_TXN_ID;
//     // }
//     request_queue.erase(new_it);
//     delete new_request;
//     lock_request_queue->cv_.notify_all();
//     return false;
//   }
//   new_request->granted_ = true;
//   txn->SetState(TransactionState::GROWING);
//   // BookKeeping(true, txn, lock_mode, oid);
//   std::shared_ptr<std::unordered_set<table_oid_t>> lock_set = GetLockSet(txn, lock_mode);
//   lock_set->emplace(oid);
//   return true;
// }

auto LockManager::LockTable(Transaction *txn, LockMode lock_mode, const table_oid_t &oid) -> bool {
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }

  if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
    }
  } else if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
    if (txn->GetState() == TransactionState::SHRINKING) {
      if (lock_mode != LockMode::INTENTION_SHARED && lock_mode != LockMode::SHARED) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      }
    }
  } else if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
    if (lock_mode != LockMode::INTENTION_EXCLUSIVE && lock_mode != LockMode::EXCLUSIVE) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
    }
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
    }
  }

  bool is_update = false;
  LockMode org_lock_mode;
  if (txn->IsTableSharedLocked(oid)) {
    if (lock_mode == LockMode::SHARED) {
      return true;
    }
    if (lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
      is_update = true;
      org_lock_mode = LockMode::SHARED;
    }
    if (lock_mode == LockMode::INTENTION_SHARED || lock_mode == LockMode::INTENTION_EXCLUSIVE) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
    }
  } else if (txn->IsTableExclusiveLocked(oid)) {
    if (lock_mode == LockMode::EXCLUSIVE) {
      return true;
    }
    if (lock_mode != LockMode::EXCLUSIVE) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
    }
  } else if (txn->IsTableIntentionSharedLocked(oid)) {
    if (lock_mode == LockMode::INTENTION_SHARED) {
      return true;
    }
    if (lock_mode != LockMode::INTENTION_SHARED) {
      is_update = true;
      org_lock_mode = LockMode::INTENTION_SHARED;
    }
  } else if (txn->IsTableIntentionExclusiveLocked(oid)) {
    if (lock_mode == LockMode::INTENTION_EXCLUSIVE) {
      return true;
    }
    if (lock_mode == LockMode::EXCLUSIVE || lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
      is_update = true;
      org_lock_mode = LockMode::INTENTION_EXCLUSIVE;
    }
    if (lock_mode == LockMode::SHARED || lock_mode == LockMode::INTENTION_SHARED) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
    }
  } else if (txn->IsTableSharedIntentionExclusiveLocked(oid)) {
    if (lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
      return true;
    }
    if (lock_mode == LockMode::EXCLUSIVE) {
      is_update = true;
      org_lock_mode = LockMode::SHARED_INTENTION_EXCLUSIVE;
    }
    if (lock_mode != LockMode::EXCLUSIVE && lock_mode != LockMode::SHARED_INTENTION_EXCLUSIVE) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
    }
  }
  //  if (is_update) {
  //    // LOG_DEBUG("txn%d update from %d to %d", txn->GetTransactionId(),
  //    static_cast<std::underlying_type<LockMode>::type>(org_lock_mode),
  //    static_cast<std::underlying_type<LockMode>::type>(lock_mode));
  //  }

  table_lock_map_latch_.lock();
  if (table_lock_map_.find(oid) == table_lock_map_.end()) {
    table_lock_map_.emplace(oid, std::make_shared<LockRequestQueue>());
  }
  auto &target_lrq = table_lock_map_[oid];
  table_lock_map_latch_.unlock();

  if (is_update) {
    std::unique_lock lk(target_lrq->latch_);
    if (target_lrq->upgrading_ != INVALID_PAGE_ID) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
    }
    target_lrq->upgrading_ = txn->GetTransactionId();
    // auto &target_lrq_queue = target_lrq->request_queue_;
    auto remove_request = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                       [&](const LockRequest *lr) { return lr->txn_id_ == txn->GetTransactionId(); });
    BUSTUB_ASSERT(remove_request != target_lrq->request_queue_.end(), "Cannot find lock request to update");
    auto *remove_ptr = *remove_request;
    BUSTUB_ASSERT(remove_ptr->granted_ == true, "??????????");
    BUSTUB_ASSERT(remove_ptr->lock_mode_ == org_lock_mode, "!!!!!!!");
    target_lrq->request_queue_.erase(remove_request);
    lk.unlock();
    delete remove_ptr;
    std::shared_ptr<std::unordered_set<table_oid_t>> org_lock_set = GetLockSet(txn, org_lock_mode);
    org_lock_set->erase(oid);
  }

  {
    std::unique_lock lk(target_lrq->latch_);
    // auto &target_lrq_queue = target_lrq->request_queue_;
    LockRequest *new_request;
    decltype(target_lrq->request_queue_.begin()) new_it;
    if (!is_update) {
      new_request = target_lrq->request_queue_.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid));
      new_it = --target_lrq->request_queue_.end();
    } else {
      // find the first lock request that not get the lock
      auto insert_pos = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                     [&](const LockRequest *lr) { return !lr->granted_; });
      new_request = new LockRequest(txn->GetTransactionId(), lock_mode, oid);
      new_it = target_lrq->request_queue_.insert(insert_pos, new_request);
    }
    // if (txn->GetTransactionId() == 2) {
    // LOG_DEBUG("txn2 start acquire lock");
    // }
    target_lrq->cv_.wait(lk, [&]() { return CheckCompatibility(txn, lock_mode, target_lrq->request_queue_); });
    if (is_update) {
      target_lrq->upgrading_ = INVALID_PAGE_ID;
    }

    if (txn->GetState() == TransactionState::ABORTED) {
      //      if (is_update) {
      //        std::shared_ptr<std::unordered_set<table_oid_t>> org_lock_set = GetLockSet(txn, org_lock_mode);
      //        org_lock_set->insert(oid);
      //      }
      // LOG_DEBUG("erase request for txn%d", (*new_it)->txn_id_);
      target_lrq->request_queue_.erase(new_it);
      delete new_request;
      target_lrq->cv_.notify_all();
      // LOG_DEBUG("queue len = %d", static_cast<int>(target_lrq_queue.size()));
      // LOG_DEBUG("remain request is for txn%d", (*target_lrq_queue.begin())->txn_id_);
      //      std::shared_ptr<std::unordered_set<table_oid_t>> lock_set = GetLockSet(txn, lock_mode);
      //      lock_set->emplace(oid);
      return false;
    }
    lk.unlock();
    std::shared_ptr<std::unordered_set<table_oid_t>> lock_set = GetLockSet(txn, lock_mode);
    lock_set->emplace(oid);
    new_request->granted_ = true;
  }

  return true;
}

auto LockManager::UnlockTable(Transaction *txn, const table_oid_t &oid) -> bool {
  LockMode org_lock_mode;
  if (txn->IsTableSharedLocked(oid)) {
    org_lock_mode = LockMode::SHARED;
  } else if (txn->IsTableExclusiveLocked(oid)) {
    org_lock_mode = LockMode::EXCLUSIVE;
  } else if (txn->IsTableIntentionSharedLocked(oid)) {
    org_lock_mode = LockMode::INTENTION_SHARED;
  } else if (txn->IsTableIntentionExclusiveLocked(oid)) {
    org_lock_mode = LockMode::INTENTION_EXCLUSIVE;
  } else if (txn->IsTableSharedIntentionExclusiveLocked(oid)) {
    org_lock_mode = LockMode::SHARED_INTENTION_EXCLUSIVE;
  } else {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }

  if (!(*txn->GetSharedRowLockSet())[oid].empty() || !(*txn->GetExclusiveRowLockSet())[oid].empty()) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::TABLE_UNLOCKED_BEFORE_UNLOCKING_ROWS);
  }

  table_lock_map_latch_.lock();
  auto &target_lrq = table_lock_map_[oid];
  table_lock_map_latch_.unlock();

  {
    std::lock_guard lk(target_lrq->latch_);
    // auto &target_lrq_queue = target_lrq->request_queue_;
    auto remove_request = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                       [&](const LockRequest *lr) { return lr->txn_id_ == txn->GetTransactionId(); });
    BUSTUB_ASSERT(remove_request != target_lrq->request_queue_.end(), "Cannot find lock request to unlock");
    auto *remove_ptr = *remove_request;
    target_lrq->request_queue_.erase(remove_request);
    delete remove_ptr;
    target_lrq->cv_.notify_all();
  }

  std::shared_ptr<std::unordered_set<table_oid_t>> org_lock_set = GetLockSet(txn, org_lock_mode);
  org_lock_set->erase(oid);

  if (org_lock_mode == LockMode::EXCLUSIVE) {
    if (txn->GetState() == TransactionState::GROWING) {
      txn->SetState(TransactionState::SHRINKING);
    }
  } else if (org_lock_mode == LockMode::SHARED) {
    BUSTUB_ASSERT(txn->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED,
                  "Unlock S locks under READ_UNCOMMITTED is undefined behaviour.");
    if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
      if (txn->GetState() == TransactionState::GROWING) {
        txn->SetState(TransactionState::SHRINKING);
      }
    }
  }

  return true;
}

auto LockManager::LockRow(Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID &rid) -> bool {
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }

  if (lock_mode != LockMode::EXCLUSIVE && lock_mode != LockMode::SHARED) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_INTENTION_LOCK_ON_ROW);
  }

  if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    if (txn->GetState() == TransactionState::SHRINKING) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
    }
  } else if (txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
    if (txn->GetState() == TransactionState::SHRINKING) {
      if (lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      }
    }
  } else if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
    if (lock_mode == LockMode::SHARED) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
    }
    if (lock_mode == LockMode::EXCLUSIVE) {
      if (txn->GetState() == TransactionState::SHRINKING) {
        txn->SetState(TransactionState::ABORTED);
        throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
      }
    }
  }

  if (lock_mode == LockMode::EXCLUSIVE) {
    if (!txn->IsTableSharedIntentionExclusiveLocked(oid) && !txn->IsTableIntentionExclusiveLocked(oid) &&
        !txn->IsTableExclusiveLocked(oid)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::TABLE_LOCK_NOT_PRESENT);
    }
  } else if (lock_mode == LockMode::SHARED) {
    if (!txn->IsTableIntentionSharedLocked(oid) && !txn->IsTableSharedLocked(oid) &&
        !txn->IsTableSharedIntentionExclusiveLocked(oid) && !txn->IsTableIntentionExclusiveLocked(oid) &&
        !txn->IsTableExclusiveLocked(oid)) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::TABLE_LOCK_NOT_PRESENT);
    }
  }

  bool is_update = false;
  LockMode org_lock_mode;
  if (txn->IsRowSharedLocked(oid, rid)) {
    if (lock_mode == LockMode::SHARED) {
      return true;
    }
    if (lock_mode == LockMode::EXCLUSIVE) {
      is_update = true;
      org_lock_mode = LockMode::SHARED;
    }
  } else if (txn->IsRowExclusiveLocked(oid, rid)) {
    if (lock_mode == LockMode::EXCLUSIVE) {
      return true;
    }
    if (lock_mode == LockMode::SHARED) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::INCOMPATIBLE_UPGRADE);
    }
  }

  row_lock_map_latch_.lock();
  if (row_lock_map_.find(rid) == row_lock_map_.end()) {
    row_lock_map_.emplace(rid, std::make_shared<LockRequestQueue>());
  }
  auto &target_lrq = row_lock_map_[rid];
  row_lock_map_latch_.unlock();

  if (is_update) {
    std::unique_lock lk(target_lrq->latch_);
    if (target_lrq->upgrading_ != INVALID_PAGE_ID) {
      txn->SetState(TransactionState::ABORTED);
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
    }
    target_lrq->upgrading_ = txn->GetTransactionId();
    // auto &target_lrq_queue = target_lrq->request_queue_;
    auto remove_request = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                       [&](const LockRequest *lr) { return lr->txn_id_ == txn->GetTransactionId(); });
    BUSTUB_ASSERT(remove_request != target_lrq->request_queue_.end(), "Cannot find lock request to update");
    auto *remove_ptr = *remove_request;
    target_lrq->request_queue_.erase(remove_request);
    lk.unlock();
    delete remove_ptr;
    auto org_lock_set = GetRowLockSet(txn, org_lock_mode);
    org_lock_set->erase(rid);
    auto org_lock_map = GetTableLockMap(txn, org_lock_mode);
    (*org_lock_map)[oid].erase(rid);
  }

  {
    std::unique_lock lk(target_lrq->latch_);
    // auto &target_lrq_queue = target_lrq->request_queue_;
    LockRequest *new_request;
    decltype(target_lrq->request_queue_.begin()) new_it;
    if (!is_update) {
      new_request = target_lrq->request_queue_.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid, rid));
      new_it = --target_lrq->request_queue_.end();
    } else {
      // find the first lock request that not get the lock
      auto insert_pos = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                     [&](const LockRequest *lr) { return !lr->granted_; });
      new_request = new LockRequest(txn->GetTransactionId(), lock_mode, oid, rid);
      new_it = target_lrq->request_queue_.insert(insert_pos, new_request);
    }
    // std::cout << "try lock txn=" << new_request->txn_id_ << " oid=" << new_request->oid_ << " rid=" <<
    // new_request->rid_ << std::endl;
    target_lrq->cv_.wait(lk, [&]() {
      if (txn->GetState() == TransactionState::ABORTED) {
        return true;
      }
      for (auto *lr : target_lrq->request_queue_) {
        if (lr->txn_id_ == txn->GetTransactionId()) {
          return true;
        }
        // A lock request can get locked only if all request before it all get locked.
        //        if (!lr->granted_) {
        //          return false;
        //        }
        if (lr->lock_mode_ == LockMode::EXCLUSIVE || lock_mode == LockMode::EXCLUSIVE) {
          return false;
        }
      }
      BUSTUB_ASSERT(false, "cannot find target lock request in the queue");
      return false;
    });
    if (is_update) {
      target_lrq->upgrading_ = INVALID_PAGE_ID;
    }

    if (txn->GetState() == TransactionState::ABORTED) {
      //      if (is_update) {
      //        auto org_lock_set = GetRowLockSet(txn, org_lock_mode);
      //        org_lock_set->insert(rid);
      //        auto org_lock_map = GetTableLockMap(txn, org_lock_mode);
      //        (*org_lock_map)[oid].insert(rid);
      //      }
      target_lrq->request_queue_.erase(new_it);
      delete new_request;
      target_lrq->cv_.notify_all();
      //      auto lock_set = GetRowLockSet(txn, lock_mode);
      //      lock_set->emplace(rid);
      //      auto lock_map = GetTableLockMap(txn, lock_mode);
      //      (*lock_map)[oid].insert(rid);
      return false;
    }
    lk.unlock();
    auto lock_set = GetRowLockSet(txn, lock_mode);
    lock_set->emplace(rid);
    auto lock_map = GetTableLockMap(txn, lock_mode);
    (*lock_map)[oid].insert(rid);
    new_request->granted_ = true;
  }

  return true;
}

auto LockManager::UnlockRow(Transaction *txn, const table_oid_t &oid, const RID &rid) -> bool {
  LockMode org_lock_mode;
  if (txn->IsRowSharedLocked(oid, rid)) {
    org_lock_mode = LockMode::SHARED;
  } else if (txn->IsRowExclusiveLocked(oid, rid)) {
    org_lock_mode = LockMode::EXCLUSIVE;
  } else {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }

  row_lock_map_latch_.lock();
  auto &target_lrq = row_lock_map_[rid];
  row_lock_map_latch_.unlock();

  {
    std::lock_guard lk(target_lrq->latch_);
    // auto &target_lrq_queue = target_lrq->request_queue_;
    auto remove_request = std::find_if(target_lrq->request_queue_.begin(), target_lrq->request_queue_.end(),
                                       [&](const LockRequest *lr) { return lr->txn_id_ == txn->GetTransactionId(); });
    BUSTUB_ASSERT(remove_request != target_lrq->request_queue_.end(), "Cannot find lock request to unlock");
    auto *remove_ptr = *remove_request;
    // std::cout << "free txn=" << (*remove_request)->txn_id_ << " oid=" << (*remove_request)->oid_ << " rid=" <<
    // (*remove_request)->rid_ << std::endl;
    target_lrq->request_queue_.erase(remove_request);
    delete remove_ptr;
    target_lrq->cv_.notify_all();
  }

  std::shared_ptr<std::unordered_set<RID>> org_lock_set = GetRowLockSet(txn, org_lock_mode);
  org_lock_set->erase(rid);
  auto lock_map = GetTableLockMap(txn, org_lock_mode);
  (*lock_map)[oid].erase(rid);

  if (org_lock_mode == LockMode::EXCLUSIVE) {
    if (txn->GetState() == TransactionState::GROWING) {
      txn->SetState(TransactionState::SHRINKING);
    }
  } else if (org_lock_mode == LockMode::SHARED) {
    BUSTUB_ASSERT(txn->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED,
                  "Unlock S locks under READ_UNCOMMITTED is undefined behaviour.");
    if (txn->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
      if (txn->GetState() == TransactionState::GROWING) {
        txn->SetState(TransactionState::SHRINKING);
      }
    }
  }

  return true;
}

void LockManager::AddEdge(txn_id_t t1, txn_id_t t2) {
  std::lock_guard lk(waits_for_latch_);
  if (waits_for_.find(t1) == waits_for_.end()) {
    waits_for_.emplace(t1, std::vector<txn_id_t>{t2});
  } else {
    if (std::find(waits_for_[t1].begin(), waits_for_[t1].end(), t2) == waits_for_[t1].end()) {
      // Keep order
      auto it = std::lower_bound(waits_for_[t1].begin(), waits_for_[t1].end(), t2);
      // [](const txn_id_t &t1, const txn_id_t &t2) { return t1 > t2; });
      //      auto it = waits_for_[t1].begin();
      //      for (; it != waits_for_[t1].end(); ++it) {
      //        if (*it < t2) {
      //          break;
      //        }
      //      }
      waits_for_[t1].insert(it, t2);
    }
  }
}

void LockManager::RemoveEdge(txn_id_t t1, txn_id_t t2) {
  std::lock_guard lk(waits_for_latch_);
  if (waits_for_.find(t1) != waits_for_.end()) {
    auto it = std::find(waits_for_[t1].begin(), waits_for_[t1].end(), t2);
    waits_for_[t1].erase(it);
    if (waits_for_[t1].empty()) {
      waits_for_.erase(t1);
    }
  }
}

auto LockManager::HasCycle(txn_id_t *txn_id) -> bool {
  std::lock_guard lk(waits_for_latch_);
  std::vector<txn_id_t> id_list;
  std::unordered_map<txn_id_t, int> state;
  for (const auto &[id1, edge_list] : waits_for_) {
    id_list.push_back(id1);
    state[id1] = 0;
  }
  // std::sort(id_list.begin(), id_list.end(), std::greater<>());
  std::sort(id_list.begin(), id_list.end());

  // state[id] == 0: not visit, 1: visiting id's son node, 2: finish visiting id and its son node.
  // dfs实现逆拓扑排序（如何检查有无环）? - ZhiHuReader的回答 - 知乎
  // https://www.zhihu.com/question/480590414/answer/2067908528
  bool find_cycle = false;
  std::function<void(txn_id_t)> dfs_visit = [&](txn_id_t from_id) {
    state[from_id] = 1;
    for (auto to_id : waits_for_[from_id]) {
      // Do not visit others
      if (find_cycle) {
        return;
      }
      if (state[to_id] == 0) {
        dfs_visit(to_id);
      } else if (state[to_id] == 1) {
        find_cycle = true;
        return;
      }
    }
    if (!find_cycle) {
      state[from_id] = 2;
    }
  };

  *txn_id = -1;
  for (int id : id_list) {
    dfs_visit(id);
    if (find_cycle) {
      for (int id2 : id_list) {
        if (state[id2] == 1) {
          *txn_id = std::max(*txn_id, id2);
        }
      }
      break;
    }
  }
  return find_cycle;
}

auto LockManager::GetEdgeList() -> std::vector<std::pair<txn_id_t, txn_id_t>> {
  std::lock_guard lk(waits_for_latch_);
  std::vector<std::pair<txn_id_t, txn_id_t>> edges(0);
  for (const auto &[id1, edge_list] : waits_for_) {
    for (const auto &id2 : edge_list) {
      edges.emplace_back(id1, id2);
    }
  }
  return edges;
}

void LockManager::RunCycleDetection() {
  while (enable_cycle_detection_) {
    std::this_thread::sleep_for(cycle_detection_interval);
    {  // TODO(students): detect deadlock
      // Build graph
      for (auto &it : table_lock_map_) {
        std::lock_guard lk(it.second->latch_);
        const auto &lrq_queue = it.second->request_queue_;
        std::vector<txn_id_t> wait_list;
        for (const auto *lr : lrq_queue) {
          if (lr->granted_) {
            wait_list.push_back(lr->txn_id_);
          } else {
            for (auto to_id : wait_list) {
              AddEdge(lr->txn_id_, to_id);
            }
          }
        }
      }
      for (auto &it : row_lock_map_) {
        std::lock_guard lk(it.second->latch_);
        const auto &lrq_queue = it.second->request_queue_;
        std::vector<txn_id_t> wait_list;
        for (const auto *lr : lrq_queue) {
          if (lr->granted_) {
            wait_list.push_back(lr->txn_id_);
          } else {
            for (auto to_id : wait_list) {
              AddEdge(lr->txn_id_, to_id);
            }
          }
        }
      }
      // PrintGraph();
      // Detect cycle
      txn_id_t txn_id = -1;
      while (HasCycle(&txn_id)) {
        // Set state to abort
        TransactionManager::txn_map_mutex.lock();
        auto *txn = TransactionManager::txn_map[txn_id];
        TransactionManager::txn_map_mutex.unlock();
        txn->SetState(TransactionState::ABORTED);
        // Notify all
        for (auto &it : table_lock_map_) {
          std::lock_guard lk(it.second->latch_);
          auto &lrq_queue = it.second->request_queue_;
          bool need_notify = false;
          for (auto it2 = lrq_queue.begin(); it2 != lrq_queue.end();) {
            if ((*it2)->txn_id_ == txn_id) {
              // it2 = lrq_queue.erase(it2);
              need_notify = true;
              break;
            }
            ++it2;
          }
          if (need_notify) {
            it.second->cv_.notify_all();
          }
        }
        for (auto &it : row_lock_map_) {
          std::lock_guard lk(it.second->latch_);
          auto &lrq_queue = it.second->request_queue_;
          bool need_notify = false;
          for (auto it2 = lrq_queue.begin(); it2 != lrq_queue.end();) {
            if ((*it2)->txn_id_ == txn_id) {
              // it2 = lrq_queue.erase(it2);
              need_notify = true;
              break;
            }
            ++it2;
          }
          if (need_notify) {
            it.second->cv_.notify_all();
          }
        }
        // Remove corresponding edges
        auto edge_list = GetEdgeList();
        for (auto edge : edge_list) {
          if (edge.first == txn_id || edge.second == txn_id) {
            RemoveEdge(edge.first, edge.second);
            // std::cout << "remove edges " << edge.first << " " << edge.second << std::endl;
          }
        }
      }
    }
  }
}

// void LockManager::PrintGraph() const {
//   for (const auto &[id1, edge_list] : waits_for_) {
//     for (const auto &id2 : edge_list) {
//       std::cout << id1 << "-->" << id2 << " ";
//     }
//     std::cout << std::endl;
//   }
// }

}  // namespace bustub
