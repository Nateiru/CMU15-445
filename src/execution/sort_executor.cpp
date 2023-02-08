#include "execution/executors/sort_executor.h"

namespace bustub {

SortExecutor::SortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void SortExecutor::Init() {
  std::vector<std::pair<Tuple, RID>>().swap(sorted_tuples_);
  child_executor_->Init();
  Tuple tuple;
  RID rid;
  while (child_executor_->Next(&tuple, &rid)) {
    sorted_tuples_.emplace_back(tuple, rid);
  }
  std::sort(sorted_tuples_.begin(), sorted_tuples_.end(),
            [this](const std::pair<Tuple, RID> &a, const std::pair<Tuple, RID> &b) {
              for (auto [order_by_type, expr] : plan_->GetOrderBy()) {
                const Value x = expr->Evaluate(&a.first, child_executor_->GetOutputSchema());
                const Value y = expr->Evaluate(&b.first, child_executor_->GetOutputSchema());
                if (x.CompareEquals(y) == CmpBool::CmpTrue) {
                  continue;
                }
                switch (order_by_type) {
                  case OrderByType::INVALID:
                  case OrderByType::ASC:
                  case OrderByType::DEFAULT:
                    return x.CompareLessThan(y) == CmpBool::CmpTrue;
                    break;
                  case OrderByType::DESC:
                    return x.CompareGreaterThan(y) == CmpBool::CmpTrue;
                    break;
                }
              }
              UNREACHABLE("doesn't support duplicate key");
            });
  std::reverse(sorted_tuples_.begin(), sorted_tuples_.end());
}

auto SortExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (!sorted_tuples_.empty()) {
    *tuple = sorted_tuples_.back().first;
    *rid = sorted_tuples_.back().second;
    sorted_tuples_.pop_back();
    return true;
  }
  return false;
}

}  // namespace bustub
