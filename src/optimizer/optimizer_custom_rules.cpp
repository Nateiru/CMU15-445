#include <algorithm>
#include <memory>
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/macros.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/mock_scan_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "execution/plans/projection_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"
#include "type/type_id.h"

// Note for 2022 Fall: You can add all optimizer rule implementations and apply the rules as you want in this file. Note
// that for some test cases, we force using starter rules, so that the configuration here won't take effects. Starter
// rule can be forcibly enabled by `set force_optimizer_starter_rule=yes`.

namespace bustub {

auto PushDownExpressionForJoin(const AbstractExpressionRef &expr)
    -> std::pair<AbstractExpressionRef, AbstractExpressionRef> {
  if (const auto *comparison_expr = dynamic_cast<const ComparisonExpression *>(expr.get());
      comparison_expr != nullptr) {
    assert(comparison_expr->GetChildren().size() == 2);
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(0).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression *>(expr->GetChildAt(1).get());
          constant_value_expr != nullptr) {
        auto col_idx = column_value_expr->GetColIdx();
        if (column_value_expr->GetTupleIdx() == 0) {
          AbstractExpressionRef x =
              std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
          AbstractExpressionRef y = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children{x, y};
          AbstractExpressionRef lret = comparison_expr->CloneWithChildren(children);
          AbstractExpressionRef rret{nullptr};
          return std::make_pair(lret, rret);
        }
        // else {
        AbstractExpressionRef x =
            std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
        AbstractExpressionRef y = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
        std::vector<AbstractExpressionRef> children{x, y};
        AbstractExpressionRef rret = comparison_expr->CloneWithChildren(children);
        AbstractExpressionRef lret{nullptr};
        return std::make_pair(lret, rret);
      }
    }
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(1).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression *>(expr->GetChildAt(0).get());
          constant_value_expr != nullptr) {
        auto col_idx = column_value_expr->GetColIdx();
        if (column_value_expr->GetTupleIdx() == 0) {
          AbstractExpressionRef y =
              std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
          AbstractExpressionRef x = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children{x, y};
          AbstractExpressionRef lret = comparison_expr->CloneWithChildren(children);
          AbstractExpressionRef rret{nullptr};
          return std::make_pair(lret, rret);
        }
        // else {
        AbstractExpressionRef y =
            std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
        AbstractExpressionRef x = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
        std::vector<AbstractExpressionRef> children{x, y};
        AbstractExpressionRef rret = comparison_expr->CloneWithChildren(children);
        AbstractExpressionRef lret{nullptr};
        return std::make_pair(lret, rret);
      }
    }
    if (const auto *left_column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(0).get());
        left_column_value_expr != nullptr) {
      if (const auto *right_column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(1).get());
          right_column_value_expr != nullptr) {
        if (left_column_value_expr->GetTupleIdx() == 0 && right_column_value_expr->GetTupleIdx() == 0) {
          AbstractExpressionRef x = std::make_shared<ColumnValueExpression>(0, left_column_value_expr->GetColIdx(),
                                                                            left_column_value_expr->GetReturnType());
          AbstractExpressionRef y = std::make_shared<ColumnValueExpression>(0, right_column_value_expr->GetColIdx(),
                                                                            right_column_value_expr->GetReturnType());
          std::vector<AbstractExpressionRef> children{x, y};
          AbstractExpressionRef lret = comparison_expr->CloneWithChildren(children);
          AbstractExpressionRef rret{nullptr};
          return std::make_pair(lret, rret);
        }
        if (left_column_value_expr->GetTupleIdx() == 1 && right_column_value_expr->GetTupleIdx() == 1) {
          AbstractExpressionRef x = std::make_shared<ColumnValueExpression>(0, left_column_value_expr->GetColIdx(),
                                                                            left_column_value_expr->GetReturnType());
          AbstractExpressionRef y = std::make_shared<ColumnValueExpression>(0, right_column_value_expr->GetColIdx(),
                                                                            right_column_value_expr->GetReturnType());
          std::vector<AbstractExpressionRef> children{x, y};
          AbstractExpressionRef rret = comparison_expr->CloneWithChildren(children);
          AbstractExpressionRef lret{nullptr};
          return std::make_pair(lret, rret);
        }
      }
    }
  }
  std::vector<AbstractExpressionRef> left_children;
  std::vector<AbstractExpressionRef> right_children;

  for (const auto &child : expr->GetChildren()) {
    auto ret = PushDownExpressionForJoin(child);
    if (ret.first != nullptr) {
      left_children.emplace_back(ret.first);
    }
    if (ret.second != nullptr) {
      right_children.emplace_back(ret.second);
    }
  }
  assert(left_children.size() <= 2 && right_children.size() <= 2);
  AbstractExpressionRef lret{nullptr};
  AbstractExpressionRef rret{nullptr};
  if (left_children.size() == 1) {
    lret = left_children[0];
  }
  if (left_children.size() == 2) {
    lret = std::make_shared<LogicExpression>(left_children[0], left_children[1], LogicType::And);
  }
  if (right_children.size() == 1) {
    rret = right_children[0];
  }
  if (right_children.size() == 2) {
    rret = std::make_shared<LogicExpression>(right_children[0], right_children[1], LogicType::And);
  }
  return std::make_pair(lret, rret);
}
auto ReWriteExpression(const AbstractExpressionRef &expr) -> AbstractExpressionRef {
  if (expr->children_.empty()) {
    return expr;
  }

  if (const auto *comparison_expr = dynamic_cast<const ComparisonExpression *>(expr.get());
      comparison_expr != nullptr) {
    assert(comparison_expr->GetChildren().size() == 2);
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(0).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression *>(expr->GetChildAt(1).get());
          constant_value_expr != nullptr) {
        return nullptr;
      }
    }
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(1).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression *>(expr->GetChildAt(0).get());
          constant_value_expr != nullptr) {
        return nullptr;
      }
    }
    if (const auto *left_column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(0).get());
        left_column_value_expr != nullptr) {
      if (const auto *right_column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(1).get());
          right_column_value_expr != nullptr) {
        if (left_column_value_expr->GetTupleIdx() == 0 && right_column_value_expr->GetTupleIdx() == 0) {
          return nullptr;
        }
        if (left_column_value_expr->GetTupleIdx() == 1 && right_column_value_expr->GetTupleIdx() == 1) {
          return nullptr;
        }
      }
    }
    return expr;
  }

  std::vector<AbstractExpressionRef> children;

  for (const auto &child : expr->GetChildren()) {
    auto ret = ReWriteExpression(child);
    if (ret != nullptr) {
      children.emplace_back(ret);
    }
  }
  assert(children.size() <= 2);
  if (children.empty()) {
    return nullptr;
  }
  if (children.size() == 1) {
    return children[0];
  }
  return expr->CloneWithChildren(children);
}
auto RewriteExpressionForJoin(const AbstractExpressionRef &expr, size_t left_column_cnt, size_t right_column_cnt)
    -> AbstractExpressionRef {
  std::vector<AbstractExpressionRef> children;
  for (const auto &child : expr->GetChildren()) {
    children.emplace_back(RewriteExpressionForJoin(child, left_column_cnt, right_column_cnt));
  }
  if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr.get());
      column_value_expr != nullptr) {
    BUSTUB_ENSURE(column_value_expr->GetTupleIdx() == 0, "tuple_idx cannot be value other than 0 before this stage.")
    auto col_idx = column_value_expr->GetColIdx();
    if (col_idx < left_column_cnt) {
      return std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
    }
    if (col_idx >= left_column_cnt && col_idx < left_column_cnt + right_column_cnt) {
      return std::make_shared<ColumnValueExpression>(1, col_idx - left_column_cnt, column_value_expr->GetReturnType());
    }
    throw bustub::Exception("col_idx not in range");
  }
  return expr->CloneWithChildren(children);
}
auto IsPredicateTrue(const AbstractExpression &expr) -> bool {
  if (const auto *const_expr = dynamic_cast<const ConstantValueExpression *>(&expr); const_expr != nullptr) {
    return const_expr->val_.CastAs(TypeId::BOOLEAN).GetAs<bool>();
  }
  return false;
}
auto PredicatePushDown(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  if (plan->GetType() == PlanType::NestedLoopJoin) {
    const auto &nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*plan);
    BUSTUB_ENSURE(nlj_plan.children_.size() == 2, "NLJ Plan should have exactly 2 children.");
    const auto &left_plan = nlj_plan.GetLeftPlan();
    const auto &right_plan = nlj_plan.GetRightPlan();
    const auto expr = nlj_plan.predicate_;
    auto [left_predicate, right_predicate] = PushDownExpressionForJoin(expr);

    // auto last_predicate = ReWriteExpression(expr);
    // AbstractExpressionRef nlj_predicate;

    auto nlj_predicate = ReWriteExpression(expr);

    std::vector<AbstractPlanNodeRef> children;
    if (left_predicate != nullptr && right_predicate != nullptr) {
      AbstractPlanNodeRef left_filter = std::make_shared<FilterPlanNode>(
          std::make_shared<Schema>(left_plan->OutputSchema()), left_predicate, left_plan);
      AbstractPlanNodeRef right_filter = std::make_shared<FilterPlanNode>(
          std::make_shared<Schema>(right_plan->OutputSchema()), right_predicate, right_plan);

      if (left_plan->GetType() == PlanType::NestedLoopJoin && right_plan->GetType() == PlanType::NestedLoopJoin) {
        const auto &left_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*left_plan);
        const auto &right_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*right_plan);
        auto left_nlj_predicate =
            RewriteExpressionForJoin(left_predicate, left_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     left_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        auto right_nlj_predicate =
            RewriteExpressionForJoin(right_predicate, right_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     right_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        if (!IsPredicateTrue(left_nlj_plan.Predicate())) {
          left_nlj_predicate =
              std::make_shared<LogicExpression>(left_nlj_predicate, left_nlj_plan.predicate_, LogicType::And);
        }
        if (!IsPredicateTrue(right_nlj_plan.Predicate())) {
          right_nlj_predicate =
              std::make_shared<LogicExpression>(right_nlj_predicate, right_nlj_plan.predicate_, LogicType::And);
        }
        auto new_left_plan = std::make_shared<NestedLoopJoinPlanNode>(
            left_nlj_plan.output_schema_, left_nlj_plan.GetLeftPlan(), left_nlj_plan.GetRightPlan(), left_nlj_predicate,
            left_nlj_plan.GetJoinType());
        auto new_right_plan = std::make_shared<NestedLoopJoinPlanNode>(
            right_nlj_plan.output_schema_, right_nlj_plan.GetLeftPlan(), right_nlj_plan.GetRightPlan(),
            right_nlj_predicate, right_nlj_plan.GetJoinType());

        return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, new_left_plan, new_right_plan,
                                                        nlj_predicate, nlj_plan.GetJoinType());
      }
      if (left_plan->GetType() == PlanType::NestedLoopJoin && right_plan->GetType() != PlanType::NestedLoopJoin) {
        const auto &left_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*left_plan);
        auto left_nlj_predicate =
            RewriteExpressionForJoin(left_predicate, left_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     left_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        if (!IsPredicateTrue(left_nlj_plan.Predicate())) {
          left_nlj_predicate =
              std::make_shared<LogicExpression>(left_nlj_predicate, left_nlj_plan.predicate_, LogicType::And);
        }
        auto new_left_plan = std::make_shared<NestedLoopJoinPlanNode>(
            left_nlj_plan.output_schema_, left_nlj_plan.GetLeftPlan(), left_nlj_plan.GetRightPlan(), left_nlj_predicate,
            left_nlj_plan.GetJoinType());

        return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, new_left_plan, right_filter,
                                                        nlj_predicate, nlj_plan.GetJoinType());
      }
      if (left_plan->GetType() != PlanType::NestedLoopJoin && right_plan->GetType() == PlanType::NestedLoopJoin) {
        const auto &right_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*right_plan);
        auto right_nlj_predicate =
            RewriteExpressionForJoin(right_predicate, right_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     right_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        if (!IsPredicateTrue(right_nlj_plan.Predicate())) {
          right_nlj_predicate =
              std::make_shared<LogicExpression>(right_nlj_predicate, right_nlj_plan.predicate_, LogicType::And);
        }
        auto new_right_plan = std::make_shared<NestedLoopJoinPlanNode>(
            right_nlj_plan.output_schema_, right_nlj_plan.GetLeftPlan(), right_nlj_plan.GetRightPlan(),
            right_nlj_predicate, right_nlj_plan.GetJoinType());

        return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, left_filter, new_right_plan,
                                                        nlj_predicate, nlj_plan.GetJoinType());
      }
      return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, left_filter, right_filter, nlj_predicate,
                                                      nlj_plan.GetJoinType());
    }
    if (left_predicate != nullptr && right_predicate == nullptr) {
      auto left_filter = std::make_shared<FilterPlanNode>(std::make_shared<Schema>(left_plan->OutputSchema()),
                                                          left_predicate, left_plan);
      if (left_plan->GetType() == PlanType::NestedLoopJoin) {
        const auto &left_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*left_plan);
        auto left_nlj_predicate =
            RewriteExpressionForJoin(left_predicate, left_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     left_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        if (!IsPredicateTrue(left_nlj_plan.Predicate())) {
          left_nlj_predicate =
              std::make_shared<LogicExpression>(left_nlj_predicate, left_nlj_plan.predicate_, LogicType::And);
        }
        auto new_left_plan = std::make_shared<NestedLoopJoinPlanNode>(
            left_nlj_plan.output_schema_, left_nlj_plan.GetLeftPlan(), left_nlj_plan.GetRightPlan(), left_nlj_predicate,
            left_nlj_plan.GetJoinType());
        return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, new_left_plan, right_plan,
                                                        nlj_predicate, nlj_plan.GetJoinType());
      }
      return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, left_filter, right_plan, nlj_predicate,
                                                      nlj_plan.GetJoinType());
    }
    if (left_predicate == nullptr && right_predicate != nullptr) {
      auto right_filter = std::make_shared<FilterPlanNode>(std::make_shared<Schema>(right_plan->OutputSchema()),
                                                           right_predicate, right_plan);
      if (right_plan->GetType() == PlanType::NestedLoopJoin) {
        const auto &right_nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*right_plan);

        auto right_nlj_predicate =
            RewriteExpressionForJoin(right_predicate, right_nlj_plan.GetLeftPlan()->OutputSchema().GetColumnCount(),
                                     right_nlj_plan.GetRightPlan()->OutputSchema().GetColumnCount());
        if (!IsPredicateTrue(right_nlj_plan.Predicate())) {
          right_nlj_predicate =
              std::make_shared<LogicExpression>(right_nlj_predicate, right_nlj_plan.predicate_, LogicType::And);
        }
        auto new_right_plan = std::make_shared<NestedLoopJoinPlanNode>(
            right_nlj_plan.output_schema_, right_nlj_plan.GetLeftPlan(), right_nlj_plan.GetRightPlan(),
            right_nlj_predicate, right_nlj_plan.GetJoinType());

        return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, left_plan, new_right_plan,
                                                        nlj_predicate, nlj_plan.GetJoinType());
      }
      return std::make_shared<NestedLoopJoinPlanNode>(nlj_plan.output_schema_, left_plan, right_filter, nlj_predicate,
                                                      nlj_plan.GetJoinType());
    }
    return plan;
  }
  return plan;
}

auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  auto optimized_plan = PredicatePushDown(plan);

  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : optimized_plan->GetChildren()) {
    children.emplace_back(OptimizeNLJAsHashJoin(child));
  }
  optimized_plan = optimized_plan->CloneWithChildren(children);
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
            /**
            //  框架代码：此时无【小表驱动大表】
            // Now it's in form of <column_expr> = <column_expr>. Let's check if one of them is from the left table, and
            // the other is from the right table.
            if (left_expr->GetTupleIdx() == 0 && right_expr->GetTupleIdx() == 1) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, nlj_plan.GetLeftPlan(),
                                                        nlj_plan.GetRightPlan(), std::move(left_expr_tuple_0),
                                                        std::move(right_expr_tuple_0), nlj_plan.GetJoinType());
            }
            if (left_expr->GetTupleIdx() == 1 && right_expr->GetTupleIdx() == 0) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, nlj_plan.GetLeftPlan(),
                                                        nlj_plan.GetRightPlan(), std::move(right_expr_tuple_0),
                                                        std::move(left_expr_tuple_0), nlj_plan.GetJoinType());
            }
            */

            // 小表驱动大表 需要比较两边表的大小
            if (nlj_plan.GetJoinType() == JoinType::INNER &&
                (nlj_plan.GetLeftPlan()->GetType() == PlanType::MockScan ||
                 nlj_plan.GetLeftPlan()->GetType() == PlanType::SeqScan) &&
                (nlj_plan.GetRightPlan()->GetType() == PlanType::MockScan ||
                 nlj_plan.GetRightPlan()->GetType() == PlanType::SeqScan)) {
              std::optional<size_t> lsize;
              std::optional<size_t> rsize;
              if (nlj_plan.GetLeftPlan()->GetType() == PlanType::MockScan) {
                const auto &lms_plan = dynamic_cast<const MockScanPlanNode &>(*nlj_plan.GetLeftPlan());
                lsize = EstimatedCardinality(lms_plan.GetTable());
              } else {
                const auto &lms_plan = dynamic_cast<const SeqScanPlanNode &>(*nlj_plan.GetLeftPlan());
                lsize = EstimatedCardinality(lms_plan.table_name_);
              }
              if (nlj_plan.GetRightPlan()->GetType() == PlanType::MockScan) {
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
            /**
             * 默认假设 Hashjoin 的数量比较大，因为是两个表的笛卡儿积 让其在右边作为大表
             */
            if (nlj_plan.GetJoinType() == JoinType::INNER && nlj_plan.GetLeftPlan()->GetType() == PlanType::HashJoin &&
                nlj_plan.GetRightPlan()->GetType() == PlanType::Filter) {
              std::swap(left_plan, right_plan);
              std::swap(left_expr_tuple_0, right_expr_tuple_0);
            }
            if (left_expr->GetTupleIdx() == 0 && right_expr->GetTupleIdx() == 1) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, left_plan, right_plan,
                                                        std::move(left_expr_tuple_0), std::move(right_expr_tuple_0),
                                                        nlj_plan.GetJoinType());
            }
            if (left_expr->GetTupleIdx() == 1 && right_expr->GetTupleIdx() == 0) {
              return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, left_plan, right_plan,
                                                        std::move(right_expr_tuple_0), std::move(left_expr_tuple_0),
                                                        nlj_plan.GetJoinType());
            }
          }
        }
      }
    }
  }

  return optimized_plan;
}
auto Optimizer::OptimizeCustom(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  auto p = plan;
  p = OptimizeMergeProjection(p);
  p = OptimizeMergeFilterNLJ(p);
  p = OptimizeNLJAsIndexJoin(p);
  p = OptimizeNLJAsHashJoin(p);  // Enable this rule after you have implemented hash join.
  p = OptimizeOrderByAsIndexScan(p);
  p = OptimizeSortLimitAsTopN(p);
  return p;
}

}  // namespace bustub
