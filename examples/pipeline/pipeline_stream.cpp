#include <iostream>
#include <string>
#include <cctype>
#include "execution.hpp"
#include "property.hpp"
#include "pipeline_stream.hpp"

auto reader = [](Out<std::string> out) {
    for(std::string line; std::getline(std::cin, line);) {
        out.push(std::move(line));
    }
};

auto filter = [](In<std::string> in, Out<std::string> out) {
    for(auto iter = in.pop(); iter; iter = in.pop()) {
        if(iter->size() < 5) out.push("***");
        else out.push(std::move(iter));
    }
};

auto upper = [](In<std::string> in, Out<std::string> out) {
    for(auto iter = in.pop(); iter; iter = in.pop()) {
        for(auto &&ch : *iter) {
            ch = std::toupper(ch);
        }
        out.push(std::move(iter));
    }
};

auto writer = [](In<std::string> in) {
    for(size_t count {}; ; ++count) {
        auto ptr = in.pop();
        if(!ptr) break;
        std::cout << 'L' << count << ": " << *ptr << std::endl;
    }
};

int main() {
    // `pipeline_stream::make`: Create a pipeline factory
    // `pipeline_stream::done`: Create a static pipeline
    using namespace pipeline_stream;

    // Type anything from your keyboard...
    auto future = make | reader | filter | upper | writer | done;

    // Press ^D
    future.wait();
}
