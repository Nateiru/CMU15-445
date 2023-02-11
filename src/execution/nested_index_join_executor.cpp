//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_index_join_executor.cpp
//
// Identification: src/execution/nested_index_join_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_index_join_executor.h"
#include "type/value_factory.h"

namespace bustub {

NestIndexJoinExecutor::NestIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                                             std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2022 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestIndexJoinExecutor::Init() {
  child_executor_->Init();
  auto index_info = GetExecutorContext()->GetCatalog()->GetIndex(plan_->GetIndexOid());
  b_plus_tree_index_ = dynamic_cast<BPlusTreeIndexForOneIntegerColumn *>(index_info->index_.get());
  table_heap_ = GetExecutorContext()->GetCatalog()->GetTable(index_info->table_name_)->table_.get();
}

auto NestIndexJoinExecutor::GetValuesFromTuple(const Tuple *tuple, const Schema *output_schema) -> std::vector<Value> {
  std::vector<Value> ret;
  ret.reserve(output_schema->GetColumnCount());
  for (const Column &col : output_schema->GetColumns()) {
    Value val = tuple->GetValue(output_schema, output_schema->GetColIdx(col.GetName()));
    ret.push_back(val);
  }
  return ret;
}

auto NestIndexJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple left_tuple;
  while (true) {
    if (!right_rids_.empty()) {
      RID right_rid = right_rids_.back();
      right_rids_.pop_back();
      Tuple right_tuple;
      table_heap_->GetTuple(right_rid, &right_tuple, GetExecutorContext()->GetTransaction());

      std::vector<Value> left_values = GetValuesFromTuple(&left_tuple, &child_executor_->GetOutputSchema());
      std::vector<Value> right_values = GetValuesFromTuple(&right_tuple, &plan_->InnerTableSchema());
      left_values.insert(left_values.end(), right_values.begin(), right_values.end());
      *tuple = Tuple(std::move(left_values), &GetOutputSchema());
      return true;
    }
    if (!child_executor_->Next(&left_tuple, rid)) {
      return false;
    }

    Value key_value = plan_->KeyPredicate()->Evaluate(&left_tuple, child_executor_->GetOutputSchema());
    
    Schema key_schema{std::vector<Column>{Column("key_schema", plan_->KeyPredicate()->GetReturnType())}};
    Tuple key_tuple = Tuple(std::vector<Value>{key_value}, &key_schema);
    b_plus_tree_index_->ScanKey(key_tuple, &right_rids_, GetExecutorContext()->GetTransaction());

    if (right_rids_.empty() && plan_->GetJoinType() == JoinType::LEFT) {
      std::vector<Value> left_values = GetValuesFromTuple(&left_tuple, &child_executor_->GetOutputSchema());
      for (const Column &col : plan_->InnerTableSchema().GetColumns()) {
        left_values.emplace_back(ValueFactory::GetNullValueByType(col.GetType()));
      }
      *tuple = Tuple(std::move(left_values), &GetOutputSchema());
      return true;
    }
  }
}

}  // namespace bustub
