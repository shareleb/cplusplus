先建立全局的系统观，然后到具体技术点。

redis: c语言写的，基于内存的，key-value 数据库。
  应用场景： 缓存，分布式锁， 数据库， 消息队列， 秒杀
  高性能 ： 线程模型，数据结构， 网络框架
  高可靠 ： 主从复制，哨兵机制， 持久化
  高可扩展： 数据分片，负载均衡

1. 键值数据库包括什么
    访问框架 || 索引模块 || 操作模块 || 存储模块
    -网络框架接收到网络包，并按照相应的协议进行解析之后处理，也就是网络连接的处理、网络请求的解析，以及数据存取的处理I/O模型设计。
    -索引类型： hash B+树 字典树等。
    -操作 增删改查
    -存储模块： 分配器， 持久化
    redis还有 高可用集群支持模块 || 高扩展集群支持模块
2. redis操作：
    数据模型及操作接口：
    五种常用数据结构： String Set Hash List SortSet (简单动态字符串， 双向链表， 压缩列表， 哈希表，跳表， 整数数组)
    三种扩展数据结构： Geo, Hyperloglog, bitmap
    自定义数据结构： 。。。。

    redis内有一个全局哈希表，保存了所有键值对， 哈希桶中的entry元素中保存了*key和*value指针。
    Redis解决哈希冲突的方式，链式哈希--同一个哈希桶中的多个元素用一个链表来保存，它们之间依次用指针连接。
    如果链太长会降低效率。 所以会有rehash.
    rehash：
        redis默认使用两个全局hash表，刚开始默认使用hash1， hash2不分配空间，当1中元素多时，redis会开始rehash
        - 给2分配空间，把1中数据映射拷贝到2， 释放1空间。
        但是copy会有耗时阻塞。
    渐进式rehash:
        copy数据时，Redis正常处理客户端请求，每处理一个请求时，从hash1中的第一索引位置开始，顺带着将这个索引位置上的所有entries拷贝到哈希表2
        等待下一个请求到来，再顺带拷贝哈希表1中的下一个索引位置的entries。
        优点： 巧妙地把一次性大量拷贝的开销，分摊到了多次处理请求的过程中，避免了耗时操作，保证了数据的快速访问。

    底层数据结构操作效率
        压缩列表：zlbytes、zltail、zllen  列表长度、列表尾的偏移量和列表中的entry个数； 压缩列表在表尾还有一个zlend 表示列表结束。
        哈希表：O(1) 跳表 (logn) 双向链表 O(N) 压缩列表 O(N) 整数数组 O(N)
        单元素操作是基础；
        范围操作非常耗时；
        统计操作通常高效；  
        例外情况只有几个。

4. redis避免数据丢失
    一旦服务器宕机，内存中的数据将全部丢失。
    解决方案：
        从后端数据库读取恢复， 会给数据库带来巨大的压力，而且读取速度还变慢了。所以需要redis持久化存储；
    AOF（日志） RDB （快照）
      AOF
        AOF是写后日志， 先执行命令，把数据写入内存，然后才记录日志
        AOF里记录的是Redis收到的每一条命令，为了避免额外的检查开销，Redis在向AOF里面记录日志的时候，并不会先去对这些命令进行语法检查
        如果先记日志再执行命令的话，日志中就有可能记录了错误的命令，Redis在使用日志恢复数据时，就可能会出错。
        好处
            -这样可以避免出现记录错误命令。
            -它是在命令执行后才记录日志，所以不会阻塞当前的写操作。
        潜在风险：
            -如果刚执行完一个命令，还没有来得及记日志就宕机了，那么这个命令和相应的数据就有丢失的风险。
            -AOF虽然避免了对当前命令的阻塞，但可能会给下一个操作带来阻塞风险 AOF日志也是在主线程中执行的，如果在把日志文件写入磁盘时，
            磁盘写压力大，就会导致写盘很慢，进而导致后续的操作也无法执行了。
            这俩风险和AOF写回磁盘的时机相关
        三种写回策略：
            AOF配置项appendfsync的三个可选值。
            -Always,同步写回，每个命令执行完，立刻同步将日志写会磁盘
            -EverySec, 每秒写回，每个写命令执行完，只是先把日志写到AOF文件的内存缓冲区，每隔一秒把缓冲区中的内容写入磁盘
            -No，操作系统控制的写回：每个写命令执行完，只是先把日志写到AOF文件的内存缓冲区，由操作系统决定何时将缓冲区内容写回磁盘。
          针对避免主线程阻塞和减少数据丢失问题，这三种写回策略都无法做到两全其美。
            -同步写回 可以做到基本不丢数据，但是影响主线程性能
            -每秒写回 采用一秒写一次的频率，如果发生宕机，上一秒内未落盘的命令操作仍然会丢失。这只能算是在避免影响主线程性能和避免数据丢失两者取了个折中
        
        AOF文件过大
            缺点：
                -无法保存太大文件
                -追加效率低
                -恢复过程缓慢
            AOF重写
                将多个命令合成一个，但是是一个非常耗时的事情。
                重写是由后台子进程bgrewriteaof来完成，每次重写时，主线程fork出后台的bgrewriteaof子进程。
                创建后写时copy，写两个地方，1是AOF缓冲区 2是AOF重写缓存区，当子进程重写完旧的数据后，给父进程发信号然后把重写缓冲区的命令追加上去。

    
5. 内存快照 RDB 
    特点：恢复快。
    Redis提供了两个命令来生成RDB文件，分别是save和bgsave。
        -save：在主线程中执行，会导致阻塞；
        -bgsave：创建一个子进程，专门用于写入RDB文件，避免了主线程的阻塞，这也是Redis RDB文件生成的默认配置。
    快照的时候能修改吗？
        避免阻塞和正常处理写操作并不是一回事。主线程的确没有阻塞，可以正常接收请求 为了保证快照完整性，只能处理读操作，因为不能修改正在执行快照的数据。
        Redis就会借助操作系统提供的写时复制技术（Copy-On-Write, COW） 在执行快照的同时，正常处理写操作。
        bgsave子进程是由主线程fork生成的，可以共享主线程的所有内存数据。bgsave子进程运行后，开始读取主线程的内存数据，并把它们写入RDB文件。
        bgsave保存的还是每更改之前的数据版本。
    多长时间做一次快照？
        太勤的话：
            -大量磁盘写操作，竞争有限的磁盘带宽
            -频繁fork出bgsave子进程，fork这个创建过程本身会阻塞主线程，
    
    上面是全量快照？ 而增量快照是借助AOF,在Redis4中有这种实现。

6. 主从复制
    可靠性： 
        一是数据尽量少丢失，二是服务尽量少中断。 
    
    针对二：Redis的做法就是增加副本冗余量。
        主从库模式，以保证数据副本的一致，采用读写分离
        读操作：主库、从库都可以接收；
        写操作：首先到主库执行，然后，主库将写操作同步给从库。

    主从之间第一次同步：
        启动时通过replicaof，来形成主库从库关系。
        -主从库间建立连接、协商同步的过程，为全量复制做准备
            -从库向主库发送psync表示要进行数据同步. psync命令包含了主库的runID和复制进度offset两个参数。
                runid: 每个Redis实例启动时都会自动生成的一个随机ID
            -主库收到psync命令后，用FULLRESYNC响应命令带上两个参数，主库runID和主库目前的复制进度offset，返回给从库。
                FULLRESYNC响应表示第一次复制采用的全量复制，也就是说，主库会把当前所有的数据都复制给从库。
        -主库将所有数据同步给从库。从库收到数据后，在本地完成数据加载。
            -主库执行bgsave命令，生成RDB文件，发送给从库
                发送给从库的时候，主库不会停止服务，仍然可以正常接收请求，请求中的写操作并没有记录到刚刚生成的RDB文件中。
                主库会在内存中用专门的replication buffer，记录RDB文件生成后收到的所有写操作。
            -从库接收后，会先清空当前数据库，然后加载RDB文件。
        -主库会把第二阶段执行过程中新收到的写命令，再发送给从库
            -当主库完成RDB文件发送后，就会把此时replication buffer中的修改操作发给从库，从库再重新执行这些操作。这样一来，主从库就实现同步了。
        
    主从级联模式分担全量复制时的主库压力
        对于主库来说，需要完成两个耗时的操作：生成RDB文件和传输RDB文件。
        如果从库很多，而且都要和主库进行全量复制的话，就会导致主库忙于fork子进程生成RDB文件，进行数据全量同步。 会导致主库响应慢，主库的网络带宽资源紧张。
        -主从从

        一旦主从库完成了全量复制，它们之间就会一直维护一个网络连接，主库会通过这个连接将后续陆续收到的命令操作再同步给从库，这个过程也称为基于长连接的命令传播

    主从库网络锻炼断链怎么办？
        主从库会采用增量复制的方式继续同步。 
            全量复制是同步所有数据，而增量复制只会把主从库网络断连期间主库收到的命令，同步给从库。
            当主从库断连后，主库会把断连期间收到的写操作命令，写入replication buffer，同时也会把这些操作命令也写入repl_backlog_buffer这个缓冲区。
            repl_backlog_buffer是一个环形缓冲区，主库会记录自己写到的位置，从库则会记录自己已经读到的位置。

          缺点： 如果从库的读取速度比较慢，就有可能导致从库还未读取的操作被主库新写的操作覆盖了，这会导致主从库间的数据不一致。
  
