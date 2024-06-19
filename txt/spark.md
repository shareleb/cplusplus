spark下载：  https://spark.apache.org/downloads.html 下载解压， 将“${解压目录}/bin”配置到PATH环境变量。

spark-shell --version 印出Spark版本号  提供交互式的运行环境. spark-shell在运行的时候，依赖于Java和Scala语言环境


SparkSession成为统一的开发入口
 RDD  弹性分布式数据集 常用算子： map、filter、flatMap和reduceByKey


RDD的flatMap操作在逻辑上可以分成两个步骤：映射和展平。


RDD是构建Spark分布式内存计算引擎

RDD是一种抽象，是Spark对于分布式数据集的抽象，它用于囊括所有内存中和磁盘中的分布式数据实体。
4大特性：
    partitions：数据分片
    partitioner：分片切割规则
    dependencies：RDD依赖
    compute：转换函数

基于不同数据形态之间的转换，构建计算流图（DAG，Directed Acyclic Graph）；
通过Actions类算子，以回溯的方式去触发执行这个计算流图。

RDD到RDD之间的转换，本质上是数据形态上的转换（Transformations）。
开发者需要使用Transformations类算子，定义并描述数据形态的转换过程，然后调用Actions类算子，将计算结果收集起来、或是物化到磁盘。

当且仅当开发者调用Actions算子时，之前调用的转换算子才会付诸执行。在业内，这样的计算模式有个专门的术语，叫作“延迟计算”（Lazy Evaluation）。



创建RDD：
在Spark中，创建RDD的典型方式有两种：
通过SparkContext.parallelize在内部数据之上创建RDD；
通过SparkContext.textFile等API从外部数据创建RDD。


分布式计算的实现，离不开两个关键要素，一个是进程模型，另一个是分布式的环境部署。

