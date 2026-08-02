#include <iostream>
#include <cstdint>

int main() {/*
    uint8_t height = 12;
    uint8_t width = 98;
    
    uint16_t h = 0;

    h |= uint16_t(height) << 0;
    h |= uint16_t(width) << 8;

    std::cout << "result :" << h << std::endl;

    uint16_t teste_recupe = (h >> 8) & 0xFF;

    std::cout << "value recup = " << teste_recupe << std::endl;*/

    std::cout << sizeof(uint8_t[3]) << std::endl;

    return 0;
}