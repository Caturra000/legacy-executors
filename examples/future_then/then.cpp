#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <optional>
#include <map>
#include <syncstream>
#include <chrono>
#include "execution.hpp"
#include "Future/Future.hpp"

using Pid = size_t;

// Avoid out-of-order output in concurrent situations
void println(auto &&...args) {
    std::osyncstream cout(std::cout);
    (cout << ... << args) << std::endl;
}

struct Webpage {
    Webpage() = default;
    explicit Webpage(std::string url): _url(std::move(url)) {};

    std::string current() const { return _url; }

    bool login(const std::string &username, const std::string &password) const {
        println('[', username, "] login...");
        if(username == "jojo") return password == "I LOVE DIO";
        if(username == "dio") return password == "I LOVE JOJO";
        return false;
    }

    std::vector<Pid> search_by_tag(std::string user_tag) {
        return _gallery[user_tag];
    }
    
    static std::string home_page() {
        return "https://www.pixiv.net";
    }

private:
    // key: tag
    // value: pixiv pid (artworks/[pid])
    // example: https://www.pixiv.net/artworks/82818137
    inline static std::map<std::string, std::vector<Pid>> _gallery {
        {"R18", {74841722, 80315612, 76611510}},
        {"NTR", {82818137, 76595171, 96615810, 96812601, 87746247}}
    };

private:
    std::string _url;
};

void jitter() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
}

static Webpage connect(std::string username, std::string url) {
    println('[', username, "] connect...");
    jitter();
    return Webpage{url};
}

int main() {
    Then_execution_context context(std::thread::hardware_concurrency());
    auto ex = context.then_executor();
    assert(bsio::query(ex, bsio::execution::directionality.then));

    struct User_info {
        std::string username;
        std::string password;
        std::string tag;
    };

    User_info jojo_info {
        .username = "jojo",
        .password = "I LOVE DIO",
        .tag = "NTR"
    };

    User_info dio_info {
        .username = "dio",
        .password = "I LOVE JOJO",
        .tag = "R18"
    };

    std::vector<User_info> pixivers {jojo_info, dio_info};
    
    for(auto user_info : pixivers) {
        ex.then_execute([user_info] {
            Webpage page = connect(user_info.username, Webpage::home_page());
            return page;
        })
        .then([user_info](Webpage page) {
            println('[', user_info.username, "] is already connected to ", page.current());
            std::optional<Webpage> opt;
            bool ok = page.login(user_info.username, user_info.password);
            jitter();
            if(ok) opt = Webpage(page.current() + "/tag/" + user_info.tag);
            return opt;
        })
        .then([user_info](std::optional<Webpage> tag_page) {
            if(!tag_page) throw std::runtime_error("401 ERROR: Unauthorized");
            println('[', user_info.username, "] is already opened ", tag_page->current());
            return tag_page->search_by_tag(user_info.tag);
        })
        .then([user_info](std::vector<Pid> &&picture_pids) {
            std::osyncstream cout(std::cout);
            cout << '[' << user_info.username << "] gets pictures: [";
            for(auto &&pid : picture_pids) {
                cout << pid << ", ";
            }
            cout << "...]" << std::endl;
            return nullptr;
        });
    }

    context.wait();
    return 0;
}