7.  哨兵模式
    哨兵机制是实现主从库自动切换的关键机制  （监控，选主，通知）
        -主库真的挂了吗？
        -该选择哪个从库作为主库？
        -怎么把新主库的相关信息通知给从库和客户端呢？
      监控：
        监控是指哨兵进程在运行时，周期性向所有的主从库发送PING命令，检测它们是否仍然在线运行，如果没有在规定时间内响应，会被认为是下线状态，
            检测的如果是从库，直接判断为主观下线， 如果是主库，判断为客观下线
        哨兵就会判定主库下线，然后开始自动切换主库的流程
      选主：
        哨兵就需要从很多个从库里按照一定的规则选择一个从库实例，把它作为新的主库
      通知：
        哨兵会把新主库的连接信息发给其他从库，让它们执行replicaof命令和新主库建立连接，并进行数据复制。同时，
        哨兵会把新主库的连接信息通知给客户端，让它们把请求操作发到新主库上。
    
    通常哨兵机制是以集群来实现的，称为哨兵集群  
        客观下线”的标准就是，当有N个哨兵实例时，最好要有N/2 + 1个实例判断主库为“主观下线。
    如何选新主库：
        筛选+打分 先筛去不符合条件的从库，然后再打分，要分最高的。
        筛选： 在线状态+ 网络状态好
        打分： 优先级、 同步程度最高、id号最小。
    
8. 哨兵机制的原理
    部署哨兵集群时，在配置哨兵的信息时，设置主库的IP和端口。

    .基于pub/sub机制的哨兵集群组成 发布订阅机制
        哨兵与主库建立链接后，就可以在主库上发布消息。 当多个哨兵实例都在主库上做了发布和订阅操作后，它们之间就能知道彼此的IP地址和端口。
            -哨兵向主库发送INFO命令来完成的得到所有的从库信息（端口，ip)
        应用程序也可以在主库上发布订阅消息， 为了区分不同应用的消息，Redis会以频道的形式进行管理。 
        只有订阅了同一个频道的应用，才能通过发布的消息进行信息交换。

    .基于pub/sub机制的客户端事件通知
        客户端可以从哨兵订阅消息 : 主库下线事件｜｜从库重新配置 ｜｜新主库切换

    .由哪个哨兵执行主从切换？
        也是一个投票过程， 要拿到大于等于哨兵配置文件中的quorum值。
    经验：要保证所有哨兵实例的配置是一致的，尤其是主观下线的判断值down-after-milliseconds

9. 数据增多， 数据切片
    -纵向扩展
        纵向扩展会受到硬件和成本的限制
    -横向扩展
        在面向百万、千万级别的用户规模时，横向扩展的Redis切片集群会是一个非常好的选择。

    -数据切片和实例的对应分布关系
        采用哈希槽，似于数据分区，每个键值对都会根据它的key，被映射到一个哈希槽中。
            -按照CRC16算法计算一个16 bit的值，再用这个16bit值对16384取模，得到0~16383范围内的模数，每个模数代表一个相应编号的哈希槽
            可以使用cluster create命令创建集群，此时，Redis会自动把这些槽平均分布在集群实例上
            你可以根据不同实例的资源配置情况，使用cluster addslots命令手动分配哈希槽。
    -客户端如何定位数据：
        客户端和集群实例建立连接后，实例就会把哈希槽的分配信息发给客户端。
           在集群刚刚创建的时候，每个实例只知道自己被分配了哪些哈希槽，是不知道其他实例拥有的哈希槽信息的。
            Redis实例会把自己的哈希槽信息发给和它相连接的其它实例，来完成哈希槽分配信息的扩散。当实例之间相互连接后，每个实例就有所有哈希槽的映射关系了。
        客户端收到哈希槽信息后，会把哈希槽信息缓存在本地，当客户端请求键值对时，会先计算键所对应的哈希槽，然后就可以给相应的实例发送请求了。
    -实例和哈希槽会变化的：
        实例有新增或删除，Redis需要重新分配哈希槽；
        为了负载均衡，Redis需要把哈希槽在所有实例上重新分布一遍。
    -MOVED 和 ASKing
      Redis Cluster方案提供了一种重定向机制，客户端给一个实例发送数据读写操作时，这个实例上并没有相应的数据，客户端要再给一个新实例发送操作命令。
        当改实例上没有时，会给客户端返回一个MOVED命令这个结果中就包含了新实例的访问地址。然后会更新本地缓存
        而槽部分数据没有迁移，会返回ASKing, ASK命令并不会更新客户端缓存的哈希槽分配信息
       
 10.  String类型结构分析
    -String类型内存开销大
        当保存64位整数时，String会把他保存为一个8字节的Long类型整数，叫做int编码
        当保存数据包含字符串时，会用SDS结构保存。 len(4B) + alloc(4B) + buf(...). len表示已用的长度， alloc表示分配的长度。


    Redis的数据类型有很多，不同数据类型都有些相同的元数据要记，Redis会用一个RedisObject结构体来统一记录这些元数据，同时指向实际数据。 8b元数据 + 8b指针
    当保存是Long类型的时候，指针直接被赋值为整数了。
    当保存是字符串时，如果小于44字节， RedisObject中的元数据、指针和SDS是一块连续的内存区域， 叫做embstr编码
    当大于44字节， 会给SDS单独分布空间，这种布局方式被称为raw编码模式。
    dictEntry 有三个8B的指针， 分别指向key,value,next_dictEntry. Redis使用的内存分配库jemalloc了， 找最接近N的2的幂次数作为分配的空间
    info memory查看内存开销。

11.  集合统计
    聚合统计 排序统计 二值状态统计 基数统计

    聚合统计
        -统计多个集合元素的聚合结果  SUNIONSTORE（并集）  SDIFFSTORE（差集） SINTERSTORE（交集）
        Set的差集、并集和交集的计算复杂度较高，数据量较大的情况下，会导致Redis实例阻塞 
        可以从主从集群中选择一个从库，让它专门负责聚合计算，或者是把数据读取到客户端，在客户端来完成聚合统计
    
    排序统计：
      List和Sorted Set就属于有序集合
        -List是按照元素进入List的顺序进行排序的
        -Sorted Set可以根据元素的权重来排序
        在面对需要展示最新列表、排行榜等场景时，如果数据更新频繁或者需要分页显示，建议优先考虑使用Sorted Set。
    
    二值状态统计
      二值状态就是指集合元素的取值就只有0和1两种
        Bitmap  GETBIT/SETBIT操作，使用一个偏移值offset对bit数组的某一个bit位进行读和写 Bitmap的偏移量是从0开始
        Bitmap还提供了BITCOUNT操作，用来统计这个bit数组中所有“1”的个数。
    
    基数统计
      基数统计就是指统计一个集合中不重复的元素个数。 数据很多时，set与hash会很耗空间。
      Redis提供的HyperLogLog，是一种用于统计基数的数据集合类型  每个 HyperLogLog只需要花费 12 KB 内存，就可以计算接近 2^64 个元素的基数
      PFADD PFCOUNT  是基于概率统计的，它给出的统计结果是有一定误差的，标准误算率是0.81%。

12. GEO
    基于位置信息服务(LBS)。 Redis采用了业界广泛使用的GeoHash编码方法， “二分区间，区间编码”。
    GEO
        GEOADD命令：用于把一组经纬度信息和相对应的一个ID记录到GEO类型集合中；
        GEORADIUS命令：会根据输入的经纬度位置，查找以这个经纬度为中心的一定范围内的其他元素。

    自定义数据结构：
        Redis键值对中的每一个值都是用RedisObject保存的。
        RedisObject内部包含type,、encoding,、lru和refcount 4个元数据，以及1个*ptr指针。
            -type：表示值的类型  如五大基础类型
            -encoding 编码方式 各个基本类型的底层数据结构
            -lru 记录了这个对象最后一次被访问的时间，用于淘汰过期的键值对
            -refcount：记录了对象的引用计数
            *ptr：是指向数据的指针。
        开发一个新的数据结构步骤
            -定义新数据类型的底层结构
            -在RedisObject的type属性中，增加这个新类型的定义
            -开发新类型的创建和释放函数
            -开发新类型的命令操作

13. redis保存时间序列
    基于hash 和 sort set， hash不能范围查询
        Redis用来实现简单的事务的MULTI和EXEC命令。当多个命令及其参数本身无误时，MULTI和EXEC命令可以保证执行这些命令时的原子性
            -MULTI命令：表示一系列原子性操作的开始。收到这个命令后，Redis就知道，接下来再收到的命令需要放到一个内部队列中，后续一起执行，保证原子性。
            -EXEC命令：表示一系列原子性操作的结束。一旦Redis收到了这个命令，就表示所有要保证原子性的命令操作都已经发送完成了。
                此时，Redis开始执行刚才放到内部队列中的所有命令操作。
    基于RedisTimeSeries模块保存时间序列数据 
        RedisTimeSeries是Redis的一个扩展模块
        RedisTimeSeries不属于Redis的内建功能模块，我们需要先把它的源码单独编译成动态链接库redistimeseries.so，
        再使用loadmodule命令进行加载，  loadmodule redistimeseries.so

