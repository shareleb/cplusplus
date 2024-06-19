Faiss 开源的向量检索库      Faiss源码解析
主要有IndexFlat IndexIVFFlat  IndexIVFPQ IndexHMSW 
        暴力       倒排           量化     图检索



这几个都继承至  Index


Index接口类：


  成员：
    D 维度
	ntotal 向量节点总数
	is_trained 是否训练完成
	metric_type 检索类型
  接口：
    train(idx_t n, const float* x); 对一组向量集合进行训练。
    add(idx_t n, const float* x) 增加向量节点到索引里。
    add_with_ids(idx_t n, const float* x, const idx_t* xids); 类似add， xids也被加入进去。
    search(idx_t n, const float* x, idx_t k, float* distances, idx_t* labels, const SearchParameters* params ) 检索
    range_search( idx_t n, const float* x, float radius, RangeSearchResult* result, const SearchParameters* params); 范围检索
    assign(idx_t n, const float* x, idx_t* labels, idx_t k = 1) 类似search,但是只返回临近标签索引
    reset() 清空数据。
    size_t remove_ids(const IDSelector& sel); 删除部分数据。
    reconstruct(idx_t key, float* recons) const; 重构近似向量，用于恢复原始数据
    void search_and_reconstruct(idx_t n， const float* x, idx_t k, float* distances, idx_t* labels, float* recons）

    virtual void compute_residual(const float* x, float* residual, idx_t key)；
    

       virtual DistanceComputer* get_distance_computer() const; 计算向量距离选择器
       virtual size_t sa_code_size() const; 
       virtual void sa_encode(idx_t n, const float* x, uint8_t* bytes) const; 编码
       virtual void sa_decode(idx_t n, const uint8_t* bytes, float* x) const; 解码

     virtual void merge_from(Index& otherIndex, idx_t add_id = 0); 把一个索引合并到另个索引里
    virtual void check_compatible_for_merge(const Index& otherIndex) const; 校验是否能合并



大规模数据处理和存储中，为了节省存储空间和提高处理速度，常常使用有损压缩技术对数据进行编码， 重构其近似向量： 可以在一定程度上恢复原始数据。
计算残差向量：残差向量的每个分量表示原始向量在该维度上的值与重构向量在该维度上的值之间的差异  简单理解就是各个维度相减。提供了近似误差的信息





Level1Quantizer：
  成员：
     Index* quantizer  索引指针
     nlist : 倒排桶个数
     quantizer_trains_alone 量化器训练模式
         0：使用量化器作为kmeans训练的索引。
         1：直接将训练集传递给量化器的train()方法进行训练。
         2：在一个平面索引上进行kmeans训练，然后将质心添加到量化器中。
     ClusteringParameters cp 聚类参数，用于替代默认的聚类方法。
  接口：
    train_q1(
            size_t n,
            const float* x,
            bool verbose,
            MetricType metric_type); 调用这个Clustering 类来训练找质心
        训练寻找质心。

    size_t coarse_code_size()  计算 向量被分配到的聚类中心（或称为质心）的索引  所占空间大小。
     void encode_listno(idx_t list_no, uint8_t* code) const; 其目的是将列表编号（list_no）编码到一个字节数组（code）中，使用的是小端字节序（little endian）方式。
    idx_t decode_listno(const uint8_t* code) const;


struct IndexIVFInterface : Level1Quantizer  继承Level1Quantizer
    nprobe 通常指的是在查询时要考虑的候选列表或桶的数量。
    size_t max_codes = 0;  查询时最大查询数量，求交截断






粗量化编码（Coarse Quantization Coding）是一种用于大规模向量搜索和分类的技术，它通过将高维数据向量映射到较低维的离散空间中的一个有限集合（即聚类中心或质心）来减少数据的存储和计算需求。


Clustering：
 kmeans 算法找质心。
 成员变量：
     int niter = 25;  找质心迭代次数
     int nredo = 1; 找几次质心

     d 维度
     k 质心数量。
    std::vector<float> centroids; 质心
  接口：    
    train_encoded 找质心


struct IndexIVF : Index, IndexIVFInterface 

    ArrayInvertedLists  invlists 倒排链表



Quantizer ：
    d 维度
    code_size  向量大小
ProductQuantizer ： Quantizer 量化
    size_t M;     ///< number of subquantizers
    size_t nbits; ///< number of bits per quantization index
    size_t dsub;  ///< dimensionality of each subvector
    size_t ksub;  ///< number of centroids for each subquantizer

IndexIVFPQ : IndexIVF
