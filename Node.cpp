//
// Created by 贾明卓 on 2024/4/12.
//

#include "Node.h"

int main(){
    int N, K, M;

    std::cin >> N >> K >> M;

    SkipList<int, int> *skiplist = new SkipList<int, int>(16);

    // 插入数据
    for (int i = 0; i < N; i++) {
        int k, v;
        std::cin >> k >> v;
        if (skiplist->insert_element(k, v) == 0) {
            std::cout << "Insert Success" << std::endl;
        } else {
            std::cout << "Insert Failed" << std::endl;
        }
    }

    skiplist->display_list();



    return 0;
}