14. redis用作消息队列
    消息队列在存取消息时，必须要满足三个需求，分别是消息保序、处理重复的消息和保证消息可靠性。
        -消息保序： 消费者仍然需要按照生产者发送消息的顺序来处理消息，避免后发送的消息被先处理了
        -重复消息处理：消费者从消息队列读取消息时，有时会因为网络堵塞而出现消息重传的情况。
        -保证消息可靠性：消费者在处理消息的时候，还可能出现因为故障或宕机导致消息没有处理完成的情况。当消费者重启后，可以重新读取消息再次进行处理。

    基于List的消息队列
      实现：
        生产者可以使用LPUSH命令把要发送的消息依次写入List，而消费者则可以使用RPOP命令，从List的另一端按照消息的写入顺序，依次读取消息并进行处理。
      缺点：
        生产者忘List写数据时，List不会主动通知消费者有新消息写入，如果消费者想要及时处理消息，程序需要不断的调用drop命令
        redis供了BRPOP命令，称为阻塞式读取。客户端在没有读到队列数据时，自动阻塞，直到有新的数据写入队列，再开始读取新数据。
        和消费者程序自己不停地调用RPOP命令相比，这种方式能节省CPU开销。
        
        -消费者程序本身能对重复消息进行判断
            消息队列要能给每一个消息提供全局唯一的ID号
            消费者程序要把已经处理过的消息的ID号记录下来。
        -List类型是如何保证消息可靠性的
            List类型提供了BRPOPLPUSH命令，这个命令的作用是让消费者程序从一个List中读取消息，
            同时，Redis会把这个消息再插入到另一个List（可以叫作备份List）留存
      问题：
        生产者消息发送很快，而消费者处理消息的速度比较慢，这就导致List中的消息越积越多，给Redis的内存带来很大压力
        我们希望启动多个消费者程序组成一个消费组，一起分担处理List中的消息。但是，List类型并不支持消费组的实现

    基于Streams的消息队列解决方案
      Streams是Redis专门为消息队列设计的数据类型，它提供了丰富的消息队列操作命令。
        -XADD：插入消息，保证有序，可以自动生成全局唯一ID；
            XADD mqstream * repo 5   *表示自动生成一个全局唯一的ID key: repo value: 5
             自动生成的id由两部分组成 时间+ 1s内的第几个。
        -XREAD：用于读取消息，可以按ID读取数据；
            消费者需要读消息时，直接使用XREAD命令从消息队列中读取
            XREAD在读取消息时，可以指定一个消息ID，并从这个消息ID的下一条消息开始进行读取
            XREAD BLOCK 100（ms) STREAMS  mqstream 1599203861727-0 || $. $表示读取最新消息
                消费者也可以在调用XRAED时设定block配置项，实现类似于BRPOP的阻塞读取操作，当消息队列中没有消息时，
                一旦设置了block配置项，XREAD就会阻塞，阻塞的时长可以在block配置项进行设置。
                当超过超时时间时，会返回null


        -XREADGROUP：按消费组形式读取消息；
            Stream特有功能， XGROUP create mqstream group1 0  创建一个名为group1的消费组，这个消费组消费的消息队列是mqstream。
            XREADGROUP group group1 consumer2  streams mqstream 0
            消息队列中的消息一旦被消费组里的一个消费者读取了，就不能再被该消费组内的其他消费者读取了

        -XPENDING和XACK：XPENDING命令可以用来查询每个消费组内所有消费者已读取但尚未确认的消息，而XACK命令用于向消息队列确认消息处理已完成。
            为了保证消费者在发生故障或宕机再次重启后，仍然可以读取未处理完的消息，Streams会自动使用内部队列
            （也称为PENDING List）留存消费组里每个消费者读取的消息，直到消费者使用XACK命令通知Streams“消息已经处理完成”。
            如果消费者没有成功处理消息，它不会给Streams发送XACK命令，消息仍然会留存 消费者在重启后，用XPENDING命令查看已读取、但尚未确认处理完成的消息。

        Streams是Redis 5.0专门针对消息队列场景设计的数据类型, 适用于轻量级的消息队列，不需要而外的组件。

16. 异步机制，避免单线程模型阻塞
   影响redis性能的5大因素
        -Redis内部的阻塞式操作；
        -CPU核和NUMA架构的影响；
        -Redis关键系统配置
        -Redis内存碎片
        -Redis缓冲区。
    
    Redis阻塞点：
        客户端：网络IO，键值对增删改查操作，数据库操作
        磁盘：生成RDB快照，记录AOF日志，AOF日志重写；
        主从节点：主库生成、传输RDB文件，从库接收RDB文件、清空数据库、加载RDB文件；
        切片集群实例：向其他实例传输哈希槽信息，数据迁移。
    
      客户端
        网络IO采用IO多路复用，不会影响
        1.复杂度高的增删改查操作肯定会阻塞Redis。 
            -（集合全量查询和聚合操作。）
            - 删除操作 第一步释放内存，操作系统把释放的内存放到一个空闲内存块的链表，以便后续进行管理和再分配。 可能会影响主线程阻塞。
                删除大量键值对的时候，最典型的就是删除包含了大量元素的集合，也称为bigkey删除
        2.bigkey删除操作就是Redis的第二个阻塞点
        3.清空数据库。
      磁盘：
        Redis进一步设计为采用子进程的方式生成RDB快照文件，以及执行AOF日志重写操作，这两个操作由子进程负责执行，慢速的磁盘IO就不会阻塞主线程了。
            Redis直接记录AOF日志时，会根据不同的写回策略对数据做落盘保存个同步写磁盘的操作的耗时大约是1～2ms，
            如果有大量的写操作需要记录在AOF日志中，并同步写回的话，就会阻塞主线程了
        4. AOF日志同步写
      主从节点交互时：
        在主从集群中，主库需要生成RDB文件，并传输给从库。主库在复制的过程中，创建和传输RDB文件都是由子进程来完成的，不会阻塞主线程
        但是，对于从库来说，它在接收了RDB文件后，需要使用FLUSHDB命令清空当前数据库，这就正好撞上了刚才我们分析的第三个阻塞点。
        此外，从库在清空当前数据库后，还需要把RDB文件加载到内存，这个过程的快慢和RDB文件的大小密切相关，RDB文件越大，加载过程越慢，
        5.加载RDB文件就成为了Redis的第五个阻塞点。
      切片集群实例交互时
        当我们部署Redis切片集群时，每个Redis实例上分配的哈希槽信息需要在不同实例间进行传递，同时，当需要进行负载均衡或者有实例增删时，
        数据会在不同的实例间进行迁移。不过，哈希槽的信息量不大，而数据迁移是渐进式执行的，所以，一般来说，这两类操作对Redis主线程的阻塞风险不大。
        -如果你使用了Redis Cluster方案，而且同时正好迁移的是bigkey的话，就会造成主线程的阻塞，因为Redis Cluster使用了同步迁移。
     
     综上所诉 主要上面5点来阻塞主线程长时间无法服务其他请求。
       解决办法：
        Redis提供了异步线程机制。所谓的异步线程机制，就是指，Redis会启动一些子线程，然后把一些任务交给这些子线程，
        让它们在后台完成，而不再由主线程来执行这些任务。使用异步线程机制执行操作，可以避免阻塞主线程。

        1，5不可以， 2，3，4可以

    异步子线程机制
        -Redis主线程启动后，会使用操作系统提供的pthread_create函数创建3个子线程，分别由它们负责AOF日志写操作、键值对删除以及文件关闭的异步执行。
        -主线程通过一个链表形式的任务队列和子线程进行交互。当收到键值对删除和清空数据库的操作时，主线程会把这个操作封装成一个任务，
            放入到任务队列中，然后给客户端返回一个完成信息，表明删除已经完成。
            但实际上，这个时候删除还没有执行，等到后台子线程从任务队列中读取任务后，才开始实际删除键值对，并释放相应的内存空间。因此，
            我们把这种异步删除也称为惰性删除（lazy free）。此时，删除或清空操作不会阻塞主线程，这就避免了对主线程的性能影响。
            和惰性删除类似，当AOF日志配置成everysec选项后，主线程会把AOF写日志操作封装成一个任务，也放到任务队列中。后台子线程读取任务后，
            开始自行写入AOF日志，这样主线程就不用一直等待AOF日志写完了。
        异步的键值对删除和数据库清空操作是Redis 4.0后提供的功能，Redis也提供了新的命令来执行这两个操作。
            键值对删除：当你的集合类型中有大量元素（例如有百万级别或千万级别元素）需要删除时，建议使用UNLINK命令。
            清空数据库：可以在FLUSHDB和FLUSHALL命令后加上ASYNC选项，这样就可以让后台子线程异步地清空数据库，如下所示：

                FLUSHDB ASYNC   
                FLUSHALL AYSNC

