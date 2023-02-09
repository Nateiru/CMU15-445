#include "execution/plans/limit_plan.h"
#include "execution/plans/sort_plan.h"
#include "execution/plans/topn_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {

auto Optimizer::OptimizeSortLimitAsTopN(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(student): implement sort + limit -> top N optimizer rule
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeSortLimitAsTopN(child));
  }
  auto optimized_plan = plan->CloneWithChildren(std::move(children));

  if (optimized_plan->GetType() == PlanType::Limit) {
    const auto &st_plan = dynamic_cast<const LimitPlanNode &>(*optimized_plan);
    BUSTUB_ENSURE(st_plan.children_.size() == 1, "st should have exactly 1 children.");
    if (st_plan.GetChildPlan()->GetType() == PlanType::Sort) {
      const auto &child_plan = dynamic_cast<const SortPlanNode &>(*st_plan.GetChildPlan());
      BUSTUB_ENSURE(child_plan.children_.size() == 1, "st should have exactly 1 children.");

      return std::make_shared<TopNPlanNode>(st_plan.output_schema_, child_plan.GetChildPlan(), child_plan.GetOrderBy(),
                                            st_plan.GetLimit());
    }
  }

  return optimized_plan;
}

}  // namespace bustub
