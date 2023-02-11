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
#include "execution/expressions/logic_expression.h"
#include "execution/expressions/comparison_expression.h"

namespace bustub {

auto PushDownExpressionForJoin(const AbstractExpressionRef expr, size_t left_column_cnt,
                                         size_t right_column_cnt) -> std::pair<AbstractExpressionRef, AbstractExpressionRef>{
  
  std::vector<AbstractExpressionRef> left_children;
  std::vector<AbstractExpressionRef> right_children;

  for (const auto &child : expr->GetChildren()) {
    auto ret = PushDownExpressionForJoin(child, left_column_cnt, right_column_cnt);
    if (ret.first != nullptr) {
      left_children.emplace_back(ret.first);
    }
    if (ret.second != nullptr) {
      right_children.emplace_back(ret.second);
    }
  }
  if (const auto *comparison_expr = dynamic_cast<const ComparisonExpression *>(expr.get());
      comparison_expr != nullptr) { 
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(0).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression*>(expr->GetChildAt(1).get());
          constant_value_expr != nullptr) {
        
        auto col_idx = column_value_expr->GetColIdx();
        std::cout << "===================col_idx: " << col_idx <<std::endl;
        std::cout << left_column_cnt << ' '<<right_column_cnt <<'\n';
        if (col_idx < left_column_cnt) {
          AbstractExpressionRef x = std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
          AbstractExpressionRef y = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children {x, y};
          AbstractExpressionRef lret = expr->CloneWithChildren(children);
          AbstractExpressionRef rret; 
          return std::make_pair(lret, rret);
        }
        if (col_idx >= left_column_cnt && col_idx < left_column_cnt + right_column_cnt) {

          AbstractExpressionRef x = std::make_shared<ColumnValueExpression>(0, col_idx - left_column_cnt, column_value_expr->GetReturnType());
          AbstractExpressionRef y = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children {x, y};
          AbstractExpressionRef rret = expr->CloneWithChildren(children);
          AbstractExpressionRef lret; 
          return std::make_pair(lret, rret);
        }
      }
    }
    if (const auto *column_value_expr = dynamic_cast<const ColumnValueExpression *>(expr->GetChildAt(1).get());
        column_value_expr != nullptr) {
      if (const auto *constant_value_expr = dynamic_cast<const ConstantValueExpression*>(expr->GetChildAt(0).get());
          constant_value_expr != nullptr) {
        auto col_idx = column_value_expr->GetColIdx();
        std::cout << "===================col_idx: " << col_idx <<std::endl;
        if (col_idx < left_column_cnt) {
          AbstractExpressionRef y = std::make_shared<ColumnValueExpression>(0, col_idx, column_value_expr->GetReturnType());
          AbstractExpressionRef x = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children {x, y};
          AbstractExpressionRef lret = expr->CloneWithChildren(children);
          AbstractExpressionRef rret; 
          return std::make_pair(lret, rret);
        }
        if (col_idx >= left_column_cnt && col_idx < left_column_cnt + right_column_cnt) {

          AbstractExpressionRef y = std::make_shared<ColumnValueExpression>(0, col_idx - left_column_cnt, column_value_expr->GetReturnType());
          AbstractExpressionRef x = std::make_shared<ConstantValueExpression>(constant_value_expr->val_);
          std::vector<AbstractExpressionRef> children {x, y};
          AbstractExpressionRef rret = expr->CloneWithChildren(children);
          AbstractExpressionRef lret; 
          return std::make_pair(lret, rret);
        }
      }
    }
  }
  assert(left_children.size() <= 2 && right_children.size() <= 2);
  AbstractExpressionRef lret{nullptr}, rret{nullptr};
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

auto PredicatePushDown(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  if (plan->GetType() == PlanType::Filter) {
    const auto &filter_plan = dynamic_cast<const FilterPlanNode &>(*plan);
    BUSTUB_ENSURE(filter_plan.children_.size() == 1, "FilterPlan should have exactly 1 children."); 
    if (plan->children_[0]->GetType() == PlanType::NestedLoopJoin) {
      std::cout << "Begin PredicatePushDown\n" <<std::endl; 
      const auto &nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*plan->children_[0]); 
      const auto &left_plan =  nlj_plan.GetLeftPlan();
      const auto &right_plan = nlj_plan.GetRightPlan();
      const auto expr = filter_plan.GetPredicate();
      auto [left_predicate, right_predicate] = PushDownExpressionForJoin(expr, 
                                               left_plan->OutputSchema().GetColumnCount(),
                                               right_plan->OutputSchema().GetColumnCount());
      std::vector<AbstractPlanNodeRef> children;
      if (left_predicate != nullptr && right_predicate != nullptr) {
        AbstractPlanNodeRef left_filter = std::make_shared<FilterPlanNode> (
                           std::make_shared<Schema>(left_plan->OutputSchema()), 
                           std::move(left_predicate), std::move(left_plan));
        AbstractPlanNodeRef right_filter= std::make_shared<FilterPlanNode> (
                           std::make_shared<Schema>(right_plan->OutputSchema()), 
                           std::move(right_predicate), std::move(right_plan));
        
        auto new_nlj_plan= std::make_shared<NestedLoopJoinPlanNode>(
                           nlj_plan.output_schema_,
                           left_filter, right_filter,
                           nlj_plan.predicate_, nlj_plan.GetJoinType());

        children.emplace_back(new_nlj_plan);
      }
      else if (left_predicate != nullptr && right_predicate == nullptr) {
        auto left_filter = std::make_shared<FilterPlanNode> (
                           std::make_shared<Schema>(left_plan->OutputSchema()), 
                           left_predicate, left_plan);
        auto new_nlj_plan= std::make_shared<NestedLoopJoinPlanNode>(
                           nlj_plan.output_schema_,
                           left_filter, right_plan,
                           nlj_plan.predicate_, nlj_plan.GetJoinType());
        children.emplace_back(new_nlj_plan);
      }
      else if (left_predicate == nullptr && right_predicate != nullptr) { 
        auto right_filter =std::make_shared<FilterPlanNode> (
                           std::make_shared<Schema>(right_plan->OutputSchema()), 
                           right_predicate, right_plan);
        auto new_nlj_plan= std::make_shared<NestedLoopJoinPlanNode>(
                           nlj_plan.output_schema_,
                           left_plan, right_filter,
                           nlj_plan.predicate_, nlj_plan.GetJoinType());
        children.emplace_back(new_nlj_plan);
      }
      else {
        // nlj_plan 不需要改变原封不动
        children.emplace_back(plan->children_[0]);
      }
      return plan->CloneWithChildren(children);
    }
  }
  return plan;
}

auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  std::cout << "OptimizeNLJAsHashJoin\n" <<std::endl;

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
