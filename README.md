## 基于BSP模型的并行图处理算法实现

### 一、运行环境

OS：Ubuntu 20.04.1 LTS

JDK：1.8

Maven：3.6.3

Hadoop：2.10.1

Hama：0.7.1

CMake：3.16.3

### 二、环境的安装与配置

**除了OS外，其它可在链接: https://pan.baidu.com/s/1Ci-EQJn4ZDJUAXJPJrPSOg 密码: 916u 下载**

#### 2.1 OS的安装

​		使用VMware Workstation安装Ubuntu系统即可。

#### 2.2 Java SE Development Kit 8 的安装

将下载的`jdk-8u11-linux-x64.tar.gz`解压，然后将解压的结果移动到`/usr/local`目录下，最后配置环境变量，启动控制台，查看环境变量是否有效。

```bash
# 解压
tar -zxvf jdk-8u11-linux-x64.tar.gz
# 移动至\usr\local
sudo mv jdk1.8.0_171  /usr/local/jdk1.8
```

**设置环境变量**：

```bash
vim ~/.bashrc
```

再尾部加入：

```bash
export JAVA_HOME=/usr/local/jdk1.8
export JRE_HOME=${JAVA_HOME}/jre
export CLASSPATH=.:${JAVA_HOME}/lib:${JRE_HOME}/lib
export PATH=.:${JAVA_HOME}/bin:$PATH
```

刷新环境变量：

```
source ~/.bashrc
```

验证是否安装成功：

```bash
java -version
```

结果：

```bash
java version "1.8.0_11"
Java(TM) SE Runtime Environment (build 1.8.0_11-b12)
Java HotSpot(TM) 64-Bit Server VM (build 25.11-b03, mixed mode)
```

#### 2.3 Apach Maven的安装与配置

将下载的`apache-maven-3.6.3-bin.tar.gz`解压，然后将解压的结果移动到`/usr/local`目录下，最后配置环境变量，启动控制台，查看环境变量是否有效。

```bash
# 解压
tar -zxvf apache-maven-3.6.3-bin.tar.gz
# 移动至\usr\local
sudo mv apache-maven-3.6.3  /usr/local/apache-maven-3.6.3
```

**设置环境变量**：

```bash
vim ~/.bashrc
```

再尾部加入：

```bash
MAVEN_HOME=/usr/local/maven
PATH=${PATH}:${MAVEN_HOME}/bin
```

刷新环境变量：

```
source ~/.bashrc
```

验证是否安装成功：

```bash
mvn -version
```

结果：

```bash
Apache Maven 3.6.3 (cecedd343002696d0abb50b32b541b8a6ba2883f)
Maven home: /usr/local/maven
Java version: 1.8.0_11, vendor: Oracle Corporation, runtime: /usr/local/jdk1.8/jre
Default locale: en_US, platform encoding: UTF-8
OS name: "linux", version: "5.4.0-53-generic", arch: "amd64", family: "unix"
```

#### 2.4 Hadoop的安装与配置

将下载的`hadoop-2.10.1.tar.gz`解压，然后将解压的结果移动到`/usr/local`目录下，最后配置环境变量，启动控制台，查看环境变量是否有效。

```bash
# 解压
tar -zxvf hadoop-2.10.1.tar.gz
# 移动至\usr\local
sudo mv hadoop-2.10.1  /usr/local/hadoop-2.10.1
```

**设置环境变量**：

```bash
vim ~/.bashrc
```

再尾部加入：

```bash
export HADOOP_HOME=/usr/local/hadoop-2.10.1
export PATH=.:${JAVA_HOME}/bin:${HADOOP_HOME}/bin:$PATH
```

刷新环境变量：

```
source ~/.bashrc
```

验证是否安装成功：

```bash
hadoop version
```

结果：

```bash
Hadoop 2.10.1
Subversion https://github.com/apache/hadoop -r 1827467c9a56f133025f28557bfc2c562d78e816
Compiled by centos on 2020-09-14T13:17Z
Compiled with protoc 2.5.0
From source with checksum 3114edef868f1f3824e7d0f68be03650
This command was run using /usr/local/hadoop-2.10.1/share/hadoop/common/hadoop-common-2.10.1.jar
```

