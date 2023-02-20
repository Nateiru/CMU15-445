

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
    auto &target_lrq_queue = target_lrq->request_queue_;
    auto remove_request = std::find_if(target_lrq_queue.begin(), target_lrq_queue.end(),
                                       [&](const LockRequest *lr) { return lr->txn_id_ == txn->GetTransactionId(); });
    BUSTUB_ASSERT(remove_request != target_lrq_queue.end(), "Cannot find lock request to update");
    auto *remove_ptr = *remove_request;
    target_lrq_queue.erase(remove_request);
    lk.unlock();
    delete remove_ptr;
    std::shared_ptr<std::unordered_set<table_oid_t>> org_lock_set = GetLockSet(txn, org_lock_mode);
    org_lock_set->erase(oid);
  }

  {
    std::unique_lock lk(target_lrq->latch_);
    auto &target_lrq_queue = target_lrq->request_queue_;
    LockRequest *new_request;
    decltype(target_lrq_queue.begin()) new_it;
    if (!is_update) {
      new_request = target_lrq_queue.emplace_back(new LockRequest(txn->GetTransactionId(), lock_mode, oid));
      new_it = --target_lrq_queue.end();
    } else {
      // find the first lock request that not get the lock
      auto insert_pos = std::find_if(target_lrq_queue.begin(), target_lrq_queue.end(),
                                     [&](const LockRequest *lr) { return !lr->granted_; });
      new_request = new LockRequest(txn->GetTransactionId(), lock_mode, oid);
      new_it = target_lrq_queue.insert(insert_pos, new_request);
    }
    // if (txn->GetTransactionId() == 2) {
    // LOG_DEBUG("txn2 start acquire lock");
    // }
    target_lrq->cv_.wait(lk, [&]() { return CheckCompatibility(txn, lock_mode, target_lrq_queue); });
    if (is_update) {
      target_lrq->upgrading_ = INVALID_PAGE_ID;
    }

    if (txn->GetState() == TransactionState::ABORTED) {
      //      if (is_update) {
      //        std::shared_ptr<std::unordered_set<table_oid_t>> org_lock_set = GetLockSet(txn, org_lock_mode);
      //        org_lock_set->insert(oid);
      //      }
      // LOG_DEBUG("erase request for txn%d", (*new_it)->txn_id_);
      target_lrq_queue.erase(new_it);
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
