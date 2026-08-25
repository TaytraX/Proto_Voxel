#include <iostream>
#include <vector>
#include <cstdint>
#include <unordered_map>



int main() {
    float d = -0.71;
    std::cout << std::fmod(d, 64) << std::endl;
    return 0;
}