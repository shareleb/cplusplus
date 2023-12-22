#ifndef MTINDEX_COMMON_HASHTABLE_H_
#define MTINDEX_COMMON_HASHTABLE_H_

#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define MAX_MMAP_FILENAME_LEN      (512)

#pragma pack(push)
#pragma pack(1)

// hash table head structure
struct  SHashTableHead {
    int m_dwBucketSize;        // number of hashtable size
    int m_dwTotalNodeNum;  // number of total hash node
    int m_dwFreeNodeIdx;     // index of free node
    int m_dwFreeNodeNum;   // number of free node
};
#pragma pack(pop)

// This template should be packed to reduce index size, however, in gcc 4.4.7 SHashNode is not packed
// by default, and in gcc 4.9.2 it is packed correctly. So, to be compatible with old index data,
// we remove the pack(1) attribute.
template<typename TKEY, typename TNODE>
struct SHashNode {
    TKEY        m_tKey;
    TNODE     m_stNode;
    int m_dwNext;
};

template<typename TKEY, typename TNODE>
class HashTable {
public:
    class Iterator {
    public:
        // return value: 0 => ok
        virtual int process(const TKEY& key, TNODE& value) = 0;  // NOLINT
    };

public:
    HashTable() {
        m_pstHashTableHead = NULL;
        m_pHashTableAddr = NULL;
        m_pdwBucketAddr = NULL;
        m_pHashNodePoolAddr = NULL;
        m_pFreeListHead = NULL;
        m_fd = -1;
    }

    ~HashTable() {
        if (m_fd != -1) {
            close(m_fd);
        }
    }

public:
    /*------- interface 1 : supply for application process to call --------*/
    int Init(int dwTotalHashNodeNum, bool need_map, const char* pMmapFileName = NULL);

    int UnInit();

    int Insert(TKEY tKey, TNODE* pNode);

    int Find(TKEY tKey, TNODE*& stNode, uint32_t udwSeqNo = 0) const;  // NOLINT

    int Find(TKEY tKey, TNODE& stNode, uint32_t udwSeqNo = 0) const;  // NOLINT

    int Delete(TKEY tKey);

    int Reset();

    int Foreach(Iterator& o);  //  NOLINT quit when o->process() retuens non-zero value

    void Clean(Iterator& o);  //  NOLINT free node(s) when o->process() returns 0


    /*-------- interface 2 : to get hash table base infomation ------*/

    int GetTotalNodeNum() const {
        return m_pstHashTableHead->m_dwTotalNodeNum;
    }

    int GetFreeNodeNum() const {
        return m_pstHashTableHead->m_dwFreeNodeNum;
    }

    int GetUsedNodeNum() const {
        return m_pstHashTableHead->m_dwTotalNodeNum - m_pstHashTableHead->m_dwFreeNodeNum;
    }

    int GetBucketNum() const {
        return m_pstHashTableHead->m_dwBucketSize;
    }

    char* GetHashTableAddr() const {
        return m_pHashTableAddr;
    }

    char* GetMmapFileName() const {
        return m_szMmapFileName;
    }

    void FreeMemory() {
        if (NULL != m_pHashTableAddr) {
            delete []m_pHashTableAddr;
            m_pHashTableAddr = NULL;
        }
    }
    int ResetHashTable();

    uint64_t GetTotalMemSize() const {
        return m_uddwMemSize;
    }

    int GetTotalHashNodeNum() const {
        return m_dwTotalHashNodeNum;
    }

private:
    int MmapFile();


    int Attach2File();

    bool CheckFileSize();

    SHashNode<TKEY, TNODE>* GetNode();

    void FreeNode(SHashNode<TKEY, TNODE>* pFreeNode);

    int GetPos(SHashNode<TKEY, TNODE>* pNode) {
        return pNode - m_pHashNodePoolAddr;
    }

private:
    // memory size
    uint64_t   m_uddwMemSize;
    // bucket num
    int m_dwBucketNum;
    // total hash node num
    int m_dwTotalHashNodeNum;
    // fild handle
    int m_fd;
    // mmap file name
    char    m_szMmapFileName[MAX_MMAP_FILENAME_LEN];

    // hash table beginning address
    char*     m_pHashTableAddr;

    // hash table head
    SHashTableHead*   m_pstHashTableHead;

    // hash table bucket addree
    int*  m_pdwBucketAddr;

    // hash node beginning address
    SHashNode<TKEY, TNODE>*    m_pHashNodePoolAddr;