17.  CPU结构影响性能
    CPU的多核架构以及多CPU架构，也会影响到Redis的性能

    主流cpu架构
        一个CPU处理器中有多个运行核心，一个运行核心称为一个物理核， 每个物理核都可以运行应用程序。
        每个物理核都拥有私有一级缓存，（包括一级指令缓存和一级数据缓存， 及私有二级缓存
        它其实是指缓存空间只能被当前的这个物理核使用，其他的物理核无法对这个核的缓存空间进行数据存取
        
        现在主流的CPU处理器中，每个物理核通常都会运行两个超线程，也叫作逻辑核。同一个物理核的逻辑核会共享使用L1、L2缓存。
        当数据或指令保存在L1、L2缓存时，物理核访问它们的延迟不超过10纳秒 
        这些L1和L2缓存的大小受限于处理器的制造技术，一般只有KB级别，存不下太多的数据
        访问内存来获取数据。而应用程序的访存延迟一般在百纳秒级别
        不同的物理核还会共享一个共同的三级缓存（L3 cache) 几MB到几十MB，
        
        在多CPU架构上，应用程序可以在不同的处理器上运行。
           如果程序在一个核上，数据被保存在内存，如果被调度到另外一个核上，就需要访问之前Socket上连接的内存，
           这种访问属于远端内存访问。和访问Socket直接连接的内存相比，远端内存访问会增加应用程序的延迟。

        -在多CPU架构下，一个应用程序访问所在Socket的本地内存和访问远端内存的延迟并不一致，
        所以，我们也把这个架构称为非统一内存访问架构（Non-Uniform Memory Access，NUMA架构）。

        L1、L2缓存中的指令和数据的访问速度很快，所以，充分利用L1、L2缓存，可以有效缩短应用程序的执行时间；
            在NUMA架构下，如果应用程序从一个Socket上调度到另一个Socket上，就可能会出现远端内存访问的情况，
            这会直接增加应用程序的执行时间。

    CPU的NUMA架构对Redis性能的影响
        -尝试着把Redis实例和CPU核绑定了，让一个Redis实例固定运行在一个CPU核上。
        可以使用taskset命令把一个程序绑定在一个核上运行。
            taskset -c 0 ./redis-server
            通过绑定Redis实例和CPU核，可以有效降低Redis的尾延迟。当然，绑核不仅对降低尾延迟有好处
            同样也能降低平均延迟、提升吞吐率，进而提升Redis性能。

        为了提升Redis的网络性能，把操作系统的网络中断处理程序和CPU核绑定 
        这个做法可以避免网络中断处理程序在不同核上来回调度执行，的确能有效提升Redis的网络处理性能。

        如果网络中断处理程序和Redis实例各自所绑的CPU核不在同一个CPU Socket上，那么，Redis实例读取网络数据时，
        就需要跨CPU Socket访问内存，这个过程会花费较多时间。

        在CPU的NUMA架构下，对CPU核的编号规则，并不是先把一个CPU Socket中的所有逻辑核编完，
        再对下一个CPU Socket中的逻辑核编码，而是先给每个CPU Socket中每个物理核的第一个逻辑核依次编号，
        再给每个CPU Socket中的物理核的第二个逻辑核依次编号。

        lscpu 查看到这些核的编号：

    绑核风险与解决方案：
        当我们把Redis实例绑到一个CPU逻辑核上时，就会导致子进程、后台线程和Redis主线程竞争CPU资源，
        一旦子进程或后台线程占用CPU时，主线程就会被阻塞，导致Redis请求延迟增加。
      解决方案： 一个Redis实例对应绑一个物理核和优化Redis源码。
            taskset -c 0,12 ./redis-server
            和只绑一个逻辑核相比，把Redis实例和物理核绑定，可以让主线程、子进程、后台线程共享使用2个逻辑核，可以在一定程度上缓解CPU资源竞争。
            因为只用了2个逻辑核，它们相互之间的CPU竞争仍然还会存在。

        优化redis源码：
            通过编程实现绑核时，要用到操作系统提供的1个数据结构cpu_set_t和3个函数CPU_ZERO、CPU_SET和sched_setaffinity，

            cpu_set_t数据结构：是一个位图，每一位用来表示服务器上的一个CPU逻辑核。
            CPU_ZERO函数：以cpu_set_t结构的位图为输入参数，把位图中所有的位设置为0。
            CPU_SET函数：以CPU逻辑核编号和cpu_set_t位图为参数，把位图中和输入的逻辑核编号对应的位设置为1。
            sched_setaffinity函数：以进程/线程ID号和cpu_set_t为参数，检查cpu_set_t中哪一位为1，
                就把输入的ID号所代表的进程/线程绑在对应的逻辑核上。
        示例代码：
            //线程函数
    void worker(int bind_cpu){
        cpu_set_t cpuset;  //创建位图变量
        CPU_ZERO(&cpu_set); //位图变量所有位设置0
        CPU_SET(bind_cpu, &cpuset); //根据输入的bind_cpu编号，把位图对应为设置为1
        sched_setaffinity(0, sizeof(cpuset), &cpuset); //把程序绑定在cpu_set_t结构位图中为1的逻辑核
        //实际线程函数工作
    }

    int main(){
        pthread_t pthread1
        //把创建的pthread1绑在编号为3的逻辑核上
        pthread_create(&pthread1, NULL, (void *)worker, 3);
    }
            
18.  波动延迟分析
    查看Redis的响应延迟。
        ./redis-cli --intrinsic-latency 120。该命令会打印120秒内监测到的最大延迟

    Redis自身操作特性的影响：
        1.慢查询命令
            就是指在Redis中执行速度慢的命令，这会导致Redis延迟增加
          解决办法：
            用其他高效命令代替。
            当你需要执行排序、交集、并集操作时，可以在客户端完成，而不要用SORT、SUNION、SINTER这些命令，以免拖慢Redis实例
        2. 过期key操作
            过期key的自动删除机制。它是Redis用来回收内存空间的常用机制
            Redis键值对的key可以设置过期时间。默认情况下，Redis每100毫秒会删除一些过期key，具体的算法如下：
                1.采样ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP个数的key，并将其中过期的key全部删除
                2.如果超过25%的key过期了，则重复删除的过程，直到过期key的比例降至25%以下。

            ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP是Redis的一个参数，默认是20，那么，一秒内基本有200个过期key会被删除。
            这一策略对清除过期key、释放内存空间很有帮助。如果每秒钟删除200个过期key，并不会对Redis造成太大影响
            
            如果触发了上面这个算法的第二条，Redis就会一直删除以释放内存空间。注意，删除操作是阻塞的（Redis 4.0后可以用异步线程机制来减少阻塞影响）
            一旦该条件触发，Redis的线程就会一直执行删除，这样一来，就没办法正常服务其他的键值操作了，
            就会进一步引起其他键值操作的延迟增加，Redis就会变慢。

            频繁使用带有相同时间参数的EXPIREAT命令设置过期key，这就会导致，在同一秒内有大量的key同时过期。
            遇到这种情况时，千万不要嫌麻烦，你首先要根据实际业务的使用需求，决定EXPIREAT和EXPIRE的过期时间参数。
            其次，如果一批key的确是同时过期，你还可以在EXPIREAT和EXPIRE的过期时间参数上，加上一个一定大小范围内的随机数，
            既保证了key在一个邻近时间范围内被删除，又避免了同时过期造成的压力。


