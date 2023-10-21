/**
 * @file hashtable.h
 * @author your name (you@domain.com)
 * @brief 线性探测法
 * @version 0.1
 * @date 2023-10-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SRC_HASHTABLE_H_
#define SRC_HASHTABLE_H_
#include <string>
#include <vector>

namespace leb {

struct HashNode {
  HashNode() : key_(0), is_use_(false) {}
  int key_;
  std::string val_;
  bool is_use_;
};

class HashTable {
public:
  HashTable(int size) : table_(size, HashNode()), use_num_(0) {}
  bool Insert(int key, const std::string &val) {
    if (use_num_ == table_.size()) {
      return false;
    }
    int k = HashFunc(key);
    while (table_[k].is_use_) {
      if (table_[k].key_ == key) {
        return false;
      }
      k = (k + 1) % table_.size();
    }
    table_[k].is_use_ = true;
    table_[k].val_ = val;
    table_[k].key_ = key;
    use_num_++;
    return true;
  }

  bool Search(int key, std::string &val) {
    int k = HashFunc(key);
    int count = 0;
    // 避免无限探测
    while (table_[k].is_use_ && count < table_.size()) {
      if (table_[k].key_ == key) {
        val = table_[k].val_;
        return true;
      }
      k = (k + 1) % table_.size();
      count++;
    }
    // key 不存在返回false;
    return false;
  }

  bool Delete(int key) {
    int k = HashFunc(key);
    int count = 0;
    while (table_[k].is_use_ && count < table_.size()) {
      if (table_[k].key_ == key) {
        table_[k].is_use_ = false;
        table_[k].val_.clear();
        table_[k].key_ = 0;
        use_num_--;
        // 处理后续位置上的节点
        int next = (k + 1) % table_.size();
        while (table_[next].is_use_) {
          int rehash = table_[next].key_ % table_.size();
          if (rehash != next) {
            // 将后续位置上的节点重新插入哈希表
            Insert(table_[next].key_, table_[next].val_);
            table_[next].is_use_ = false;
            table_[next].val_.clear();
            table_[next].key_ = 0;
            use_num_--;
          }
          next = (next+1) % table_.size();
        }
        return true;
      } else {
        k = (k + 1) % table_.size();
        count++;
      }
    }
    return false;
  }

private:
  int HashFunc(int key) { return key % table_.size(); }

  std::vector<HashNode> table_;
  int use_num_;
};

} // namespace leb

#endif