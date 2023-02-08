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
    const auto &nlj_plan = dynamic_cast<const LimitPlanNode &>(*optimized_plan);
    BUSTUB_ENSURE(nlj_plan.children_.size() == 1, "NLJ should have exactly 1 children.");
    if (nlj_plan.GetChildPlan()->GetType() == PlanType::Sort) {
      const auto &child_plan = dynamic_cast<const SortPlanNode &>(*nlj_plan.GetChildPlan());
      BUSTUB_ENSURE(child_plan.children_.size() == 1, "NLJ should have exactly 1 children.");

      return std::make_shared<TopNPlanNode>(nlj_plan.output_schema_, child_plan.GetChildPlan(), child_plan.GetOrderBy(),
                                            nlj_plan.GetLimit());
    }
  }

  return optimized_plan;
}

}  // namespace bustub