19.  波动延迟分析
    Redis会采用AOF日志或RDB快照。其中，AOF日志提供了三种日志写回策略：no、everysec、always。
    这三种写回策略依赖文件系统的两个系统调用完成，也就是write和fsync。

    write只要把日志记录写到内核缓冲区，就可以返回了，并不需要等待日志实际写回到磁盘；而fsync需要把日志记录写回到磁盘后才能返回，时间较长。

    AOF回写：
        no： 调用write写日志文件，由操作系统周期将日志写回磁盘。
        everysec 每秒调用一次fsync,将日志写到磁盘
        always : 每执行一个操作，调用一个fsync

        在使用everysec时，Redis允许丢失一秒的操作记录，Redis主线程并不需要确保每个操作记录日志都写回磁盘。fsync的执行时间很长，
        如果是在Redis主线程中执行fsync，就容易阻塞主线程。所以，当写回策略配置为everysec时，Redis会使用后台的子线程异步完成fsync的操作。

        而对于always策略来说，Redis需要确保每个操作记录日志都写回磁盘，如果用后台子线程异步完成，主线程就无法及时地知道每个操作是否已经完成了，
        这就不符合always策略的要求了。所以，always策略并不使用后台子线程来执行。

        另外，在使用AOF日志时，为了避免日志文件不断增大，Redis会执行AOF重写，生成体量缩小的新的AOF日志文件。
        AOF重写本身需要的时间很长，也容易阻塞Redis主线程，所以，Redis使用子进程来进行AOF重写。

        AOF重写会对磁盘进行大量IO操作，同时，fsync又需要等到数据写到磁盘后才能返回，当AOF重写的压力比较大时，就会导致fsync被阻塞。
        虽然fsync是由后台子线程负责执行的，但是，主线程会监控fsync的执行进度。

        当主线程使用后台子线程执行了一次fsync，需要再次把新接收的操作记录写回磁盘时，如果主线程发现上一次的fsync还没有执行完，那么它就会阻塞。
        所以，如果后台子线程执行的fsync频繁阻塞的话（比如AOF重写占用了大量的磁盘IO带宽），主线程也会阻塞，导致Redis性能变慢。

        由于fsync后台子线程和AOF重写子进程的存在，主IO线程一般不会被阻塞。但是，如果在重写日志时，AOF重写子进程的写入量比较大，fsync线程也会被阻塞
        ，进而阻塞主线程，导致延迟增加。

        可以检查下Redis配置文件中的appendfsync配置项，该配置项的取值表明了Redis实例使用的是哪种AOF日志写回策略，
            no : appendfsync no
            everysec: appendfsync everysec
            always: appendfsync always
            如果业务应用对延迟非常敏感，但同时允许一定量的数据丢失，那么，可以把配置项no-appendfsync-on-rewrite设置为yes    
            no-appendfsync-on-rewrite yes
                -这个为yes时，表示在AOF重写时，不进行fsync操作，也就是说，Redis实例把写命令写到内存后不调用后台线程进行fsync操作，就可以直接返回了。
                当然，如果此时实例发生宕机，就会导致数据丢失。反之，如果这个配置项设置为no（也是默认配置），在AOF重写时，
                Redis实例仍然会调用后台线程进行fsync操作，这就会给实例带来阻塞。

            或者采用高速的固态硬盘作为AOF日志的写入设备。
            高速固态盘的带宽和并发度比传统的机械硬盘的要高出10倍及以上。在AOF重写和fsync后台线程同时执行时，固态硬盘可以提供较为充足的磁盘IO资源，
            让AOF重写和fsync后台线程的磁盘IO资源竞争减少，从而降低对Redis的性能影响。

        触发swap的原因主要是物理机器内存不足：
            Redis实例自身使用了大量的内存，导致物理机器的可用内存不足；
            和Redis实例在同一台机器上运行的其他进程，在进行大量的文件读写操作。文件读写本身会占用系统内存，这会导致分配给Redis实例的内存量变少，
            进而触发Redis发生swap。
        解决：
            ：增加机器的内存或者使用Redis集群

    操作系统：内存大页：
        Linux内核从2.6.38开始支持内存大页机制,虽然内存大页可以给Redis带来内存分配方面的收益.
        redis采用采用写时复制机制，旦有数据要被修改，Redis并不会直接修改内存中的数据，而是将这些数据拷贝一份，然后再进行修改
        当客户端请求修改或新写入数据较多时，内存大页机制将导致大量的拷贝，这就会影响Redis正常的访存操作，最终导致性能变慢。
        我们要先排查下内存大页
            -cat /sys/kernel/mm/transparent_hugepage/enabled
                如果执行结果是always，就表明内存大页机制被启动了；如果是never，就表示，内存大页机制被禁止。

         在实际生产环境中部署时，建议不要使用内存大页机制，操作也很简单，只需要执行下面的命令就可以了：
            echo never /sys/kernel/mm/transparent_hugepage/enabled

20. 删除数据后，内存占用率还高
    数据删除后，Redis释放的内存空间会由内存分配器管理，并不会立即返回给操作系统。所以，操作系统仍然会记录着给Redis分配了大量内存。
    风险：
        Redis释放的内存空间可能并不是连续的，虽然有空闲空间，Redis却无法用来保存数据，不仅会减少Redis能够实际保存的数据量，
        还会降低Redis运行机器的成本回报率。
    内存碎片：
      形成原因：
        内因是操作系统的内存分配机制
        外因是Redis的负载特征。

        内存分配机制：
            Redis可以使用libc、jemalloc、tcmalloc多种内存分配器来分配内存，默认使用jemalloc
                jemalloc的分配策略之一，是按照一系列固定的大小划分内存空间，2的n次方
        外因键值对大小不一样和删改操作
            Redis通常作为共用的缓存系统或键值数据库对外提供服务，，所以，不同业务应用的数据都可能保存在Redis中，这就会带来不同大小的键值对。
            这样一来，Redis申请内存空间分配时，本身就会有大小不一的空间需求。这是第一个外因。
            第二个外因是，这些键值对会被修改和删除，这会导致空间的扩容和释放，如果修改后的键值对变大或变小了，就需要占用额外的空间或者释放不用的空间。
            另一方面，删除的键值对就不再需要内存空间了，此时，就会把空间释放出来，形成空闲空间。

    如何判断是否有内存碎片？
        INFO memory
            mem_fragmentation_ratio的指标，它表示的就是Redis当前的内存碎片率    
                used_memory_rss/used_memory
                used_memory_rss是操作系统实际分配给Redis的物理内存空间,里面就包含了碎片；而used_memory是Redis为了保存数据实际申请使用的空间。

                -mem_fragmentation_ratio 大于1但小于1.5。这种情况是合理的。这是因为，刚才我介绍的那些因素是难以避免的。毕竟，
                内因的内存分配器是一定要使用的，分配策略都是通用的，不会轻易修改；而外因由Redis负载决定，也无法限制。所以，存在内存碎片也是正常的。
                -mem_fragmentation_ratio 大于 1.5 。这表明内存碎片率已经超过了50%。这个时候，我们就需要采取一些措施来降低内存碎片率了。

    如何清理内存碎片？
        -方法就是重启Redis实例
            -如果Redis中的数据没有持久化，那么，数据就会丢失；
            -即使Redis数据持久化了，我们还需要通过AOF或RDB进行恢复，恢复时长取决于AOF或RDB的大小，如果只有一个Redis实例，恢复阶段无法提供服务。
        -从4.0后Redis自身提供了一种内存碎片自动清理的方法
            “搬家让位，合并空间”。
            当有数据把一块连续的内存空间分割成好几块不连续的空间时，操作系统就会把数据拷贝到别处。此时，
            数据拷贝需要能把这些数据原来占用的空间都空出来，把原本不连续的内存空间变成连续的空间

            碎片清理是有代价的，操作系统需要把多份数据拷贝到新位置，把原有空间释放出来，这会带来时间开销。因为Redis是单线程，在数据拷贝时，
            Redis只能等着，这就导致Redis无法及时处理请求，性能就会降低。而且，有的时候，数据拷贝还需要注意顺序，就像刚刚说的清理内存碎片的例子，
            操作系统需要先拷贝D，并释放D的空间后，才能拷贝B。这种对顺序性的要求，会进一步增加Redis的等待时间，导致性能降低。

            解决办法：
                Redis需要启用自动内存碎片清理，可以把activedefrag配置项设置为yes
                config set activedefrag yes
                这个命令只是启用了自动清理功能，但是，具体什么时候清理，会受到下面这两个参数的控制。这两个参数分别设置了触发内存清理的一个条件，
                如果同时满足这两个条件，就开始清理。在清理的过程中，只要有一个条件不满足了，就停止自动清理。
                    -active-defrag-ignore-bytes 100mb：表示内存碎片的字节数达到100MB时，开始清理；
                    active-defrag-threshold-lower 10：表示内存碎片空间占操作系统分配给Redis的总空间比例达到10%时，开始清理。

                为了尽可能减少碎片清理对Redis正常请求处理的影响，自动内存碎片清理功能在执行时，还会监控清理操作占用的CPU时间，而且还设置了两个参数，
                分别用于控制清理操作占用的CPU时间比例的上、下限，既保证清理工作能正常进行，又避免了降低Redis性能。这两个参数具体如下：
                    active-defrag-cycle-min 25： 表示自动清理过程所用CPU时间的比例不低于25%，保证清理能正常开展；
                    active-defrag-cycle-max 75：表示自动清理过程所用CPU时间的比例不高于75%，一旦超过，就停止清理，从而避免在清理时，
                    大量的内存拷贝阻塞Redis，导致响应延迟升高。

