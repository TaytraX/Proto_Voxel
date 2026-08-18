#include <iostream>
#include <vector>
#include <cstdint>

struct Exemple {
    std::vector<int> ft;
    uint32_t tre[6];
};

Exemple* teste;

void print(Exemple* t) {
    teste = t;
}

int main() {
    Exemple test[2];

    test[0].ft.push_back(2);
    test[0].ft.push_back(2);
    test[0].ft.push_back(3);
    test[1].ft.push_back(5);

    print(test);

    std::cout << teste->ft.size() << std::endl;

    return 0;
}