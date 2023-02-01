#include <string>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/header_page.h"

namespace bustub {
INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, BufferPoolManager *buffer_pool_manager, const KeyComparator &comparator,
                          int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      root_page_id_(INVALID_PAGE_ID),
      buffer_pool_manager_(buffer_pool_manager),
      comparator_(comparator),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { return (root_page_id_ == INVALID_PAGE_ID); }
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result, Transaction *transaction) -> bool {
  // step 1. find page
  LockRoot(OpType::READ);
  auto target_leaf_page = FindLeafPage(key, false, OpType::READ, transaction);
  if (target_leaf_page == nullptr) {
    UnlatchAndUnpin(OpType::READ, transaction);
    return false;
  }
  // step 2. find value
  RID target_rid;
  bool exist = target_leaf_page->Lookup(key, target_rid, comparator_);
  if (exist) {
    result->clear();
    result->push_back(target_rid);
  }
  UnlatchAndUnpin(OpType::READ, transaction, 1); 
  return exist;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value, Transaction *transaction) -> bool {
  LockRoot(OpType::INSERT);
  if (IsEmpty()) {
    StartNewTree(key, value);
    UnLockRoot(OpType::INSERT);
    return true;
  }
  bool ret = InsertIntoLeaf(key, value, transaction);
  UnlatchAndUnpin(OpType::INSERT, transaction, 1);
  return ret;
}