21. 缓存区：
    缓冲区的功能其实很简单，主要就是用一块内存空间来暂时存放命令数据，以免出现因为数据和命令的处理速度慢于发送速度而导致的数据丢失和性能问题。
    但因为缓冲区的内存空间有限，如果往里面写入数据的速度持续地大于从里面读取数据的速度，就会导致缓冲区需要越来越多的内存来暂存数据。
    当缓冲区占用的内存超出了设定的上限阈值时，就会出现缓冲区溢出。

    缓冲区在Redis中的一个主要应用场景，就是在客户端和服务器端之间进行通信时，用来暂存客户端发送的命令数据，
    或者是服务器端返回给客户端的数据结果。
    缓冲区的另一个主要应用场景，是在主从节点间进行数据同步时，用来暂存主节点接收的写命令和数据。
    
    输入缓冲区就是用来暂存客户端发送的请求命令的，所以可能导致溢出的情况主要是下面两种：
        写入了bigkey，比如一下子写入了多个百万级别的集合类型数据；
        服务器端处理请求的速度过慢，例如，Redis主线程出现了间歇性阻塞，无法及时处理正常发送的请求，导致客户端发送的请求在缓冲区越积越多。

    如何查看输入缓冲区的内存使用情况，以及如何避免溢出：
        要查看和服务器端相连的每个客户端对输入缓冲区的使用情况，我们可以使用CLIENT LIST命令：
        CLIENT LIST
            cmd，表示客户端最新执行的命令。这个例子中执行的是CLIENT命令。
            qbuf，表示输入缓冲区已经使用的大小。这个例子中的CLIENT命令已使用了26字节大小的缓冲区
            qbuf-free，表示输入缓冲区尚未使用的大小。这个例子中的CLIENT命令还可以使用32742字节的缓冲区。qbuf和qbuf-free的总和就是，
                Redis服务器端当前为已连接的这个客户端分配的缓冲区总大小。这个例子中总共分配了 26 + 32742 = 32768字节，也就是32KB的缓冲区。

        有了CLIENT LIST命令，我们就可以通过输出结果来判断客户端输入缓冲区的内存占用情况了。如果qbuf很大，而同时qbuf-free很小，就要引起注意了，
        因为这时候输入缓冲区已经占用了很多内存，而且没有什么空闲空间了。此时，客户端再写入大量命令的话，就会引起客户端输入缓冲区溢出，
        Redis的处理办法就是把客户端连接关闭，结果就是业务程序无法进行数据存取了。

    我们可以从两个角度去考虑如何避免，一是把缓冲区调大，二是从数据命令的发送和处理速度入手。
        Redis的客户端输入缓冲区大小的上限阈值，在代码中就设定为了1GB。也就是说，Redis服务器端允许为每个客户端最多暂存1GB的命令和数据。
        1GB的大小，对于一般的生产环境已经是比较合适的了。
        Redis并没有提供参数让我们调节客户端输入缓冲区的大小。如果要避免输入缓冲区溢出，那我们就只能从数据命令的发送和处理速度入手，
        也就是前面提到的避免客户端写入bigkey，以及避免Redis主线程阻塞

      如何应对输出缓冲区溢出？
        Redis的输出缓冲区暂存的是Redis主线程要返回给客户端的数据，一般来说，主线程返回给客户端的数据，既有简单且大小固定的OK响应
        （例如，执行SET命令）或报错信息，也有大小不固定的、包含具体数据的执行结果（例如，执行HGET命令）。

        因此，Redis为每个客户端设置的输出缓冲区也包括两部分：一部分，是一个大小为16KB的固定缓冲空间，用来暂存OK响应和出错信息；
        另一部分，是一个可以动态增加的缓冲空间，用来暂存大小可变的响应结果。
        那什么情况下会发生输出缓冲区溢出呢？ 我为你总结了三种：
            服务器端返回bigkey的大量结果；
            执行了MONITOR命令；
                MONITOR命令是用来监测Redis执行的。执行这个命令之后，就会持续输出监测到的各个命令操作，：
                MONITOR命令主要用在调试环境中，不要在线上生产环境中持续使用MONITOR
            缓冲区大小设置得不合理。
                我们可以通过client-output-buffer-limit配置项，来设置缓冲区的大小。
                设置缓冲区大小的上限阈值；
                设置输出缓冲区持续写入数据的数量上限阈值，和持续写入数据的时间的上限阈值。



23.  旁路缓存
    工作原理， 替换策略， 异常处理， 扩展机制

    工作原理：
        计算机默认有两种缓存， cpu末级缓存，LLC,用来缓存内存中数据。  内存中的高速页缓存，用来缓存磁盘中的数据。
        LLC的大小是MB级别，page cache的大小是GB级别，而磁盘的大小是TB级别， 缓存系统的容量大小总是小于后端慢速系统的

    redis缓存处理请求的两种情况。
        把Redis用作缓存时，我们会把Redis部署在数据库的前端, 业务应用在访问数据时，会先查询Redis中是否保存了相应的数据
            -缓存命中，Redis中有相应数据，就直接读取Redis，性能非常快。
            -缓存缺失，Redis中没有保存相应数据，就从后端数据库中读取数据，性能就会变慢。一旦发生缓存缺失，为了让后续请求能从缓存中读取到数据，我们需要把缺失的数据写入Redis，这个过程叫作缓存更新。

        使用Redis缓存时，我们基本有三个操作：
            -应用读取数据时，需要先读取Redis；
            -发生缓存缺失时，需要从数据库读取数据；
            -然后需要更新缓存。

    redis作为旁路缓存的使用操作
        Redis是一个独立的系统软件，和业务应用程序是两个软件 ， 如果应用程序想要使用Redis缓存，我们就要在程序中增加相应的缓存操作代码。所以，我们也把Redis称为旁路缓存。
        读取缓存、读取数据库和更新缓存的操作都需要在应用程序中来完成。

        当应用程序需要读取数据时，我们需要在代码中显式调用Redis的GET操作接口，进行查询；
        如果缓存缺失了，应用程序需要再和数据库连接，从数据库中读取数据；
        当缓存中的数据需要更新时，我们也需要在应用程序中显式地调用SET操作接口，把更新的数据写入缓存。
      缓存类型：
        只读缓存和读写缓存。 只读缓存能加速读请求，而读写缓存可以同时加速读写请求。 读写缓存又有两种数据写回策略，在保证性能和保证数据可靠性之间进行选择

        只读缓存：
            读时调用redis,所有写操作会直接发送给后端数据库，在数据库中增删改。 对于删改的数据来说，如果Redis已经缓存了相应的数据，
            应用需要把这些缓存的数据删除，Redis中就没有这些数据了。 当应用再次读取这些数据时，会发生缓存缺失，应用会把这些数据从数据库中读出来，
            并写到缓存中。
            只读缓存直接在数据库中更新数据的好处是，所有最新的数据都在数据库中，而数据库是提供数据可靠性保障的，这些数据不会有丢失的风险。
            当我们需要缓存图片、短视频这些用户只读的数据时，就可以使用只读缓存这个类型了。

        读写缓存：
            除了读请求会发送到缓存进行处理（直接在缓存中查询数据是否存在)，所有的写请求也会发送到缓存，在缓存中直接对数据进行增删改操作。
            而Redis是内存数据库，一旦出现掉电或宕机，内存中的数据就会丢失。这也就是说，应用的最新数据可能会丢失，给应用业务带来风险。
            我们会有同步直写和异步写回两种策略。其中，同步直写策略优先保证数据可靠性，而异步写回策略优先提供快速响应。
            -同步直写是指，写请求发给缓存的同时，也会发给后端数据库进行处理，等到缓存和数据库都写完数据，才给客户端返回。
            即使缓存宕机或发生故障，最新的数据仍然保存在数据库中，这就提供了数据可靠性保证。
            -异步写回策略，则是优先考虑了响应延迟。此时，所有写请求都先在缓存中处理。等到这些增改的数据要被从缓存中淘汰出来时，
            缓存将它们写回后端数据库
        如果需要对写请求进行加速，我们选择读写缓存；
        如果写请求很少，或者是只需要提升读请求的响应速度的话，我们选择只读缓存。

