#include <iostream>
#include <string>
#include <cctype>
#include "execution.hpp"
#include "property.hpp"
#include "pipeline.hpp"

void reader(Out<std::string> out) {
    for(std::string line; std::getline(std::cin, line);) {
        out.push(std::move(line));
    }
}

void filter(In<std::string> in, Out<std::string> out) {
    for(auto iter = in.pop(); iter; iter = in.pop()) {
        if(iter->size() < 5) out.push("***");
        else out.push(std::move(iter));
    }
}

void upper(In<std::string> in, Out<std::string> out) {
    for(auto iter = in.pop(); iter; iter = in.pop()) {
        for(auto &&ch : *iter) {
            ch = std::toupper(ch);
        }
        out.push(std::move(iter));
    }
}

void writer(In<std::string> in) {
    for(size_t count {}; ; ++count) {
        auto ptr = in.pop();
        if(!ptr) break;
        std::cout << 'L' << count << ": " << *ptr << std::endl;
    }
}

int main() {
    // Type anything from your keyboard...
    auto future = pipeline(reader, filter, upper, writer);

    // Press ^D
    future.wait();
}