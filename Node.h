//
// Created by 贾明卓 on 2024/4/12.
//

#ifndef SKIPLIST_NODE_H
#define SKIPLIST_NODE_H
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

using namespace std;
#define STORE_FILE "store/dumpFile" //存储文件路径
std::mutex mtx; //互斥锁
std::string delimiter =":";
template<typename K, typename V>
class Node{
private:
    K key; //键
    V value; //值

public:
    int node_level; //节点所在层
    Node<K, V>** forward; //指向下一个节点的指针数组
    Node(K k, V v, int level) : key(k), value(v), node_level(level){
        this->forward = new Node<K, V> *[level + 1];
        memset(this->forward, 0, sizeof(Node<K, V> *) * (level + 1));
    }

    ~Node(){
        delete[] forward;
    }

    K get_key() const{
        return key;
    }

    V get_value() const{
        return value;
    }

    void set_value(V v){
        this->value = v;
    }
};


template<typename K, typename V>
class SkipList{
public:

    SkipList(int max_level) : max_level(max_level){
        this->element_count = 0; //当前节点
        this->skip_level = 0; //当前层
        K k; //默认键
        V v; //默认值
        //创建头节点
        this->header = new Node<K, V>(k, v, max_level);
    }

    ~SkipList(){
        delete header;
    }

    int get_random_level(); //获取节点的随即层数

    Node<K, V>* create_node(K, V, int level); // 节点创建

    void display_list(); //展示节点

    bool search_element(K key); //查询节点

    int insert_element(const K key, const V v);

    void delete_element(K key); //删除节点

    void dump_file(); //持久化数据到文件

    void load_file(); //从文件加载数据

    void clear(Node<K, V>*); //递归删除节点

    int size(); // 返回跳表的节点数

private:
    void get_key_value_from_string(const std::string &str, std::string *key, std::string *value);
    bool is_valid_string(const std::string &str);

private:
    int max_level; //跳表的最大层数
    int skip_level; //跳表的当前层数
    Node<K, V>* header; //跳表的头节点
    int element_count; //跳表的节点总数
    std::ofstream file_writer; //文件写入器
    std::ofstream file_reader; //文件读取器

};
template<typename K, typename  V>
int SkipList<K, V>::get_random_level() {
    //初始化层级；
    int k = 1;
    //使用随机过程增加层级
    while(rand() % 2){
        k++;
    }
    k =(k < this->max_level) ? k : this->max_level;
    return k;
}

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(K k, V v, int level){
    Node<K, V>* node = new Node<K, V>(k, v, level);
    return node;
}
template<typename K, typename V>
bool SkipList<K, V>:: search_element(K key){
    //定义一个指针，初始化位跳表的头节点
    Node<K, V>* cur = header;
    //从最高层级开始搜索
    //当前节点的下一个节点的键值大于带查找的键值时，进行下沉到下一层
    for(int i = this->skip_level; i >= 0; --i){
        //遍历当前层级，直到下一个节点的键值大于等于查找的键值
        while(cur->forward[i] && cur->forward[i]->get_key() < key) {
            cur = cur->forward[i];
        }
    }
    //检查当前层（最底层）的下一个节点的值是否为带查找的元素
    cur = cur->forward[0];
    if(cur && cur->get_key() == key) {
        return true;
    }

    return false;
}

template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    mtx.lock();
    Node<K, V>* cur = this->header;
    Node<K, V>* update[max_level + 1]; // 用于记录每一层中待更新指针的节点
    memset(update, 0, sizeof(Node<K, V>*) * (max_level + 1));

    // 从高向下搜索插入位置
    for(int i = skip_level; i >= 0; --i){
        // 寻找当前链表中最接近且小于 key 的节点
        while(cur->forward[i] && cur->forward[i]->get_key() < key){
            cur = cur->forward[i];
        }
        // 保存每层中该节点，以便后续插入时更新指针
        update[i] = cur;
    }

    // 移动到最底层节点，准备插入
    cur = cur->forward[0];

    //检查代插入节点是否已存在
    if(cur != nullptr && cur->get_key() == key){
        //节点已存在，取消插入
        cout << " key: " << ", exists" << endl;
        mtx.unlock();
        return 1;
    }

    //待插入的键不存在于跳表中
    if(cur == nullptr || cur->get_key() != key){
        int random_level = get_random_level();
        //新节点的层级超过跳表的当前最高层级
        if(random_level > skip_level){
            //对所有新的更高层级，将头节点设置为它们的前驱节点
            for(int i = skip_level + 1 ; i < random_level + 1; ++i){
                update[i] = header;
            }
            skip_level = random_level;
        }
        Node<K, V> *node = create_node(key, value, random_level);

        //在各层插入新节点
        for(int i = 0; i <= random_level; ++i){
            //新节点指向当前节点的下一个节点
            node->forward[i] = update[i]->forward[i];
            //当前节点的下一个节点更新为新节点
            update[i]->forward[i] = node;
        }
        this->element_count++;
    }

    mtx.unlock();
    return 0;
}


/*
 * 1.定位待删除的节点
 * 2.更新指针关系
 * 3.内存回收
 */
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
    mtx.lock();
    Node<K, V> *cur = this->header;
    Node<K, V> *update[max_level + 1];
    memset(update, 0, sizeof(Node<K, V>*) * (max_level + 1));

    //自顶向下搜索待删除的节点
    for(int i = skip_level; i >= 0; --i){
        //找到要删除节点的前驱节点
        while(cur->forward[i] && cur->forward[i]->get_key() < key){
            cur = cur->forward[i];
        }
        update[i] = cur; //记录删除节点的前驱
    }

    cur = cur->forward[0];
    if(cur != nullptr && cur->get_key() == key){
        //逐层更新指针，移除节点
        for(int i = 0; i <= skip_level; ++i){
            if(update[i]->forward[i] != cur)
                break;
            update[i]->forward[i] = cur->forward[i];
        }

        //调整跳表层级
        while(skip_level > 0 && header->forward[skip_level] == nullptr){
            skip_level--;
        }
        delete cur;
        element_count--;
    }
    mtx.unlock();
}

template<typename K, typename V>
void SkipList<K, V>::display_list() {
    for (int i = skip_level - 1; i >= 1; --i) {
        Node<K, V> *cur = header;
        cout << "Level " << i << ": ";
        while (cur->forward[i] != nullptr) {
            cout << cur->forward[i]->get_key() << ": " << cur->forward[i]->get_value() << " ";
            cur = cur->forward[i];
        }
        cout << endl;
    }
}

template <typename K, typename V>
void SkipList<K, V>::dump_file() {
    file_writer.open(STORE_FILE); // 打开文件
    Node<K, V>* node = this->_header->forward[0]; // 从头节点开始遍历

    while (node != nullptr) {
        file_writer << node->get_key() << ":" << node->get_value() << ";\n"; // 写入键值对
        node = node->forward[0]; // 移动到下一个节点
    }

    file_writer.flush(); // 刷新缓冲区，确保数据完全写入
    file_writer.close(); // 关闭文件
}

template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    return !str.empty() && str.find(delimiter) != std::string::npos;
}

template <typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string &str, std::string *key, std::string *value) {
    if (!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

// Load data from disk
template <typename K, typename V>
void SkipList<K, V>::load_file() {
    file_reader.open(STORE_FILE);
    std::string line;
    std::string *key = new std::string();
    std::string *value = new std::string();

    while (getline(file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        // Define key as int type
        insert_element(stoi(*key), *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }

    delete key;
    delete value;
    file_reader.close();
}
#endif //SKIPLIST_NODE_H