24. 缓存满替换策略：
        缓存数据的淘汰机制。
        建议把缓存容量设置为总数据量的15%到30%，兼顾访问性能和内存空间开销。CONFIG SET maxmemory 4gb
      
      redis缓存淘汰策略：
        Redis 4.0之前一共实现了6种内存淘汰策略，之后有出现两种
        不进行数据淘汰的策略，只有noeviction这一种。
        会进行淘汰的7种其他策略。
            在设置了过期时间的数据中进行淘汰，包括volatile-random、volatile-ttl、volatile-lru、volatile-lfu（Redis 4.0后新增）四种。    
            在所有数据范围内进行淘汰，包括allkeys-lru、allkeys-random、allkeys-lfu（Redis 4.0后新增）三种。

        默认情况下，Redis在使用的内存空间超过maxmemory值时，并不会淘汰数据，也就是设定的noeviction策略。
        对应到Redis缓存，也就是指，一旦缓存被写满了，再有写请求来时，Redis不再提供服务，而是直接返回错误。
        Redis用作缓存时，实际的数据集通常都是大于缓存容量的 总会有新的数据要写入缓存，这个策略本身不淘汰数据，也就不会腾出新的缓存空间，
        我们不把它用在Redis缓存中

        另外volatile 三种即使缓存没有写满，这些数据如果过期了，也会被删除。

        我们使用EXPIRE命令对一批键值对设置了过期时间后，无论是这些键值对的过期时间是快到了，还是Redis的内存使用量达到了maxmemory阈值，
        Redis都会进一步按照volatile-ttl、volatile-random、volatile-lru、volatile-lfu这四种策略的具体筛选规则进行淘汰。

        -volatile-ttl在筛选时，会针对设置了过期时间的键值对，根据过期时间的先后进行删除，越早过期的越先被删除。
        -volatile-random就像它的名称一样，在设置了过期时间的键值对中，进行随机删除。
        -volatile-lru会使用LRU算法筛选设置了过期时间的键值对。
        -volatile-lfu会使用LFU算法选择设置了过期时间的键值对。

        -allkeys-random策略，从所有键值对中随机选择并删除数据；
        -allkeys-lru策略，使用LRU算法在所有数据中进行筛选。
        -allkeys-lfu策略，使用LFU算法在所有数据中进行筛选。
        如果一个键值对被删除策略选中了，即使它的过期时间还没到，也需要被删除。当然，如果它的过期时间到了但未被策略选中，同样也会被删除。

        Least Recently Used，从名字上就可以看出，这是按照最近最少使用的原则来筛选数据，最不常用的数据会被筛选出来，而最近频繁使用的数据会留在缓存中。

        Redis中LRU算法被做了简化，以减轻数据淘汰对缓存性能的影响 Redis默认会记录每个数据的最近一次访问的时间戳（数据结构RedisObject中的lru字段记录）。 
        Redis在决定淘汰的数据时，第一次会随机选出N个数据，把它们作为一个候选集合 Redis会比较这N个数据的lru字段，把lru字段值最小的数据从缓存中淘汰出去。
        Redis提供了一个配置参数maxmemory-samples，这个参数就是Redis选出的数据个数N  CONFIG SET maxmemory-samples 100



        优先使用allkeys-lru策略。这样，可以充分利用LRU这一经典缓存算法的优势，把最近最常访问的数据留在缓存中，提升应用的访问性能。如果你的业务数据中有明显的冷热数据区分，我建议你使用allkeys-lru策略。
        如果业务应用中的数据访问频率相差不大，没有明显的冷热数据区分，建议使用allkeys-random策略，随机选择淘汰的数据就行。
        如果你的业务中有置顶的需求，比如置顶新闻、置顶视频，那么，可以使用volatile-lru策略，同时不给这些置顶数据设置过期时间。这样一来，这些需要置顶的数据一直不会被删除，而其他数据会在过期时根据LRU规则进行筛选。

    如何处理被淘汰的数据
        一般来说，一旦被淘汰的数据选定后，如果这个数据是干净数据，那么我们就直接删除；如果这个数据是脏数据，我们需要把它写回数据库。
        干净数据和脏数据的区别就在于，和最初从后端数据库里读取时的值相比，有没有被修改过。干净数据一直没有被修改，所以后端数据库里的数据也是最新值。在替换时，它可以被直接删除。
        而脏数据就是曾经被修改过的，已经和后端数据库中保存的数据不一致了。此时，如果不把脏数据写回到数据库中，这个数据的最新值就丢失了，就会影响应用的正常使用。
        这么一来，缓存替换既腾出了缓存空间，用来缓存新的数据，同时，将脏数据写回数据库，也保证了最新数据不会丢失。

        对于Redis来说，它决定了被淘汰的数据后，会把它们删除。即使淘汰的数据是脏数据，Redis也不会把它们写回数据库。所以，我们在使用Redis缓存时，如果数据被修改了，需要在数据修改时就将它写回数据库。
        否则，这个脏数据被淘汰时，会被Redis删除，而数据库里也没有最新的数据了。

25.  缓存与数据库数据不一致
    缓存中的数据和数据库中的不一致；缓存雪崩；缓存击穿和缓存穿透。

    一致性：
        -缓存中有数据，那么，缓存的数据值需要和数据库中的值相同
        -缓存中本身没有数据，那么，数据库中的值必须是最新值

    可以把缓存分成读写缓存和只读缓存：
        同步直写策略：写缓存时，也同步写数据库，缓存和数据库中的数据一致；
        异步写回策略：写缓存时不同步写数据库，等到数据从缓存中淘汰时，再写回数据库。使用这种策略时，如果数据还没有写回数据库，缓存就发生了故障，那么，此时，数据库就没有最新的数据了。

        对于读写缓存来说，要想保证缓存和数据库中的数据一致，就要采用同步直写策略。不过，需要注意的是，如果采用这种策略，就需要同时更新缓存和数据库。所以，我们要在业务应用中使用事务机制，
        来保证缓存和数据库的更新具有原子性，也就是说，两者要不一起更新，要不都不更新，返回错误信息，进行重试。否则，我们就无法实现同步直写。

        对数据一致性的要求可能不是那么高，比如说缓存的是电商商品的非关键属性或者短视频的创建或修改时间等，那么，我们可以使用异步写回策略。


    新增数据：
        数据会直接写到数据库中，不用对缓存做任何操作，此时，缓存中本身就没有新增数据，而数据库中是最新值，这种情况符合我们刚刚所说的一致性的第 2 种情况，所以，此时，缓存和数据库的数据是一致的。
    删改数据：
        如果发生删改，既要操作数据库，也要在缓存中删除数据，这两个操作如果无法保证原子性，也就是说，要不都完成，要不都没完成，此时，就会出现数据不一致问题了。

        我们假设应用先删除缓存，再更新数据库，如果缓存删除成功，但是数据库更新失败，那么，应用再访问数据时，缓存中没有数据，就会发生缓存缺失。然后，应用再访问数据库，但是数据库中的值为旧值，应用就访问到旧值了。

        不同情况：
            先删除缓存值，后更新数据库值：
                数据库更新失败，导致请求再次访问时，发现缓存缺失，再读数据库时，从数据库中读到旧值
            先更新数据库值，后删除缓存值：
                缓存删除失败，导致请求再次访问失败时，读到了之前缓存的旧数据。
        如何解决数据不一致问题：
            可以把要删除的缓存值或者更新数据库的值暂存到消息队列（kafka消息队列）当应用没有能够成功删除缓存值或者是更新数据库值时，可以再从消息队列中读取这些值，然后进行再次删除或更新
            如果能够成功的删除或更新，就把这些值从消息队列中移出去，以免重复操作，此时，可以保证数据库和缓存的一致性，否者还要再次操作，如果重试超过一定次数，还是没有成功，我们就要向业务层发送报错信息。
            
            实际上，即使这两个操作第一次执行时都没有失败，当有大量并发请求时，应用还是有可能读到不一致的数据
        也有两种办法：
          先删除缓存，再更新数据库:
            假设线程 A 删除缓存值后，还没有来得及更新数据库（比如说有网络延迟），线程 B 就开始读取数据了，那么这个时候，线程 B 会发现缓存缺失，
            就只能去数据库读取。这会带来两个问题：
                1.线程 B 读取到了旧值；
                2.线程 B 是在缓存缺失的情况下读取的数据库，所以，它还会把旧值写入缓存，这可能会导致其他线程从缓存中读到旧值。
                等到线程 B 从数据库读取完数据、更新了缓存后，线程 A 才开始更新数据库，此时，缓存中的数据是旧值，而数据库中的是最新值，两者就不一致了。

            在线程 A 更新完数据库值以后，我们可以让它先 sleep 一小段时间，再进行一次缓存删除操作。
                之所以要加上 sleep 的这段时间，就是为了让线程 B 能够先从数据库读取数据，再把缺失的数据写入缓存，然后，线程 A 再进行删除。
                所以，线程 A sleep 的时间，就需要大于线程 B 读取数据再写入缓存的时间。
                在第一次删除缓存值后，延迟一段时间再次进行删除，所以我们也把它叫做“延迟双删”。
        
          先更新数据库，再删除缓存值。
            如果线程 A 删除了数据库中的值，但还没来得及删除缓存值，线程 B 就开始读取数据了，那么此时，线程 B 查询缓存时，发现缓存命中，
            就会直接从缓存中读取旧值。不过，在这种情况下，如果其他线程并发读缓存的请求不多，那么，就不会有很多请求读取到旧值。
            而且，线程 A 一般也会很快删除缓存值，这样一来，其他线程再次读取时，就会发生缓存缺失，进而从数据库中读取最新值。
            所以，这种情况对业务的影响较小

    小结：
        删除缓存值或更新数据库失败而导致数据不一致，你可以使用重试机制确保删除或更新操作成功。在删除缓存值、更新数据库的这两步操作中，
        有其他线程的并发读操作，导致其他线程读取到旧值，应对方案是延迟双删。

    在大多数业务场景下，我们会把 Redis 作为只读缓存使用。针对只读缓存来说，我们既可以先删除缓存值再更新数据库，也可以先更新数据库再删除缓存。
    我的建议是，优先使用先更新数据库再删除缓存的方法，原因主要有两个：
        先删除缓存值再更新数据库，有可能导致请求因缓存缺失而访问数据库，给数据库带来压力；
        如果业务应用中读取数据库和写缓存的时间不好估算，那么，延迟双删中的等待时间就不好设置。

        当使用先更新数据库再删除缓存时，也有个地方需要注意，如果业务层要求必须读取一致的数据，那么，我们就需要在更新数据库时，
        先在 Redis 缓存客户端暂存并发读请求，等数据库更新完、缓存值删除后，再读取数据，从而保证数据一致性。