/**
 *  StartNewTree 分配一块内存区域作为根节点，将一个给定的键值对插入其中
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::StartNewTree(const KeyType &key, const ValueType &value) -> void {
  // step 1. ask for new page from buffer pool manager
  page_id_t new_page_id;
  auto new_page = buffer_pool_manager_->NewPage(&new_page_id);
  // new_page->WLatch();
  if (new_page == nullptr) {
    throw "out of memory";
  }
  assert(new_page != nullptr);
  auto root = reinterpret_cast<LeafPage *>(new_page->GetData());

  // step 2. update b+ tree's root page id
  root->Init(new_page_id, INVALID_PAGE_ID, leaf_max_size_);
  root_page_id_ = new_page_id;
  UpdateRootPageId(1);
  // step 3. insert entry directly into leaf page.
  root->Insert(key, value, comparator_);
  // new_page->WUnlatch();
  buffer_pool_manager_->UnpinPage(new_page_id, true);
}

/* +++++++++++++++++++++++++
 * Insert constant key & value pair into leaf page
 * User needs to first find the right leaf page as insertion target, then look
 * through leaf page to see whether insert key exist or not. If exist, return
 * immdiately, otherwise insert entry. Remember to deal with split if necessary.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertIntoLeaf(const KeyType &key, const ValueType &value, Transaction *transaction) -> bool {
  LeafPage *leaf_page = FindLeafPage(key, false, OpType::INSERT, transaction);
  assert(leaf_page != nullptr);
  ValueType temp_value;
  bool exist = leaf_page->Lookup(key, temp_value, comparator_);
  // B+树的key不能重复 重复返回 false
  if (exist) {
    // buffer_pool_manager_->UnpinPage(leaf_page->GetPageId(), false);
    return false;
  }
  leaf_page->Insert(key, value, comparator_);
  // 如果size不满足条件 需要分裂
  if (leaf_page->GetSize() > leaf_page->GetMaxSize()) {
    LeafPage *new_leaf_page = Split(leaf_page, transaction);
    InsertIntoParent(leaf_page, new_leaf_page->KeyAt(0), new_leaf_page, transaction);
  }
  // buffer_pool_manager_->UnpinPage(leaf_page->GetPageId(), true);
  return true;
}
/*
 * Insert key & value pair into internal page after split
 * @param   old_node      input page from split() method
 * @param   key
 * @param   new_node      returned page from split() method
 * User needs to first find the parent page of old_node, parent node must be
 * adjusted to take info of new_node into account. Remember to deal with split
 * recursively if necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertIntoParent(BPlusTreePage *old_node, const KeyType &key, BPlusTreePage *new_node,
                                      Transaction *transaction) {
  if (old_node->IsRootPage()) {
    auto new_page = buffer_pool_manager_->NewPage(&root_page_id_);
    auto new_root_page = reinterpret_cast<InternalPage *>(new_page->GetData());
    // new root page init
    new_root_page->Init(root_page_id_, INVALID_PAGE_ID, internal_max_size_);
    UpdateRootPageId(0);
    // set parent page id
    new_root_page->PopulateNewRoot(old_node->GetPageId(), key, new_node->GetPageId());
    old_node->SetParentPageId(root_page_id_);
    new_node->SetParentPageId(root_page_id_);
    // fetch page and new page need to unpin page
    // buffer_pool_manager_->UnpinPage(new_node->GetPageId(), true);
    buffer_pool_manager_->UnpinPage(new_root_page->GetPageId(), true);
    return;
  }
  auto parent_page_id = old_node->GetParentPageId();
  auto parent_page = buffer_pool_manager_->FetchPage(parent_page_id);  // parent_page pined
  auto parent_node = reinterpret_cast<InternalPage *>(parent_page->GetData());
  // insert new node after old node
  parent_node->InsertNodeAfter(old_node->GetPageId(), key, new_node->GetPageId());
  new_node->SetParentPageId(parent_page_id);
  // buffer_pool_manager_->UnpinPage(new_node->GetPageId(), true);
  if (parent_node->GetSize() > parent_node->GetMaxSize()) {
    // need to split
    InternalPage *new_parent_node = Split(parent_node, transaction);  // new_parent_node pined
    InsertIntoParent(parent_node, new_parent_node->KeyAt(0), new_parent_node, transaction);
    // buffer_pool_manager_->UnpinPage(new_parent_node->GetPageId(), true);  // unpin new_parent_node
  }
  buffer_pool_manager_->UnpinPage(parent_node->GetPageId(), true);  // unpin parent_node
}
/*
 * Split input page and return newly created page.
 * Using template N to represent either internal page or leaf page.
 * User needs to first ask for new page from buffer pool manager(NOTICE: throw
 * an "out of memory" exception if returned value is nullptr), then move half
 * of key & value pairs from input page to newly created page
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
auto BPLUSTREE_TYPE::Split(N *node, Transaction *transaction) -> N * {
  // step 1 ask a new page
  page_id_t new_page_id;
  Page *new_page = buffer_pool_manager_->NewPage(&new_page_id);  // pinned
  if (new_page == nullptr) {
    throw std::string("out of memory");
  }
  new_page->WLatch();
  if (transaction != nullptr) {
    transaction->AddIntoPageSet(new_page);
  }
  // step 2 move half of key & value pairs from input page to newly created page
  N *new_node = reinterpret_cast<N *>(new_page->GetData());
  new_node->Init(new_page_id, node->GetParentPageId(), node->IsLeafPage() ? leaf_max_size_ : internal_max_size_);
  node->MoveHalfTo(new_node, buffer_pool_manager_);
  // fetch page and new page need to unpin page(do it outside)
  return new_node;
}
/*
 * Find leaf page containing particular key, if leftMost flag == true, find
 * the left most leaf page
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeafPage(const KeyType &key, bool left_most, 
                                  OpType op, Transaction *transaction) -> LeafPage * {
  if (root_page_id_ == INVALID_PAGE_ID) {
    throw std::runtime_error("Unexpected. root_page_id is INVALID_PAGE_ID");
  }
  auto page = buffer_pool_manager_->FetchPage(root_page_id_);
  assert(page != nullptr);
  if (op == OpType::READ) {
    page->RLatch();
  } else {
    page->WLatch();
  }
  if (transaction != nullptr) {
    transaction->AddIntoPageSet(page);
  }
  auto node = reinterpret_cast<BPlusTreePage *>(page);
  while (!node->IsLeafPage()) {
    auto internal_page = reinterpret_cast<InternalPage *>(node);
    page_id_t next_page_id = left_most ? internal_page->ValueAt(0) : internal_page->Lookup(key, comparator_);
    Page *next_page = buffer_pool_manager_->FetchPage(next_page_id);
    auto next_node = reinterpret_cast<BPlusTreePage *>(next_page);
    if (transaction == nullptr) {
      buffer_pool_manager_->UnpinPage(node->GetPageId(), false);
    }else {
      transaction->AddIntoPageSet(page);
    }
    page = next_page;
    node = next_node;
    if (op == OpType::READ) {
      page->RLatch();
      UnlatchAndUnpin(op, transaction);
    }else {
      page->WLatch();
      if (node->IsSafe(op)) {
        UnlatchAndUnpin(op, transaction);
      }
    }
  }
  return reinterpret_cast<LeafPage *>(page->GetData());
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::UnlatchAndUnpin(enum OpType op,Transaction *transaction, bool flag) {
  if (transaction == nullptr) {
    return;
  }
  auto pages = transaction->GetPageSet();
  for (auto page : *pages) {
    page_id_t page_id = page->GetPageId();
    BPlusTreePage * node = reinterpret_cast<BPlusTreePage *> (page->GetData());
    // std::cout <<page_id << ' ' <<root_page_id_ << '\n';
    if (op == OpType::READ) {
      if (node->IsRootPage()) {
        UnLockRoot(op);
      }
      page->RUnlatch();
      buffer_pool_manager_->UnpinPage(page_id, false);
    } else {
      if (node->IsRootPage() && (flag || node->IsSafe(op))) {
        UnLockRoot(op);
      }
      page->WUnlatch();
      buffer_pool_manager_->UnpinPage(page_id, true);
    }
  }
  transaction->GetPageSet()->clear();
  for (const auto &page_id: *transaction->GetDeletedPageSet()) {
    buffer_pool_manager_->DeletePage(page_id);
  }
  transaction->GetDeletedPageSet()->clear();
}
/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key, Transaction *transaction) {
  LockRoot(OpType::DELETE);
  if (IsEmpty()) {
    UnLockRoot(OpType::DELETE);
    return;
  }
  auto leaf_page = FindLeafPage(key, false, OpType::DELETE, transaction);
  auto cur_size = leaf_page->RemoveAndDeleteRecord(key, comparator_);
  if (cur_size < leaf_page->GetMinSize()) {
    CoalesceOrRedistribute(leaf_page, transaction);
  }
  UnlatchAndUnpin(OpType::DELETE, transaction, 1);
  assert(Check());
}
/*
 * User needs to first find the sibling of input page. If sibling's size + input
 * page's size > page's max size, then redistribute. Otherwise, merge.
 * Using template N to represent either internal page or leaf page.
 * @return: true means target leaf page should be deleted, false means no
 * deletion happens
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
auto BPLUSTREE_TYPE::CoalesceOrRedistribute(N *node, Transaction *transaction) -> bool {
  // if (N is the root and N has only one remaining child)
  if (node->IsRootPage()) {
    bool need_delete = AdjustRoot(node);
    if (need_delete) {
      transaction->AddIntoDeletedPageSet(node->GetPageId());
    }
    return need_delete;
  }
  N *sibling_node;
  bool is_right_sibling = FindLeftSibling(node, sibling_node, transaction);
  auto parent = buffer_pool_manager_->FetchPage(node->GetParentPageId());
  auto parent_node = reinterpret_cast<InternalPage *>(parent);
  // if (entries in N and N2 can fit in a single node)
  if (node->GetSize() + sibling_node->GetSize() <= node->GetMaxSize()) {
    if (is_right_sibling) {
      std::swap(node, sibling_node);
    }
    int remove_index = parent_node->ValueIndex(node->GetPageId());
    bool remove_success = Coalesce(sibling_node, node, parent_node, remove_index, transaction);
    if (!remove_success) {
      buffer_pool_manager_->UnpinPage(parent_node->GetPageId(), true);
    }
    return true;
  }
  /* Redistribution: borrow an entry from N2 */
  int node_index = parent_node->ValueIndex(node->GetPageId());
  Redistribute(sibling_node, node, node_index);
  buffer_pool_manager_->UnpinPage(parent_node->GetPageId(), false);
  // Let N2 be the previous or next child of parent(N)
  return false;
}
// 返回左儿子 没有则右儿子
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
auto BPLUSTREE_TYPE::FindLeftSibling(N *node, N *&sibling, Transaction *transaction) -> bool {
  auto parent_page = buffer_pool_manager_->FetchPage(node->GetParentPageId());
  auto parent_node = reinterpret_cast<InternalPage *>(parent_page);
  int node_index = parent_node->ValueIndex(node->GetPageId());
  int sibling_index = node_index - 1;
  if (node_index == 0) {  // no left sibling
    sibling_index = node_index + 1;
  }
  auto sibling_page = buffer_pool_manager_->FetchPage(parent_node->ValueAt(sibling_index));
  sibling_page->RLatch();
  if (transaction != nullptr) {
    transaction->AddIntoPageSet(sibling_page);
  }
  sibling = reinterpret_cast<N *>(sibling_page);
  buffer_pool_manager_->UnpinPage(parent_node->GetPageId(), false);
  return (node_index == 0);  // index == 0 means sibling is right
}
/*
 * Update root page if necessary
 * NOTE: size of root page can be less than min size and this method is only
 * called within coalesceOrRedistribute() method
 * case 1: when you delete the last element in root page, but root page still
 * has one last child
 * case 2: when you delete the last element in whole b+ tree
 * @return : true means root page should be deleted, false means no deletion
 * happend
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::AdjustRoot(BPlusTreePage *old_root_node) -> bool {
  if (old_root_node->IsLeafPage() && old_root_node->GetSize() == 0) {  // case 2 此时删除后树变成空的
    assert(old_root_node->GetSize() == 0);
    assert(old_root_node->GetParentPageId() == INVALID_PAGE_ID);
    // buffer_pool_manager_->UnpinPage(old_root_node->GetPageId(), false);
    // buffer_pool_manager_->DeletePage(old_root_node->GetPageId());
    root_page_id_ = INVALID_PAGE_ID;
    UpdateRootPageId();
    return true;
  }
  if (!old_root_node->IsLeafPage() && old_root_node->GetSize() == 1) {  // case 1
    auto old_root_page = reinterpret_cast<InternalPage *>(old_root_node);
    page_id_t new_root_page_id = old_root_page->RemoveAndReturnOnlyChild();
    root_page_id_ = new_root_page_id;
    UpdateRootPageId(0);
    Page *new_root_page = buffer_pool_manager_->FetchPage(new_root_page_id);
    auto new_root = reinterpret_cast<BPlusTreePage *>(new_root_page->GetData());
    new_root->SetParentPageId(INVALID_PAGE_ID);
    buffer_pool_manager_->UnpinPage(new_root_page_id, true);
    // buffer_pool_manager_->UnpinPage(old_root_node->GetPageId(), false);
    // buffer_pool_manager_->DeletePage(old_root_node->GetPageId());
    return true;
  }
  return false;
}
/*
 * Move all the key & value pairs from one page to its sibling page, and notify
 * buffer pool manager to delete this page. Parent page must be adjusted to
 * take info of deletion into account. Remember to deal with coalesce or
 * redistribute recursively if necessary.
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 * @param   parent             parent page of input "node"
 * @return  true means parent node should be deleted, false means no deletion
 * happend
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
auto BPLUSTREE_TYPE::Coalesce(N *&neighbor_node, N *&node,
                              BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *&parent, int index,
                              Transaction *transaction) -> bool {
  // assumption neighbor_node is before node
  assert(node->GetSize() + neighbor_node->GetSize() <= node->GetMaxSize());
  node->MoveAllTo(neighbor_node, index, buffer_pool_manager_);
  transaction->AddIntoDeletedPageSet(node->GetPageId());
  // auto page_id = node->GetPageId();
  // buffer_pool_manager_->UnpinPage(page_id, false);
  // buffer_pool_manager_->DeletePage(page_id);
  // buffer_pool_manager_->UnpinPage(neighbor_node->GetPageId(), true);
  parent->Remove(index);
  if (parent->GetSize() < parent->GetMinSize()) {
    return CoalesceOrRedistribute(parent, transaction);
  }
  return false;
}
/*
 * Redistribute key & value pairs from one page to its sibling page. If index ==
 * 0, move sibling page's first key & value pair into end of input "node",
 * otherwise move sibling page's last key & value pair into head of input
 * "node".
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
void BPLUSTREE_TYPE::Redistribute(N *neighbor_node, N *node, int index) {
  if (index == 0) {
    // node <= neighbor_node
    neighbor_node->MoveFirstToEndOf(node, buffer_pool_manager_);
    auto page = buffer_pool_manager_->FetchPage(neighbor_node->GetParentPageId());
    auto parent_node = reinterpret_cast<InternalPage *>(page->GetData());
    parent_node->SetKeyAt(parent_node->ValueIndex(neighbor_node->GetPageId()), neighbor_node->KeyAt(0));
    buffer_pool_manager_->UnpinPage(neighbor_node->GetParentPageId(), true);
  } else {
    // neighbor_node <= node
    neighbor_node->MoveLastToFrontOf(node, index, buffer_pool_manager_);
    auto page = buffer_pool_manager_->FetchPage(neighbor_node->GetParentPageId());
    auto parent_node = reinterpret_cast<InternalPage *>(page->GetData());
    parent_node->SetKeyAt(index, node->KeyAt(0));
    buffer_pool_manager_->UnpinPage(neighbor_node->GetParentPageId(), true);
  }
}
/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leaftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE {
  LockRoot(OpType::READ);
  auto start_leaf = FindLeafPage(KeyType(), true);
  UnLockRoot(OpType::READ);
  return INDEXITERATOR_TYPE(start_leaf, 0, buffer_pool_manager_);
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  LockRoot(OpType::READ);
  auto start_leaf = FindLeafPage(key);
  UnLockRoot(OpType::READ);
  if (start_leaf == nullptr) {
    return INDEXITERATOR_TYPE(start_leaf, 0, buffer_pool_manager_);
  }
  int start_index = start_leaf->KeyIndex(key, comparator_);
  return INDEXITERATOR_TYPE(start_leaf, start_index, buffer_pool_manager_);
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE {
  return INDEXITERATOR_TYPE(nullptr, 0, buffer_pool_manager_);
}
/*
 * Input parameter is void, find the leaftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::begin() -> INDEXITERATOR_TYPE {
  return Begin();
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  return Begin(key);
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::end() -> INDEXITERATOR_TYPE {
  return INDEXITERATOR_TYPE(nullptr, 0, buffer_pool_manager_);
}
/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { return root_page_id_; }

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/
/*
 * Update/Insert root page id in header page(where page_id = 0, header_page is
 * defined under include/page/header_page.h)
 * Call this method everytime root page id is changed.
 * @parameter: insert_record      defualt value is false. When set to true,
 * insert a record <index_name, root_page_id> into header page instead of
 * updating it.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::UpdateRootPageId(int insert_record) {
  auto *header_page = static_cast<HeaderPage *>(buffer_pool_manager_->FetchPage(HEADER_PAGE_ID));
  if (insert_record != 0) {
    // create a new record<index_name + root_page_id> in header_page
    header_page->InsertRecord(index_name_, root_page_id_);
  } else {
    // update root_page_id in header_page
    header_page->UpdateRecord(index_name_, root_page_id_);
  }
  buffer_pool_manager_->UnpinPage(HEADER_PAGE_ID, true);
}

/*
 * This method is used for test only
 * Read data from file and insert one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;

    KeyType index_key;
    index_key.SetFromInteger(key);
    RID rid(key);
    Insert(index_key, rid, transaction);
  }
}
/*
 * This method is used for test only
 * Read data from file and remove one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;
    KeyType index_key;
    index_key.SetFromInteger(key);
    Remove(index_key, transaction);
  }
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Draw(BufferPoolManager *bpm, const std::string &outf) {
  if (IsEmpty()) {
    LOG_WARN("Draw an empty tree");
    return;
  }
  std::ofstream out(outf);
  out << "digraph G {" << std::endl;
  ToGraph(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm, out);
  out << "}" << std::endl;
  out.flush();
  out.close();
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Print(BufferPoolManager *bpm) {
  if (IsEmpty()) {
    LOG_WARN("Print an empty tree");
    return;
  }
  ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm);
}

/**
 * This method is used for debug only, You don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 * @param out
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToGraph(BPlusTreePage *page, BufferPoolManager *bpm, std::ofstream &out) const {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    // Print node name
    out << leaf_prefix << leaf->GetPageId();
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << leaf->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << ",size=" << leaf->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      out << "<TD>" << leaf->KeyAt(i) << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << leaf->GetPageId() << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << leaf->GetPageId() << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }

    // Print parent links if there is a parent
    if (leaf->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << leaf->GetParentPageId() << ":p" << leaf->GetPageId() << " -> " << leaf_prefix
          << leaf->GetPageId() << ";\n";
    }
  } else {
    auto *inner = reinterpret_cast<InternalPage *>(page);
    // Print node name
    out << internal_prefix << inner->GetPageId();
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << inner->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << ",size=" << inner->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        out << inner->KeyAt(i);
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Parent link
    if (inner->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << inner->GetParentPageId() << ":p" << inner->GetPageId() << " -> " << internal_prefix
          << inner->GetPageId() << ";\n";
    }
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i))->GetData());
      ToGraph(child_page, bpm, out);
      if (i > 0) {
        auto sibling_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i - 1))->GetData());
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_page->GetPageId() << " " << internal_prefix
              << child_page->GetPageId() << "};\n";
        }
        bpm->UnpinPage(sibling_page->GetPageId(), false);
      }
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

/**
 * This function is for debug only, you don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToString(BPlusTreePage *page, BufferPoolManager *bpm) const {
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    std::cout << "Leaf Page: " << leaf->GetPageId() << " parent: " << leaf->GetParentPageId()
              << " next: " << leaf->GetNextPageId() << std::endl;
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  } else {
    auto *internal = reinterpret_cast<InternalPage *>(page);
    std::cout << "Internal Page: " << internal->GetPageId() << " parent: " << internal->GetParentPageId() << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(internal->ValueAt(i))->GetData()), bpm);
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}
/***************************************************************************
 *  Check integrity of B+ tree data structure.
 ***************************************************************************/
