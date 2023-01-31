/**
 * index_iterator.cpp
 */
#include <cassert>

#include "storage/index/index_iterator.h"

namespace bustub {

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(B_PLUS_TREE_LEAF_PAGE_TYPE *leaf, int index, BufferPoolManager *buffer_pool_manager)
                    :leaf_(leaf), index_(index), buffer_pool_manager_(buffer_pool_manager){}


INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool {
    return (leaf_ == nullptr);
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> const MappingType & {
    assert(leaf_ != nullptr);
    return leaf_->GetItem(index_);
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
    if (IsEnd()) {
        return *this;
    }
    ++index_;
    if (index_ >= leaf_->GetSize()) {
        page_id_t next_page_id = leaf_->GetNextPageId();
        if (next_page_id == INVALID_PAGE_ID) {
            leaf_ = nullptr;
            index_ = 0;
        }
        else {
            auto page = buffer_pool_manager_->FetchPage(next_page_id);
            leaf_ = reinterpret_cast<B_PLUS_TREE_LEAF_PAGE_TYPE *>(page->GetData());
            buffer_pool_manager_->UnpinPage(page->GetPageId(), false);
            index_ = 0;
        }
    }
    return *this; 
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