26.  缓存雪崩 缓存击穿 缓存穿透
    这三个问题一旦发生，会导致大量的请求积压到数据库，如果并发量太高，会导致数据库宕机。
    -缓存雪崩
        大量的请求在redis中没有找到对应缓存，然后发送到数据库，导致数据库层压力激增。
      原因：
        1.大量数据同时过期，导致大量请求无法得到处理
            解决方案：  我们可以避免给大量的数据设置相同的过期时间，EXPIRE 命令给每个数据设置过期时间时，给这些数据的过期时间增加一个较小的随机数
                      也可以通过服务降级。
        2.Redis 缓存实例发生故障宕机了
            在业务系统中实现服务熔断或请求限流机制
            主从切换。
    -缓存击穿
        缓存击穿是指，针对某个访问非常频繁的热点数据的请求，无法在缓存中进行处理，紧接着，访问该数据的大量请求，一下子都发送到了后端数据库，
        导致了数据库压力激增，会影响数据库处理其他请求。
      原因：
        缓存击穿经常发生在热点数据过期失效的时候。
            -对于访问特别频繁的热点数据，我们就不设置过期时间了
    -缓存穿透：
        缓存穿透是指数据即不在redis 也不在数据库，导致请求在访问缓存时，发生缓存缺失，再去访问数据库，发现数据库也没有访问的数据，缓存成了摆设。
            业务层误操作，缓存和数据库中的数据被删除了， 所以缓存和数据库中都没有数据。
            恶意攻击：专门访问数据库中没有的数据
        -专门缓存空值或缺省值
        -使用布隆过滤器，快速判断数据是否纯在，避免从数据库中查询数据是否存在，减轻数据库压力。
        -前端进行请求检测。

27. 缓存被污染了
    有些数据被访问的次数非常少，甚至只会被访问一次。当这些数据服务完访问请求后，如果还继续留存在缓存中的话，就只会白白占用缓存空间。这种情况，就是缓存污染。
    LFU

39. redis 6.0特性
    -面向网络处理的多 IO 线程、
    -客户端缓存、
    -细粒度的权限控制，
    -以及 RESP 3 协议的使用。
    
    面向网络处理的多IO线程可以提高网络请求处理的速度，客户端缓存可以让应用直接在客户端本地读取数据，这两个特性可以提升 Redis 的性能。
    除此之外，细粒度权限控制让 Redis 可以按照命令粒度控制不同用户的访问权限，加强了 Redis 的安全保护。
    RESP 3 协议则增强客户端的功能，可以让应用更加方便地使用 Redis 的不同数据类型

    Redis 一直被大家熟知的就是它的单线程架构，虽然有些命令操作可以用后台线程或子进程执行（比如数据删除、快照生成、AOF 重写）
    从网络 IO 处理到实际的读 写命令处理，都是由单个线程完成的。
    单个主线程处理网络请求的速度跟不上底层网络硬件的速度。
    一般有两种方法：
        用用户态网络协议栈（例如 DPDK）取代内核网络协议栈，让网络请求的处理不用在内核里执行，直接在用户态完成处理就行。
        对于高性能的 Redis 来说，避免频繁让内核进行网络请求处理，可以很好地提升请求处理效率。但是，这个方法要求在 Redis 的整体架构中，
        添加对用户态网络协议栈的支持，需要修改 Redis 源码中和网络相关的部分（例如修改所有的网络收发请求函数），
        这会带来很多开发工作量。而且新增代码还可能引入新 Bug，导致系统不稳定。所以，Redis 6.0 中并没有采用这个方法。
    第二种：
        多个 IO 线程来处理网络请求，提高网络请求处理的并行度。Redis 6.0 就是采用的这种方法。
        但是，Redis 的多 IO 线程只是用来处理网络请求的，对于读写命令，Redis 仍然使用单线程来处理。Redis 处理请求时，网络处理经常是瓶颈，
        通过多个 IO 线程并行处理网络操作，可以提升实例的整体处理性能。而继续使用单线程执行命令操作，就不用为了保证 Lua 脚本、事务的原子性，
        额外开发多线程互斥机制了。这样一来，Redis 线程模型实现就简单了。

        -：服务端和客户端建立 Socket 连接，并分配处理线程。
           主线程负责接收建立连接请求。当有客户端请求和实例建立 Socket 连接时，主线程会创建和客户端的连接，
           并把 Socket 放入全局等待队列中。紧接着，主线程通过轮询方法把 Socket 连接分配给 IO 线程。
        -： IO 线程读取并解析请求
           主线程一旦把 Socket 分配给 IO 线程，就会进入阻塞状态，等待 IO 线程完成客户端请求读取和解析。
           因为有多个 IO 线程在并行处理，所以，这个过程很快就可以完成。
        -： 主线程执行请求操作
            等到 IO 线程解析完请求，主线程还是会以单线程的方式执行这些命令操作。下面这张图显示了刚才介绍的这三个阶段，你可以看下，加深理解。  
        -： IO 线程回写 Socket 和主线程清空全局队列
            当主线程执行完请求操作后，会把需要返回的结果写入缓冲区，然后，主线程会阻塞等待 IO 线程把这些结果回写到 Socket 中，并返回给客户端。
            和 IO 线程读取和解析请求一样，IO 线程回写 Socket 时，也是有多个线程在并发执行，所以回写 Socket 的速度也很快。
            等到 IO 线程回写 Socket 完毕，主线程会清空全局队列，等待客户端的后续请求。
            

40.
    新型非易失存储（Non-Volatile Memory，NVM） NVM器件具有容量大、性能快、能持久化保存数据的特性.
    可以让软件以字节粒度进行寻址访问，所以，在实际应用中，NVM可以作为内存来使用，我们称为NVM内存。
    在redis上运用： 大容量，快速持久化数据和恢复。

    Redis是基于DRAM内存的键值数据库，而跟传统的DRAM内存相比，NVM有三个显著的特点。
      -NVM内存最大的优势是可以直接持久化保存数据, 数据保存在NVM内存上后，即使发生了宕机或是掉电，数据仍然存在NVM内存上.
      -NVM内存的访问速度接近DRAM的速度
      -NVM内存的容量很大
    Intel在2019年4月份时推出的Optane AEP内存条（简称AEP内存）
    两种模式：
      Memory模式下
        这种模式是把NVM内存作为大容量内存来使用的，也就是说，只使用NVM容量大和性能高的特性，没有启用数据持久化的功能。 
        在Memory模式下，服务器上仍然需要配置DRAM内存，但是，DRAM内存是被CPU用作AEP内存的缓存，DRAM的空间对应用软件不可见。
        换句话说，软件系统能使用到的内存空间，就是AEP内存条的空间容量。
      App Direct模式
        这种模式启用了NVM持久化数据的功能。在这种模式下，应用软件把数据写到AEP内存上时，数据就直接持久化保存下来了。
        所以，使用了App Direct模式的AEP内存，也叫做持久化内存（Persistent Memory PM）。
    基于NVM内存的redis实践
        当AEP内存使用Memory模式时，应用软件就可以利用它的大容量特性来保存大量数据，Redis也就可以给上层业务应用提供大容量的实例。
        Redis可以像在DRAM内存上运行一样，直接在AEP内存上运行，不用修改代码。 在Memory模式下，AEP内存的访问延迟会比DRAM高一点
        NVM的读延迟大约是200~300ns，而写延迟大约是100ns。
       
       无论是RDB还是AOF，都需要把数据或命令操作以文件的形式写到硬盘上。对于RDB来说，虽然Redis实例可以通过子进程生成RDB文件，
       但是，实例主线程fork子进程时， 仍然会阻塞主线程。而且，RDB文件的生成需要经过文件系统，文件本身会有一定的操作开销。
       现在Redis在涉及持久化操作时的问题：
            -RDB文件创建时的fork操作会阻塞主线程；
            -AOF文件记录日志时，需要在数据可靠性和写性能之间取得平衡；
            -使用RDB或AOF恢复数据时，恢复效率受RDB和AOF大小的限制。
        但是，如果我们使用持久化内存，就可以充分利用PM快速持久化的特点，来避免RDB和AOF的操作。因为PM支持内存访问，而Redis的操作都是内存操作，
        那么，我们就可以把Redis直接运行在PM上。同时，数据本身就可以在PM上持久化保存了，我们就不再需要额外的RDB或AOF日志机制来保证数据可靠性了。

        如何使用PM:
            /dev/pmem0
             mkfs.ext4 /dev/pmem0
             mount -o dax /dev/pmem0  /mnt/pmem0
             创建好了以后，再把这些文件通过内存映射（mmap）的方式映射到Redis的进程空间 我们就可以把Redis接收到的数据直接保存到映射的内存空间上了，
             而这块内存空间是由PM提供的。所以，数据写入这块空间时，就可以直接被持久化保存了。如果要修改或删除数据，PM本身也支持以字节粒度进行数据访问，
             所以，Redis可以直接在PM上修改或删除数据。
             如果使用PM来运行Redis，需要评估下PM提供的访问延迟和访问带宽，是否能满足业务层的需求。

