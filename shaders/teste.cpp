#include <iostream>

class Teste {

    public:
    virtual void print() = 0;
    virtual void crepit() = 0;
};

class Teste2 : public Teste {

    public:
    void print() override {
        std::cout << "Teste2" << std::endl;
    }
    void crepit() override {
        std::cout << "Teste2 crepitando" << std::endl;
    }
};

class Teste3 : public Teste {

    public:
    void print() override {
        std::cout << "Teste3" << std::endl;
    }
    void crepit() override {
        std::cout << "Teste3 crepitando" << std::endl;
    }
};

int main() {
    class Teste* teste = new Teste2();
    class Teste* teste2 = new Teste3();
    teste->print();
    teste2->print();

    return 0;
}