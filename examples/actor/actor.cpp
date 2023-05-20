#include <vector>
#include <memory>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include "execution.hpp"
#include "property.hpp"
#include "actor_framework.hpp"

constexpr bool verbose = false;
using Token = size_t;
constexpr size_t num_member = 100;
constexpr Token token = 1e5;
size_t num_thread = std::thread::hardware_concurrency();

class Member: public Actor {
public:
    explicit Member(bsio::Static_thread_pool &pool): Actor(pool) {
        register_handler(&Member::init_handler);
    }

private:
    // call once
    void init_handler(Actor_address next /*message*/, Actor_address from) {
        _final = from;
        _next = next;

        if constexpr (verbose) {
            std::stringstream ss;
            ss << this << " init: [" << next << ", " << from << "]" << std::endl;
            std::cout << ss.str();
        }

        // notify to receiver
        defer_send(Token{}, _final);

        register_handler(&Member::token_handler);
        deregister_handler(&Member::init_handler);
    }

    void token_handler(Token token /*message*/, Actor_address ignored /*from*/) {
        std::ignore = ignored;
        // For verbose
        // Avoid concurrent and reordered output
        std::unique_ptr<std::stringstream> pss;

        if constexpr (verbose) {
            pss = std::make_unique<std::stringstream>();
            *pss << this << " receives token: " << token;
        }

        if(token > 0) [[likely]] token--;
        Actor_address to = token ? _next : _final;

        if constexpr (verbose) {
            *pss << ", to: " << to << std::endl;
            std::cout << pss->str(); 
        }

        defer_send(token, to);
    }

private:
    Actor_address _next;
    Actor_address _final;
};

int main() {
    using Thread_pool_execution_context = bsio::Static_thread_pool;
    struct Single_thread_exeuction_context: Thread_pool_execution_context {
        Single_thread_exeuction_context(): Thread_pool_execution_context(1) {} 
    };
    std::vector<Single_thread_exeuction_context> thread_pool(num_thread);
    std::vector<std::unique_ptr<Actor>> members(num_member);
    Receiver<Token> receiver;

    for(size_t idx = 0; idx < members.size(); ++idx) {
        members[idx] = std::make_unique<Member>(thread_pool[idx % thread_pool.size()]);
    }
    // back() != front()
    assert(members.size() > 1);

    // In each iteration, receiver sends a member's address as a message to the next member
    for(size_t i = 0; i < members.size(); ++i) {
        // Finally construct a circular message ring
        size_t next = (i+1 == members.size()) ? 0 : i+1;
        send(members[i]->address(), receiver.address(), members[next]->address());
    }

    // A synchronization point
    receiver.wait(members.size());

    for(auto &&member : members) {
        send(token, receiver.address(), member->address());
    }

    receiver.wait(members.size());

    std::cout << "done!" << std::endl;

    return 0;
}
