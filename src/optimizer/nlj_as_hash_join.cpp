#include <algorithm>
#include <memory>
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/macros.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "execution/plans/projection_plan.h"
#include "optimizer/optimizer.h"
#include "type/type_id.h"
#include "execution/plans/mock_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"

namespace bustub {

auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeNLJAsHashJoin(child));
  }
  auto optimized_plan = plan->CloneWithChildren(std::move(children));

  if (optimized_plan->GetType() == PlanType::NestedLoopJoin) {
    const auto &nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*optimized_plan);
    // Has exactly two children
    BUSTUB_ENSURE(nlj_plan.children_.size() == 2, "NLJ should have exactly 2 children.");

    // Check if expr is equal condition where one is for the left table, and one is for the right table.
    if (const auto *expr = dynamic_cast<const ComparisonExpression *>(&nlj_plan.Predicate()); expr != nullptr) {
      if (expr->comp_type_ == ComparisonType::Equal) {
        if (const auto *left_expr = dynamic_cast<const ColumnValueExpression *>(expr->children_[0].get());
            left_expr != nullptr) {
          if (const auto *right_expr = dynamic_cast<const ColumnValueExpression *>(expr->children_[1].get());
              right_expr != nullptr) {
            // Ensure both exprs have tuple_id == 0
            auto left_expr_tuple_0 =
                std::make_shared<ColumnValueExpression>(0, left_expr->GetColIdx(), left_expr->GetReturnType());
            auto right_expr_tuple_0 =
                std::make_shared<ColumnValueExpression>(0, right_expr->GetColIdx(), right_expr->GetReturnType());
            auto left_plan = nlj_plan.GetLeftPlan();
            auto right_plan = nlj_plan.GetRightPlan();
            /**  框架代码：此时无【小表驱动大表】  
             * // Now it's in form of <column_expr> = <column_expr>. Let's check if one of them is from the left table, and
             * // the other is from the right table.
             * if (left_expr->GetTupleIdx() == 0 && right_expr->GetTupleIdx() == 1) {
             *   return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, nlj_plan.GetLeftPlan(),
             *                                             nlj_plan.GetRightPlan(), std::move(left_expr_tuple_0),
             *                                             std::move(right_expr_tuple_0), nlj_plan.GetJoinType());
             * }
             * if (left_expr->GetTupleIdx() == 1 && right_expr->GetTupleIdx() == 0) {
             *   return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, nlj_plan.GetLeftPlan(),
             *                                             nlj_plan.GetRightPlan(), std::move(right_expr_tuple_0),
             *                                             std::move(left_expr_tuple_0), nlj_plan.GetJoinType());
             * }
             */
            // 小表驱动大表 需要比较两边表的大小
            if (nlj_plan.GetJoinType() == JoinType::INNER  && 
               (nlj_plan.GetLeftPlan()->GetType()  == PlanType::MockScan || nlj_plan.GetLeftPlan()->GetType()  == PlanType::SeqScan)&& 
               (nlj_plan.GetRightPlan()->GetType() == PlanType::MockScan || nlj_plan.GetRightPlan()->GetType() == PlanType::SeqScan)) {
              
              std::optional<size_t> lsize;
              std::optional<size_t> rsize;
              if (nlj_plan.GetLeftPlan()->GetType() == PlanType::MockScan) {
                const auto &lms_plan = dynamic_cast<const MockScanPlanNode &>(*nlj_plan.GetLeftPlan());
                lsize = EstimatedCardinality(lms_plan.GetTable());
              } else {
                const auto &lms_plan = dynamic_cast<const SeqScanPlanNode &>(*nlj_plan.GetLeftPlan());
                lsize = EstimatedCardinality(lms_plan.table_name_);
              }
              if (nlj_plan.GetLeftPlan()->GetType() == PlanType::MockScan) {
                const auto &rms_plan = dynamic_cast<const MockScanPlanNode &>(*nlj_plan.GetRightPlan());
                rsize = EstimatedCardinality(rms_plan.GetTable());
              } else {
                const auto &rms_plan = dynamic_cast<const SeqScanPlanNode &>(*nlj_plan.GetRightPlan());
                rsize = EstimatedCardinality(rms_plan.table_name_);
              }
              // 右表小 => 小表(Left)驱动大表(Right)
              if (rsize < lsize) { 
                std::swap(left_plan, right_plan);
                std::swap(left_expr_tuple_0, right_expr_tuple_0); 
              }
            }
            if (left_expr->GetTupleIdx() == 0 && right_expr->GetTupleIdx() == 1) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, left_plan,
                                                        right_plan, std::move(left_expr_tuple_0),
                                                        std::move(right_expr_tuple_0), nlj_plan.GetJoinType());
            }
            if (left_expr->GetTupleIdx() == 1 && right_expr->GetTupleIdx() == 0) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, left_plan,
                                                        right_plan, std::move(right_expr_tuple_0),
                                                        std::move(left_expr_tuple_0), nlj_plan.GetJoinType());
            }
            
          }
        }
      }
    }
  }

  return optimized_plan;
}

}  // namespace bustub