**hadoop伪分布式配置**

伪分布式只需要更改两个文件就够了，`core-site.xml`和`hdfs-site.xml`。这两个文件都在hadoop目录下的`etc/hadoop`中。

```bash
cd /usr/local/hadoop-2.10.1/etc/hadoop
```

首先是`core-site.xml`，设置临时目录位置，否则默认会在`/tmp/hadoo-hadoop`中，这个文件夹在重启时可能被系统清除掉，所以需要改变配置路径。修改<configuration> </configuration>

```xml
<configuration>
        <property>
             <name>hadoop.tmp.dir</name>
             <value>file:/usr/local/hadoop-2.10.1/tmp</value>
             <description>Abase for other temporary directories.</description>
        </property>
        <property>
             <name>fs.defaultFS</name>
             <value>hdfs://localhost:9000</value>
        </property>
</configuration>
```

然后就是`hdfs-site.xml`，伪分布式只有一个节点，所以必须配置成1。还配置了datanode和namenode的节点位置。

```xml
<configuration>
        <property>
             <name>dfs.replication</name>
             <value>1</value>
        </property>
        <property>
             <name>dfs.namenode.name.dir</name>
             <value>file:/usr/local/hadoop-2.10.1/tmp/dfs/name</value>
        </property>
        <property>
             <name>dfs.datanode.data.dir</name>
             <value>file:/usr/local/hadoop-2.10.1/tmp/dfs/data</value>
        </property>
</configuration>
```

配置完成后在`/usr/local/hadoop-2.10.1` (注意是自己的hadoop目录) 下使用以下命令 执行format命令，格式化名称节点。

```bash
./bin/hdfs namenode -format 
```

![image-20201119212418131](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119212418131.png)

最后一步是设置Java环境的路径，在`/usr/local/hadoop-2.10.1/etc/hadoop`目录下，编辑`hadoop-env.sh`。

```bash
# sudo gedit hdoop-env.sh

# 将语句
export JAVA_HOME=$JAVA_HOME  
# 也有可能语句为export JAVA_HOME=   （且被注释掉了）

# 修改为
export JAVA_HOME=/usr/lib/jvm/java-8-oracle  # 自己的Java home路径，可以在终端输入$JAVA_HOME 查看

# 保存后退出，重新执行./sbin/start-dfs.sh
```

**开启hdfs并验证是否打开：**

```bash
./sbin/start-dfs.sh
jps
```

结果：

```bash
Starting namenodes on [localhost]
localhost: starting namenode, logging to /usr/local/hadoop-2.10.1/logs/hadoop-idestro-namenode-ubuntu.out
localhost: starting datanode, logging to /usr/local/hadoop-2.10.1/logs/hadoop-idestro-datanode-ubuntu.out
Starting secondary namenodes [0.0.0.0]
0.0.0.0: starting secondarynamenode, logging to /usr/local/hadoop-2.10.1/logs/hadoop-idestro-secondarynamenode-ubuntu.out
100421 DataNode
100229 NameNode
100649 SecondaryNameNode
101117 Jps
```

打开http://localhost:50070/：

![image-20201119213748440](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119213748440.png)

#### 2.5 Hama的安装与配置

将下载的`hama-dist-0.7.1.tar.gz`解压，然后将解压的结果移动到`/usr/local`目录下，最后配置环境变量，启动控制台，查看环境变量是否有效。

```bash
# 解压
tar -zxvf hama-dist-0.7.1.tar.gz
# 移动至\usr\local
sudo mv hama-dist-0.7.1  /usr/local/hama-dist-0.7.1
```

**修改配置文件：**

1）hama-site.xml内容如下，每台机器具有相同的配置：（因集群中并没有单独的zookeeper，就没有对其进行设定）

