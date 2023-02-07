//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"
#include "type/type_id.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void InsertExecutor::Init() {
	table_info_ = GetExecutorContext()->GetCatalog()->GetTable(plan_->TableOid());
	table_indexes_ = GetExecutorContext()->GetCatalog()->GetTableIndexes(table_info_->name_);
	used_ = false;
	child_executor_->Init();
}

auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
	if (used_) {
		return false;
	}
	used_ = true;
	int cnt = 0;
	while (child_executor_->Next(tuple, rid)) {
		table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
		for (auto index_info : table_indexes_) {
			const auto key = tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
			index_info->index_->InsertEntry(key, *rid, GetExecutorContext()->GetTransaction());
		}
		cnt++;
	}
	Schema ret_schema{std::vector<Column>{Column("__bustub_internal.insert_rows", TypeId::INTEGER)}};
	*tuple = Tuple{std::vector<Value>{Value(TypeId::INTEGER, cnt)}, &ret_schema};
	return true;
}

}  // namespace bustub