    // hash node free list head
    SHashNode<TKEY, TNODE>*  m_pFreeListHead;
};

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::MmapFile() {
    m_fd = open(m_szMmapFileName, O_RDWR | O_CREAT, S_IRWXU/*0700*/);
    if (m_fd == -1) {
        return -1;
    }
    // seek to the memszie
    if (lseek(m_fd, m_uddwMemSize - 1, SEEK_SET) == -1) {
        return -2;
    }
    if (write(m_fd, " ", 1) != 1) {
        return -3;
    }
    // system-call func mmap
    void* pMmapAddr = mmap(NULL, m_uddwMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, (off_t)0);
    if (pMmapAddr == MAP_FAILED) {
        return -4;
    }
    // hash table addr
    m_pHashTableAddr = reinterpret_cast<char*>(pMmapAddr);
    return 0;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::ResetHashTable() {
    // hash table head
    m_pstHashTableHead = reinterpret_cast<SHashTableHead*>(m_pHashTableAddr);
    // hash bucket addr
    m_pdwBucketAddr = reinterpret_cast<int*>(m_pHashTableAddr + sizeof(SHashTableHead));
    // hash node pool addr
    m_pHashNodePoolAddr = (SHashNode<TKEY, TNODE>*)(m_pHashTableAddr + sizeof(SHashTableHead) + sizeof(int) * m_dwBucketNum);  // NOLINT

    // init hashtable head info
    m_pstHashTableHead->m_dwBucketSize = m_dwBucketNum;
    m_pstHashTableHead->m_dwTotalNodeNum = m_dwTotalHashNodeNum;
    m_pstHashTableHead->m_dwFreeNodeNum = m_dwTotalHashNodeNum;
    m_pstHashTableHead->m_dwFreeNodeIdx = 0;

    // init hash bucket
    int dwIndex = 0;
    for (dwIndex = 0; dwIndex < m_dwBucketNum; dwIndex++) {
        m_pdwBucketAddr[dwIndex] = -1;
    }

    // init hash node pool
    for (dwIndex = 0; dwIndex < m_dwTotalHashNodeNum - 1; dwIndex++) {
        m_pHashNodePoolAddr[dwIndex].m_dwNext = dwIndex + 1;
    }
    m_pHashNodePoolAddr[dwIndex].m_dwNext = -1;

    m_pFreeListHead = m_pHashNodePoolAddr + m_pstHashTableHead->m_dwFreeNodeIdx;

    return 0;
}



template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Attach2File() {
    // hash table head
    m_pstHashTableHead = reinterpret_cast<SHashTableHead*>(m_pHashTableAddr);
    // hash bucket addr
    m_pdwBucketAddr = reinterpret_cast<int*>(m_pHashTableAddr + sizeof(SHashTableHead));
    // hash node pool addr
    m_pHashNodePoolAddr = (SHashNode<TKEY, TNODE>*)(m_pHashTableAddr + sizeof(SHashTableHead) + sizeof(int) * m_dwBucketNum);  // NOLINT
    // hash node pool free list head
    m_pFreeListHead = m_pHashNodePoolAddr + m_pstHashTableHead->m_dwFreeNodeIdx;
    return 0;
}

template<typename TKEY, typename TNODE>
bool  HashTable<TKEY, TNODE>::CheckFileSize() {
    struct stat statbuf;
    if (::stat(m_szMmapFileName, &statbuf) != 0) {
        return false;
    }
    return (((uint64_t)statbuf.st_size) == m_uddwMemSize);
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Init(int dwTotalHashNodeNum, bool need_map, const char* pMmapFileName) {
    if (dwTotalHashNodeNum < 0) {
        return -1;
    }
    if (pMmapFileName == NULL) {
        snprintf(m_szMmapFileName, MAX_MMAP_FILENAME_LEN, "%s", "default_hash.dat");
    } else {
        snprintf(m_szMmapFileName, MAX_MMAP_FILENAME_LEN, "%s", pMmapFileName);
    }
    m_dwTotalHashNodeNum = dwTotalHashNodeNum;
    if (m_dwTotalHashNodeNum >= 10000) {
        m_dwBucketNum = m_dwTotalHashNodeNum;
    } else {
        m_dwBucketNum = m_dwTotalHashNodeNum;
    }

    // get the min prime number that bigger than  m_dwBucketNum
    int dwIdx = 0;
    while (true) {
        for (dwIdx = 2; dwIdx < sqrt(static_cast<double>(m_dwBucketNum)); dwIdx++) {
            if (m_dwBucketNum % dwIdx == 0) {
                break;
            }
        }
        if (dwIdx < sqrt(m_dwBucketNum)) {
            m_dwBucketNum++;
            continue;
        } else {
            break;
        }
    }

    // set memsize suiteble for system pagesize
    int pagesize = sysconf(_SC_PAGESIZE);
    m_uddwMemSize = sizeof(SHashTableHead) + sizeof(int) * m_dwBucketNum + sizeof(SHashNode<TKEY, TNODE>) * m_dwTotalHashNodeNum;  // NOLINT
    if (m_uddwMemSize % pagesize != 0) {
        m_uddwMemSize = (m_uddwMemSize / pagesize + 1) * pagesize;
    }

    if (access(m_szMmapFileName, F_OK) != 0) {   /* file does not exist */
        if (need_map) {
            return -5;
        }
        m_pHashTableAddr = new char[m_uddwMemSize];
        memset(m_pHashTableAddr, 0, m_uddwMemSize);

        ResetHashTable();
    } else { /* file does exist */
        if (!CheckFileSize()) {
            return -3;
        }

        m_pHashTableAddr = new char[m_uddwMemSize];
        memset(m_pHashTableAddr, 0, m_uddwMemSize);

        FILE* fp = fopen(m_szMmapFileName, "rb");
        if (fp == NULL) {
            return -4;
        }
        if (fread(m_pHashTableAddr, m_uddwMemSize, 1, fp) != 1) {
            return -5;
        }
        fclose(fp);

        Attach2File();
    }

    return 0;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Reset() {
    // init hashtable head info
    m_pstHashTableHead->m_dwFreeNodeNum = m_dwTotalHashNodeNum;
    m_pstHashTableHead->m_dwFreeNodeIdx = 0;

    // init hash bucket
    int dwIndex = 0;
    for (dwIndex = 0; dwIndex < m_dwBucketNum; dwIndex++) {
        m_pdwBucketAddr[dwIndex] = -1;
    }

    // init hash node pool
    for (dwIndex = 0; dwIndex < m_dwTotalHashNodeNum - 1; dwIndex++) {
        m_pHashNodePoolAddr[dwIndex].m_dwNext = dwIndex + 1;
    }
    m_pHashNodePoolAddr[dwIndex].m_dwNext = -1;

    m_pFreeListHead = m_pHashNodePoolAddr + m_pstHashTableHead->m_dwFreeNodeIdx;

    return 0;
}



template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::UnInit() {
    FILE* fp = fopen(m_szMmapFileName, "w+");
    if (fp == NULL) {
        return -1;
    }
    if (fwrite(m_pHashTableAddr, m_uddwMemSize, 1, fp) != 1) {
        return -2;
    }
    fclose(fp);
    return 0;
}



template<typename TKEY, typename TNODE>
void HashTable<TKEY, TNODE>::FreeNode(SHashNode<TKEY, TNODE>* pFreeNode) {
    int dwFreeNodeIdx = pFreeNode - m_pHashNodePoolAddr;
    pFreeNode->m_dwNext = m_pstHashTableHead->m_dwFreeNodeIdx;
    m_pFreeListHead = pFreeNode;
    m_pstHashTableHead->m_dwFreeNodeIdx = dwFreeNodeIdx;
    m_pstHashTableHead->m_dwFreeNodeNum++;
}

template<typename TKEY, typename TNODE>
SHashNode<TKEY, TNODE>* HashTable<TKEY, TNODE>::GetNode() {
    if (m_pstHashTableHead->m_dwFreeNodeNum == 0) {
        return NULL;
    }
    SHashNode<TKEY, TNODE>* pAllocNode = m_pFreeListHead;
    m_pstHashTableHead->m_dwFreeNodeIdx = m_pFreeListHead->m_dwNext;
    m_pFreeListHead = m_pHashNodePoolAddr + m_pstHashTableHead->m_dwFreeNodeIdx;
    m_pstHashTableHead->m_dwFreeNodeNum--;
    return pAllocNode;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Insert(TKEY tKey, TNODE* pNode) {
    int dwBuckNo = (uint64_t)tKey % m_pstHashTableHead->m_dwBucketSize;
    int* pBucket =  m_pdwBucketAddr + dwBuckNo;
    int pCur = *pBucket;
    int pLast = -1;
    int pInsertPos = -1;

    SHashNode<TKEY, TNODE>*  pInsertNode = GetNode();
    if (pInsertNode == NULL) {
        return -1;
    }
    pInsertPos = GetPos(pInsertNode);
    pInsertNode->m_tKey = tKey;
    pInsertNode->m_stNode = *pNode;

    if (pCur == -1) {
        *pBucket = pInsertPos;
        pInsertNode->m_dwNext = -1;
        return 0;
    } else {
        while (pCur != -1 && m_pHashNodePoolAddr[pCur].m_tKey < tKey) {
            pLast = pCur;
            pCur = m_pHashNodePoolAddr[pCur].m_dwNext;
        }

        if (pCur != -1  && m_pHashNodePoolAddr[pCur].m_tKey == tKey) {
            m_pHashNodePoolAddr[pCur].m_stNode = *pNode;    // update the found node
            FreeNode(pInsertNode);
            return 1;
        }

        if (pLast == -1) {    /* insert head*/
            *pBucket = pInsertPos;
            pInsertNode->m_dwNext = pCur;
        } else if (pLast != -1 && pCur == -1) {  /* insert tail*/
            m_pHashNodePoolAddr[pLast].m_dwNext = pInsertPos;
            pInsertNode->m_dwNext = -1;
        } else { /* insert middle*/
            m_pHashNodePoolAddr[pLast].m_dwNext = pInsertPos;
            pInsertNode->m_dwNext = pCur;
        }

        return 0;
    }
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Find(TKEY tKey, TNODE*& pstNode, uint32_t udwSeqNo) const {  // NOLINT
    int dwBuckNo = (uint64_t)tKey % m_pstHashTableHead->m_dwBucketSize;
    int* pBucket =  m_pdwBucketAddr + dwBuckNo;
    int pCur = *pBucket;

    while (pCur != -1 && m_pHashNodePoolAddr[pCur].m_tKey < tKey) {
        pCur = m_pHashNodePoolAddr[pCur].m_dwNext;
    }

    if (pCur == -1 || m_pHashNodePoolAddr[pCur].m_tKey > tKey) {
        return -1;
    } else if (m_pHashNodePoolAddr[pCur].m_tKey == tKey) {
        pstNode = &m_pHashNodePoolAddr[pCur].m_stNode;
        return 0;
    }

    return -1;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Find(TKEY tKey, TNODE& stNode, uint32_t udwSeqNo) const {  // NOLINT
    int dwBuckNo = (uint64_t)tKey % m_pstHashTableHead->m_dwBucketSize;
    int* pBucket =  m_pdwBucketAddr + dwBuckNo;
    int pCur = *pBucket;

    while (pCur != -1 && m_pHashNodePoolAddr[pCur].m_tKey < tKey) {
        pCur = m_pHashNodePoolAddr[pCur].m_dwNext;
    }

    if (pCur == -1 || m_pHashNodePoolAddr[pCur].m_tKey > tKey) {
        return -1;
    } else if (m_pHashNodePoolAddr[pCur].m_tKey == tKey) {
        stNode = m_pHashNodePoolAddr[pCur].m_stNode;
        return 0;
    }

    return -1;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Delete(TKEY tKey) {
    int dwBuckNo = (uint64_t)tKey % m_pstHashTableHead->m_dwBucketSize;
    int* pBucket =  m_pdwBucketAddr + dwBuckNo;
    int pCur = *pBucket;
    int pLast = -1;

    if (*pBucket == -1) {
        return -1;
    }

    while (pCur != -1 && m_pHashNodePoolAddr[pCur].m_tKey < tKey) {
        pLast = pCur;
        pCur = m_pHashNodePoolAddr[pCur].m_dwNext;
    }

    if (pCur == -1 || m_pHashNodePoolAddr[pCur].m_tKey > tKey) {
        return -1;
    }

    if (m_pHashNodePoolAddr[pCur].m_tKey == tKey) {
        if (pLast == -1) {  /* delete head*/
            *pBucket = m_pHashNodePoolAddr[pCur].m_dwNext;
        } else if (pLast != -1 && pCur != -1) { /* not head*/
            m_pHashNodePoolAddr[pLast].m_dwNext = m_pHashNodePoolAddr[pCur].m_dwNext;
        }
        FreeNode(m_pHashNodePoolAddr + pCur);
        return 0;
    }

    return -1;
}

template<typename TKEY, typename TNODE>
int HashTable<TKEY, TNODE>::Foreach(Iterator& o) {  // NOLINT
    for (int i = 0; i < m_dwBucketNum; ++i) {
        int dwCur = m_pdwBucketAddr[i];

        while (dwCur != -1) {
            if (o.process(m_pHashNodePoolAddr[dwCur].m_tKey,
                          m_pHashNodePoolAddr[dwCur].m_stNode) != 0)
                return -1;

            dwCur = m_pHashNodePoolAddr[dwCur].m_dwNext;
        }
    }

    return 0;
}

template<typename TKEY, typename TNODE>
void HashTable<TKEY, TNODE>::Clean(Iterator& o) {  // NOLINT
    for (int i = 0; i < m_dwBucketNum; ++i) {
        int dwCur = m_pdwBucketAddr[i];
        int dwPrev = -1;

        while (dwCur != -1) {
            SHashNode<TKEY, TNODE>* pCurNode = &(m_pHashNodePoolAddr[dwCur]);

            if (o.process(pCurNode->m_tKey, pCurNode->m_stNode) == 0) {
                if (dwPrev == -1) {  //  first node is going to be deleted
                    m_pdwBucketAddr[i] = pCurNode->m_dwNext;
                } else {
                    m_pHashNodePoolAddr[dwPrev].m_dwNext = pCurNode->m_dwNext;
                }

                dwCur = pCurNode->m_dwNext;
                FreeNode(pCurNode);
            } else {
                dwPrev = dwCur;
                dwCur = pCurNode->m_dwNext;
            }
        }
    }
}

#endif  