```xml
<?xml version="1.0"?> 
<?xml-stylesheet type="text/xsl" href="configuration.xsl"?> 
<configuration> 
    <property> 
        <name>bsp.master.address</name> 
        <value>dm4:40000</value> 
        <description>The address of the bsp master server. Either the 
        literal string "local" or a host:port for distributed mode 
        </description> 
    </property> 
    <property> 
        <name>fs.default.name</name> 
        <value>hdfs://dm4:9000/</value> 
        <description> 
        The name of the default file system. Either the literal string 
        "local" or a host:port for HDFS.不建议填写为“dm4:9000/”
        </description> 
    </property> 
    <property> 
    <name>hama.zookeeper.quorum</name> 
        <value>dm4</value> 
        <description>Comma separated list of servers in the ZooKeeper Quorum. 
        For example, "host1.mydomain.com,host2.mydomain.com,host3.mydomain.com". 
        By default this is set to localhost for local and pseudo-distributed modes 
        of operation. For a fully-distributed setup, this should be set to a full 
        list of ZooKeeper quorum servers. If HAMA_MANAGES_ZK is set in hama-env.sh 
        this is the list of servers which we will start/stop zookeeper on. 
        </description> 
    </property>
</configuration>
```

2）在hama-env.sh文件中加入JAVA_HOME变量，由于每台机器jdk的位置不同，各台机器要单独配置

**启动hama**

```bash
./bin/start-bspd.sh
```

验证是否成功启动：

```bash
jps
```

结果：

```bash
101280 ZooKeeperRunner
100421 DataNode
100229 NameNode
102100 Jps
101476 GroomServerRunner
101335 BSPMasterRunner
100649 SecondaryNameNode
```

访问：http://localhost:40013

![image-20201119215022138](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119215022138.png)

#### 2.6 CMake的安装

将下载的`make-3.81.tar.gz`解压，然后将解压的结果移动到`/usr/local`目录下，最后配置环境变量，启动控制台，查看环境变量是否有效。

```bash
# 解压
tar -zxvf make-3.81.tar.gz
# 进入make-3.81.tar.gz
cd make-3.81.tar.gz
# 解决缺库问题
sudo apt-get install libssl-dev
# 更新g++
sudo apt-get install g++
./bootstrap
# 编译构建
make
# 安装
sudo make install
```

**验证：**

```bash
cmake --version
```

**结果：**

```bash
cmake version 3.16.3

CMake suite maintained and supported by Kitware (kitware.com/cmake).
```

### 三、并行最短路算法的构造

![image-20201119223328682](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119223328682.png)

其中大圆代表每个peer节点，小圆代表被分配的图节点。橙色的peer节点指master节点，绿色的小圆代表了图中的source节点。其中左侧蓝色部分是初始化过程，右侧是一个循环的过程，loop指代了循环的轮次。

|    Super Step    |                            Action                            |
| :--------------: | :----------------------------------------------------------: |
|        0         | 根据读入的文件，每个peer按照切片获得的图节点信息，初始化各自的距离，用map进行存储，初始化所有图节点状态为非激活状态 |
|        1         |                   发送信息给source的邻节点                   |
| $5\times loop+2$ | 每个peer开始收集信息，根据信息判断能否产生松弛，如果能松弛，改变图节点状态为激活状态 |
| $5\times loop+3$ | 每个peer判断所有图节点是否为非激活状态，如果是，向master节点发送true的信息 |
| $5\times loop+4$ | 该超步其实仅有master节点在执行，master节点根据接收的消息判断图的节点是否为非激活状态，如果是，发送"release"给所有peer，否则发送"keep"给所有图节点 |
| $5\times loop+5$ | 所有peer接收信息，如果信息为"release"，则直接进去cleanup函数，否则继续执行下一步超步。 |
| $5\times loop+6$ | 每个peer判断每个图节点的状态，如果是激活状态，发送距离信息给它的邻节点，并重置状态为非激活 |

### 四、实验结果

```
cd zju-cpp-final
./run.sh
```

随机产生的图为：

![image-20201119230042575](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119230042575.png)

结果为：

![image-20201119230109903](https://typaro.oss-cn-beijing.aliyuncs.com/images/image-20201119230109903.png)

经核对，是正确结果。

### 四、总结

经过几天的不懈努力，在未知领域中不断探索，看源码，debug调试...终于解决了这个问题，虽然可能这个解决方案并不是最好的，但从中学到的知识可不是这么一个结果就能衡量的。