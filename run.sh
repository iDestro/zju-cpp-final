cd c++
mvn install
cd ..
echo -e "\033[32m delete exist files... \033[0m"
hadoop fs -rm /examples/bin/single_shortest_path
hadoop fs -rm /examples/input/single_shortest_path/graph.seq
hama jar hama-examples-*.jar gen vectorwritablematrix 10 10 /examples/input/single_shortest_path/graph.seq false true 1 6 0
echo -e "\033[32m generate graph... \033[0m"
hama seqdumper -file /examples/input/single_shortest_path/graph.seq
hadoop fs -put c++/target/native/examples/single_shortest_path /examples/bin/single_shortest_path
hama pipes -conf c++/src/main/native/examples/conf/single_shortest_path.xml -output /examples/output/single_shortest_path
echo -e "\033[32m single shortest path output: \033[0m"
hama seqdumper -file /examples/output/single_shortest_path/part-00000
hama seqdumper -file /examples/output/single_shortest_path/part-00001
hama seqdumper -file /examples/output/single_shortest_path/part-00002
