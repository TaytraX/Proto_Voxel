#include <iostream>
#include <string>
#include <cstdint>
#include <queue>

std::queue<int> g;
int main() {
    for(int i = 1; i < 20; i++) g.push(i);

    std::cout << g.empty() << std::endl;

    return 0;
}