/**
 * This function is for check B+ tree is balanced ?
 * return : the height of B+ tree ( >=0 ) 
 * the children's height of node must be same, which means balanced  
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsBalanced(page_id_t pid) -> int {
  if (IsEmpty()) {
    return 1;
  }
  auto node = reinterpret_cast<BPlusTreePage *>(buffer_pool_manager_->FetchPage(pid));
  if (node == nullptr){
    throw Exception("all page are pinned while isBalanced");
  }
  // height of leaf = 0 
  if (node->IsLeafPage()) {
    buffer_pool_manager_->UnpinPage(pid, false);
    return 0;
  }
  auto page = reinterpret_cast<InternalPage *>(node);
  int cur_height = 0;
  int last_child_height = -2;
  for (int i = 0; i < page->GetSize(); i++) {
    int child_height = IsBalanced(page->ValueAt(i));
    if (child_height >= 0 && last_child_height == -2) {
      last_child_height = child_height;
      cur_height = child_height + 1;
    }
    else if(last_child_height != child_height) {
      cur_height = -1; // 不平衡
      break;
    }
  }
  buffer_pool_manager_->UnpinPage(pid, false);
  return cur_height;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Check() -> bool {
  bool passed = 1;
  bool is_balanced = (IsBalanced(root_page_id_) >= 0);
  passed &= is_balanced;
  if (!is_balanced) {
    std::cout << "problem in balance!\n";
  }
  return passed;
}


template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
