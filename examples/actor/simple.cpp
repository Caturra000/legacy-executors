#include <iostream>
#include "actor_framework.hpp"

class Object: public Actor {
public:
    Object(auto &pool): Actor(pool) { register_handler(&Object::integer_message_handler); }
private:
    void integer_message_handler(int any, Actor_address from) {
        std::cout << any << " " << from << std::endl;
    }
};

int main() {
    bsio::Static_thread_pool pool(1);
    Object o1(pool), o2(pool);
    send(1, o1.address(), o2.address());
    send(2, o2.address(), o1.address());
    pool.wait();
    return 0;
}