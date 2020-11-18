/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hama/Pipes.hh"
#include "hama/TemplateFactory.hh"
#include "hadoop/StringUtils.hh"
#include "DenseDoubleVector.hh"
#include <time.h>
#include <math.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map; 
using HamaPipes::BSP;
using HamaPipes::BSPJob;
using HamaPipes::Partitioner;
using HamaPipes::BSPContext;
//using HadoopUtils::Splitter;
using math::DenseDoubleVector;
int INF = 0x7fffffff;
class SSSPTRYBSP : public BSP<int, string, int, double, string> {
 private:
	 int source_id;//单源头最短路径起点默认0,目前没想到好办法改,变成那种能指定输入的
	 int numpeers;//总节点数
	 int id_peer;//节点id
	 string master_task_;
	 int num_peers;
	 vector<string> peernamelist;//初始化节点列表
	 map<int,double> d;//距离
	 map<int, vector<double> > ms;//临边 原本叫做ms  存储了权重 
	 map<int, int> msflag;//更新标记(更新了再通过ms传递消息)
public:
	SSSPTRYBSP(BSPContext<int, string, int, double, string>& context) {
		source_id = 0;//默认源节点id是0
	}
	void setup(BSPContext<int, string, int, double , string>& context) {
		master_task_ = context.getPeerName(context.getNumPeers() / 2);
		num_peers = context.getNumPeers();
		//初始化peernamelist列表
		peernamelist = context.getAllPeerNames();
		//初始化节点id
		id_peer=context.getPeerIndex();
		//初始化总节点数
		numpeers = context.getNumPeers();
	}

	void bsp(BSPContext<int, string, int, double, string>& context) {
		// return;
		int v_id;//顶点值 和对应的权重 
		string e_list;
		while (context.readNext(v_id, e_list)) {
			if (v_id == source_id) {//这个点的初始化距离 
				d[v_id] = 0;
			}else{
				d[v_id] = INF;//初始化距离
			}  
			DenseDoubleVector * row_vector = new DenseDoubleVector(e_list);
			int size = row_vector->getLength();
			for(int i=0;i<size;i++){//对称矩阵的话 就是0 要和1 2 3 ；1的话和 2 3
				if(i<=v_id){
					ms[v_id].push_back(INF);
				}else{
					ms[v_id].push_back(row_vector->get(i));
				}
			}
			msflag[v_id] = 0;//初始化信息flag,只有当msflag[id]!=0的时候才向id节点的临近节点发送信息
		}
		context.sync();
		if (ms.find(source_id) != ms.end()) {//找到源节点就开始第一轮发送信息
			int cnt = 0;//这个是顶点的id 
			for (auto i : ms[source_id]) {
				if(i != INF){//如果说有边的话才会发送发送消息 
					std::stringstream message;
					message <<":"<< cnt << ":" << HadoopUtils::toString((d[source_id] + i));
					context.sendMessage(peernamelist[cnt%numpeers], message.str());//消息i为目标节点id,d[]+1为距离
				}
				cnt++;
			}
		}
		context.sync();
		bool is_goto_cleanup = false;
		while (true) {
			int msg_count = context.getNumCurrentMessages();
			//接收消息 处理消息 发送消息 
			for (int ii = 0; ii < msg_count; ii++) {
				string received = context.getCurrentMessage();//读消息列表消息
				string key_value_str = received.substr(1);
				int pos = (int)key_value_str.find(received.substr(0,1));
				int first = HadoopUtils::toInt(key_value_str.substr(0,pos));
				int second = HadoopUtils::toInt(key_value_str.substr(pos+1));
				if (second < d[first]) {//更新节点状态
					d[first] = second;//更新目标节点距离源节点的距离
					msflag[first] = 1;//表示节点信息更新需要发送信息
				}
			}
			context.sync();

			bool is_all_invalid = true;
			for (auto it=msflag.begin();it!=msflag.end();it++) {//遍历消息flag,找到所有需要发送信息的id
				if (it->second != 0) {//如果说状态改变了 
					is_all_invalid = false;
					break;
				}
			}
			std::stringstream message;
			message << ":" << -1 << ":" << (is_all_invalid ? "true" : "false");
			context.sendMessage(master_task_, message.str());

			context.sync();
			if (context.getPeerName().compare(master_task_)==0) {
				int invalid_peer_nums = 0;
				int msg_count = context.getNumCurrentMessages();
				for (int ii = 0; ii < msg_count; ii++) {
					string received = context.getCurrentMessage();//读消息列表消息
					string key_value_str = received.substr(1);
					int pos = (int)key_value_str.find(received.substr(0,1));
					int first = HadoopUtils::toInt(key_value_str.substr(0,pos));
					string second = key_value_str.substr(pos+1);
					if (first == -1 && second == "true") {
						invalid_peer_nums++;
					}
				}
				for(string peer_name: peernamelist) {
					std::stringstream message;
					message <<":"<< -2 << ":" << (invalid_peer_nums == num_peers ? "release" : "keep");
					context.sendMessage(peer_name, message.str());
				}
			}
			context.sync();
			msg_count = context.getNumCurrentMessages();
			for (int ii = 0; ii < msg_count; ii++) {
				string received = context.getCurrentMessage();//读消息列表消息
				string key_value_str = received.substr(1);
				int pos = (int)key_value_str.find(received.substr(0,1));
				int first = HadoopUtils::toInt(key_value_str.substr(0,pos));
				string second = key_value_str.substr(pos+1);
				if (first == -2 && second == "release") {
					is_goto_cleanup = true;
					break;
				}
			}
			if (is_goto_cleanup) {
				return;
			}
			context.sync();
			for (auto it=msflag.begin();it!=msflag.end();it++) {//遍历消息flag,找到所有需要发送信息的id
				if (it->second != 0) {//如果说状态改变了 
					int cnt = 0; 
					for (auto ii : ms[it->first]) {
						if(ii != INF){
							std::stringstream message;
							message <<":"<< cnt<< ":" << HadoopUtils::toString(d[it->first] + ii);
							context.sendMessage(peernamelist[cnt%numpeers], message.str());
						} 
						cnt++; 
					}
					it->second = 0;//重置flag
				}
			}
			context.sync();
		}
	}
	void cleanup(BSPContext<int, string, int, double, string>& context) {
		for (auto i:d ) {
			context.write(i.first, i.second);
		}
	}


};
class MatrixRowPartitioner : public Partitioner<int, string> {
public:
	MatrixRowPartitioner(BSPContext<int, string, int, double, string>& context) { }

	int partition(const int& key, const string& value, int32_t num_tasks) {
		return key % num_tasks;
	}
};

int main(int argc, char *argv[]) {
	return HamaPipes::runTask<int, string, int, double, string>(HamaPipes::TemplateFactory<SSSPTRYBSP, int, string, int, double, string, MatrixRowPartitioner>());
}