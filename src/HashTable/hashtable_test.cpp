#include "iostream"
#include "hashtable.h"

#include <iostream>
#include "hashtable.h"

int main() {
    leb::HashTable hashTable(1024);

    // 插入键值对
    hashTable.Insert(1, "One");
    hashTable.Insert(2, "Two");
    hashTable.Insert(3, "Three");
    hashTable.Insert(1026, "1024");

    // 搜索键值对
    std::string value;
    if (hashTable.Search(2, value)) {
        std::cout << "Value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found" << std::endl;
    }

    // 删除键值对
    if (hashTable.Delete(2)) {
        std::cout << "Key 2 deleted" << std::endl;
    } else {
        std::cout << "Key 2 not found" << std::endl;
    }

    // 再次搜索键值对
    if (hashTable.Search(2, value)) {
        std::cout << "Value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found" << std::endl;
    }


    std::string value1;
    if (hashTable.Search(1026, value1)) {
        std::cout << "Value for key 1026: " << value1 << std::endl;
    } else {
        std::cout << "Key 1026 not found" << std::endl;
    }
    return 0;
}
