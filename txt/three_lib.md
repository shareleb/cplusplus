
gperftools:
    gperftools 是一个google性能分析工具。如高性能的内存分配器（TCMalloc）、CPU性能分析器（CPU Profiler）、堆性能分析器（Heap Profiler）和堆检查器（Heap Checker）。 
    编译时会需要安装autoconf automake  libtool 然后执行.autogen.sh, 然后会生成一个configure。

    第一种方式： 源码安装
    1. 下载tar包，解压
    2.   ./configure --prefix=/home/leb/project/cppsdk/install/ && make && make install && cd ..
        选项指定了安装目录。这意味着软件将被安装到/home/leb/project/cppsdk/install/目录下.
    如果有需要还可以 指定自定义的库路径和头文件路径，可以通过设置LDFLAGS和CPPFLAGS环境变量来实现
        LDFLAGS=-L/home/leb/project/cppsdk/install/lib/ \
        CPPFLAGS=-I/home/leb/project/cppsdk/install/include \
    
 代码中使用时链接-ltcmalloc      
    https://blog.csdn.net/bandaoyu/article/details/108630996?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522171573967716800188526949%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=171573967716800188526949&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-2-108630996-null-null.142^v100^pc_search_result_base4&utm_term=%E5%A6%82%E4%BD%95%E5%AE%89%E8%A3%85tcmalloc&spm=1018.2226.3001.4187



faiss 向量检索库
    facebook 开源库，用于高效向量相似性搜索库。适用于大规模向量数据集存储与检索
    多种索引结构 FLat IVF HNSW PQ LSH 
    多语言 Python  C++
    