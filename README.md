# Legacy Executors

## Overview

这个项目是针对C++标准库提案[P0443R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0443r9.html)和[N4242](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4242.html)提出的unified executors模型的C++20复刻实现兼低配版

目前标准库executors提案仅P0443已经迭代到[R14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)版本(2020-09-15)，并且随后又出现了长度惊人的[P2300R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2300r6.html)(2023-01-19)

很显然，对比之下，目前这份实现是个披着C++20皮，却被标准库抛弃的实现。但是仍有可取之处：
- 进不了标准，也能自己用
- `asio`一直在使用且迭代该模型
- **足够简单且高可扩展**（这很重要）

相比高度抽象且尚未标准化的`std::execution`，我觉得使用这套方案嵌入于任何三方项目中都是开发友好的

## 实现进展

总的来说就是完成三个模块：
* 实现`require/query`的非侵入定制点
* 以及`lightweight executor`和`applicable property`的交互
* 对所有相同`property`，将对应`executor`做类型抹除处理以支持多态的`polymorphic executor`
* （其实还有一个赠品线程池，因为提案写了）

对于`executor`的`property`，库内已集成：
* `directionality`：表示`executor`是否有向，比如是不考虑返回的`oneway`，或者是需要返回的`twoway`，或者`continuation`风格的`then`
* `blocking`：表示一个任务的执行是否会阻塞执行流，`never`会保证当前执行流不会阻塞，但有最高的线程（安全）开销，`always`相反，`possibly`折中
* `mapping`：任务与`execution agent`的映射关系，比如是使用per-thread，还是复用thread，还是inline，还是coroutine
* `outstanding_work`：维护当前的执行上下文（`execution context`），可用于阻止提前退出，避免退出再次提交任务后不必的恢复开销
* `relationship`：如果明确一个任务的提交已经是在`execution context`内部再次提交，可以用`continuation`标记

相比提案，我砍掉了萃取的支持。因为到了`C++20`之后，有不少接口是可以直接用`requires-expression`来完成

## 快速上手

可以看`examples`里的丰富示例，目前有：
* `hello_world`：简单实现`inline executor`、使用内置线程池，以及类`asio`的任务提交方式
* `custom_executor`: 使用自定义`property`定制一个具有任务优先级的并发线程池
* `polymorphic`: 实现多态下的通用`executor`
* `future`：使用`twoway_executor`实现`promise/future`异步模型
* `future_then`: 使用`then_executor`实现`promise/future`且符合`then continuation`模式
* `stackful_coroutine`:使用`co_executor`实现有栈协程
* `actor`: 定制一个`actor`通信框架
* `pipeline`: 定制一个`pipeline`模式

你可以通过了解示例，来理解`executor`到底是什么

> TL;DR 你也可以直接看提案，不长

也可以通过这些示例，尝试把unified executors适配到任意一个第三方项目

### 示例1：hello world!

你可以简单的定制一个`inline executor`，表示立刻在当前执行

```C++
#include <concepts>
#include <iostream>
#include <cassert>
#include "execution.hpp"
#include "property.hpp"

using Blocking = bsio::execution::Blocking;
constexpr auto blocking = bsio::execution::blocking;

struct Inline_executor {
    void execute(std::invocable auto &&func) { func(); }

    static constexpr auto require(Blocking::Always) {
        return Inline_executor{};
    }

    constexpr auto prefer(Blocking::Never) {
        return *this;
    }

    constexpr auto query(Blocking::Always) {
        return true;
    }

    constexpr auto query(Blocking) {
        return blocking.always;
    }

    auto operator<=>(const Inline_executor&) const = default;
};

auto require(Inline_executor e, Blocking::Never) {
    throw std::runtime_error("impossible.");
    return nullptr;
}

auto prefer(Inline_executor e, Blocking::Possibly) {
    return e;
}

int main() {
    // call static Inline_executor::require()
    auto ex = bsio::require(Inline_executor{}, blocking.always);

    // call ::prefer()
    auto ex2 = bsio::prefer(ex, blocking.possibly);
    ex2.execute([] { std::cout << "hi!" << std::endl; });

    // call Inline_executor::query
    std::cout << std::boolalpha << bsio::query(ex2, blocking.always) << std::endl;

    static_assert(ex2 == ex);
    static_assert(blocking.always == bsio::query(ex2, blocking));
}
```

一个`executor`是轻量级且类型安全的：
* 轻量级意味着你可以随意使用值语义，既：copy是廉价的
* 类型安全意味着`executor`本身可以是携带`template`信息。这意味着，当没有所需的`property`时，会在编译期立刻发现（阻止）；以及使得`execution context`并没有任何`template`，这在API的简化上有很大帮助
* 无论是lightweight copyable executor还是non-templated execution context，都是提案为了简化泛型编程而做出的设计

`bsio::require`接受一个`executor`和另一个用户需要的`property`，返回一个有对应`property`的`executor`；在这里存在的细节是，`bsio::require`如果发现不存在这样的`executor`，那么会直接编译期阻止编译，而`bsio::prefer`则相对宽松，如果不存在，那么可以原样返回。

需要查询一个`executor`是否有这样的`property`，或者对应的`property`具体是什么，可以调用`bsio::query()`来完成

这三个最关键的接口都可以做到编译时完成，既`constexpr`，除非用户层定制的`executor`需要额外的运行时信息

这里可以留意到细节，用户层也有同名的`prefer/require/query`。其实`bsio::`库是调用用户层定义的函数来完成的，提案给出的是一种非侵入式的特性，它可以是成员函数，也可以是自由函数，或者是其它namespace下的同名函数，包括库内部已经实现好的`bsio::require`

### 示例2：hello world!!

前面的例子没怎么看出一个类型安全的`executor`是什么概念，那么在这个线程池使用示例的例子中进一步了解一下复杂的`executor`类型

```C++
#include "execution.hpp"
#include "property.hpp"
#include <iostream>
#include <atomic>
std::atomic<int> sum {0};
void cal(int i) {
    sum.fetch_add(i, std::memory_order_relaxed);
}

int main() {
    bsio::Static_thread_pool pool(std::thread::hardware_concurrency());
    constexpr auto blocking = bsio::execution::blocking;
    constexpr auto relationship = bsio::execution::relationship;

    auto ex1 = pool.executor();
    auto ex2 = ex1.require(blocking.never);
    auto ex3 = ex2.require(relationship.continuation);
    auto ex4 = ex1.require(blocking.always);

    static_assert(ex2.query(blocking.never), "executor-2 must have a never-block property");
    static_assert(ex3.query(relationship.continuation));

    constexpr static size_t TEST_ROUND = 100000;

    for(size_t i = 0; i < TEST_ROUND; ++i) {
        ex1.execute([=] {cal(1);});
        ex2.execute([=] {cal(1);});
        ex3.execute([=] {cal(1);});
    }

    std::function<void()> recursive = [&, ex = ex3, r = std::make_shared<std::atomic<size_t>>()]() mutable {
        size_t v = r->fetch_add(1, std::memory_order_relaxed);
        if(v < TEST_ROUND) {
            sum.fetch_add(1, std::memory_order_relaxed);
            ex.execute(std::ref(recursive));
            // Greedy multishot
            if(v * v < TEST_ROUND) {
                ex.execute(std::ref(recursive));
            }
        }
    };

    ex4.execute(recursive);

    pool.wait();

    std::cout << sum.load() << std::endl;

    return 0;
}
```

在这个例子中，不同的`executor`如`ex1 / ex2 / ex3 / ex4`都可能是完全不同的类型，因此一般的使用方式是靠`auto`推导

> 比如`ex3`可能是一个
> ``` C++
> bsio::Static_thread_pool::Executor_impl<bsio::execution::Directionality::Oneway, bsio::execution::Blocking::Never, bsio::execution::Relationship::Continuation, std::allocator<void>>
> ```
> 类型，既把`property`都放到模板上，这样就可以完成`require`等接口的`constexpr`零开销实现（构造函数也需要`constexpr`）。当然用户层也可以自定义返回非模板示例

这个例子就简单展示一下线程池的创建、执行以及等待

### 示例3：Hello world!!!

```C++
#include <iostream>
#include "bsio.hpp"

int main() {
    static size_t recursive = 10;
    bsio::Static_thread_pool pool(2);
    bsio::dispatch(pool.executor(), [] {std::cout<<"x\n";});
    bsio::post(pool.executor(), [] {std::cout<<"xx\n";});
    bsio::defer(pool.executor(), [] {std::cout<<"xxx\n";});
    pool.wait();
    if(recursive--) {
        main();
    }
    return 0;
}
```

这些接口其实是为了迎合`asio`风格额外添加的，适用于大部分场合，提前打包而不必每次都`require`不同的`property`

接口用途的区别可以看[这里](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio/reference/Executor1.html)

### 示例4：custom executor


```C++
#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <cassert>
#include <array>
#include "execution.hpp"
#include "property.hpp"
#include "priority.hpp"

int main() {
    Priority_execution_context context;
    auto ex = context.executor();
    // Lowest priority
    auto last_step_ex = bsio::require(ex, Priority{low_priority.value()-1});

    std::atomic<size_t> seq {0};

    // Higher priority has a smaller value

    std::atomic<size_t> low_seq_sum {0};
    std::atomic<size_t> normal_seq_sum {0};
    std::atomic<size_t> high_seq_sum {0};

    std::vector<std::thread> threads;

    constexpr size_t num_executor = 3;
    // Worker threads
    constexpr size_t num_worker = 10;
    // Background threads
    constexpr size_t num_bg = 2;
    constexpr size_t num_task = num_executor * num_worker * 1e5;

    auto update_seq_sum = [&seq](std::atomic<size_t> &ex_seq) {
        auto val = seq.fetch_add(1, std::memory_order_acq_rel);
        ex_seq.fetch_add(val, std::memory_order_acq_rel);
    };

    std::thread background[num_bg];
    for(auto &&bg : background) {
        bg = std::thread {&Priority_execution_context::run, &context};
    }

    auto task = [&, ex, ptr = std::addressof(context)] {
        auto low_ex = bsio::require(ex, low_priority);
        auto normal_ex = bsio::require(ex, normal_priority);
        auto high_ex = bsio::require(ex, high_priority);
        assert(bsio::query(high_ex, bsio::execution::context) == ptr);

        for(auto _ = num_task / num_executor / num_worker; _--;) {
            low_ex.execute([&] { update_seq_sum(low_seq_sum); });
            normal_ex.execute([&] { update_seq_sum(normal_seq_sum); });
            high_ex.execute([&] { update_seq_sum(high_seq_sum); });
        }
    };
    for(size_t thread_idx = 0; thread_idx < num_worker; thread_idx++) {
        std::thread {task}.detach();
    }

    last_step_ex.execute([&] {
        // Stop background thread(s)
        context.stop();
        constexpr double num_executor_task = num_task * num_executor;
        auto low_seq_avg = low_seq_sum / num_executor_task;
        auto normal_seq_avg = normal_seq_sum / num_executor_task;
        auto high_seq_avg = high_seq_sum / num_executor_task;
        std::cout << std::fixed;
        std::cout << "low-avg: \t" << low_seq_avg << std::endl;
        std::cout << "normal-avg: \t" << normal_seq_avg << std::endl;
        std::cout << "high-avg: \t" << high_seq_avg << std::endl;
        assert(high_seq_avg < normal_seq_avg);
        assert(normal_seq_avg < low_seq_avg);
        assert(seq == num_task);
    });

    for(auto &&bg : background) bg.join();
}
```

property是可以自定义的，这个例子通过定义一个优先级`priority`来实现具有任务优先级的线程池

### 示例5：polymorphic executor

```C++
#include <iostream>
#include <chrono>
#include "execution.hpp"
#include "property.hpp"

struct Double_executor {
    void execute(std::invocable auto &&func) {
        func();
        if(blocking) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(200ms);
        }
        func();
    }

    auto operator<=>(const Double_executor &) const = default;

    bool blocking {false};
};

Double_executor require(Double_executor, bsio::execution::Blocking::Possibly) {
    return Double_executor{.blocking = true};
}

int main() {
    using namespace bsio::execution;
    using Executor = Polymorphic_executor<Directionality::Oneway, Blocking::Possibly>;
    Executor ex;

    bsio::Static_thread_pool pool(1);    
    ex = pool.executor();
    ex.execute([] { std::cout << "foo\n"; });

    ex = Double_executor{};
    ex = bsio::require(ex, blocking.possibly);
    ex.execute([] { std::cout << "bar\n"; });

    pool.wait();
    return 0;
}
```

由于类型安全的特性，不同的executor并不能用单一的类型来描述，这在非泛型编程中受到一定的限制。比如一个已有的框架中不可能为了一个`executor`而大幅改动将不含任何`template`的类为`template`实现，因此引入多态类在实用性上是有必要的

注意如果是对于一个`polymorphic executor`进行`bsio::prefer`，且用户层并没有实现的话，那么会返回一个cloned executor

### 示例6：leader-followers

```C++
#include <functional>
#include <any>
#include <mutex>
#include "execution.hpp"

struct LF {
public:
    LF(size_t threads): _pool(threads) {}

public:
    void on_accept(auto f) {
        _accpet = std::move(f);
    }

    void on_process(auto f) {
        _process = std::move(f);
    }

    void stop() { _pool.stop(); }
    void wait() { _pool.wait(); }

public:
    void start() {
        std::call_once(_first_check, [this] {
            if(!_accpet || !_process) {
                throw std::logic_error("You should first register callbacks");
            }
        });

        start_leader();
    }

private:
    void start_leader() {
        auto ex = bsio::require(_pool.executor(),
                    bsio::execution::blocking.never);
        // Leader thread
        ex.execute([this, ex]() mutable {
            std::any ret = _accpet();
            // Promote a follower thread
            // to become the new leader
            ex.execute([this] {start_leader();});
            _process(std::move(ret));
        });
    }

private:
    // Followers
    bsio::Static_thread_pool _pool;
    // Wait for event
    std::function<std::any()> _accpet;
    // Long-running task
    std::function<void(std::any)> _process;

    std::once_flag _first_check;
};
```

这里展示一个最简单的leader-followers模式，这种模式适用于简单的per-thread的改进。既不需要queue做生产者消费者中介，也不会因为任务提交切换线程而带来更多开销

### 示例7：promise future

```C++
#include <iostream>
#include <chrono>
#include <cassert>
#include "execution.hpp"

int main() {
    bsio::Static_thread_pool background(1);
    auto executor = bsio::require(background.executor(), bsio::execution::directionality.twoway);
    std::future<void> future = executor.twoway_execute([] {
        std::cout << "Hi" << std::endl;
    });
    future.wait();
    std::cout << "Bye" << std::endl;

    std::future<int> guess_number = executor.twoway_execute([] {
        return 1926 & 817;
    });
    std::cout << guess_number.get() << std::endl;

    return 0;
}
```

库内部也继承了标准库风格的异步`promise-future`，支持返回接受任意类型。只需要使能`directionality.twoway`

注意这种`property`属于`interface-changable property`，就是说该`executor`只有`twoway_execute()`而没有`execute()`

> 后者是one way executor使用，同时存在两个接口的话显然违反类型安全以及单一职责

### 示例8：promise future continuation

```C++
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
```

很显然，标准库的`std::future`是不支持continuation style的（连`then`异步都不支持，多少有点残疾）

那么用`executors`库定义一下统一接口岂不是分分钟的事情

现在，在这个例子中存在2个执行流（jojo和dio）并发执行一个模拟网上冲浪的过程，并且continuation style下的实现能用近乎同步的方式描述异步流程

### 示例9：stackful coroutine

```C++
#include <ranges>
#include <iostream>
#include <queue>
#include "co.hpp"
#include "Co_executor.hpp"

int main() {
    auto ex = bsio::require(Co_executor{}, bsio::execution::mapping.other);

    Looper looper(ex);

    auto fibonacci = ex.execute([&](size_t n) {
        constexpr size_t shift = 2;
        constexpr size_t mask = 3;
        size_t table[2 + shift] = {0, 1};
        for(auto i : std::views::iota(shift, n + shift)) {
            table[i & mask] = table[i-1 & mask] + table[i-2 & mask];
            std::cout << "[fibonacci]-" << i-shift << ":\t" << table[i & mask] << std::endl;
            looper.yield();
        }
    }, 7);

    auto stirling = ex.execute([&] {
        constexpr size_t table[] = {
            1, 3, 13, 75, 541, 4683, 47293, 545835, 7087261
        };
        for(auto i : std::views::iota(size_t{0}, std::size(table))) {
            std::cout << "[stirling]-" << i << ":\t" << table[i] << std::endl;
            looper.yield();
        }
    });

    return 0;
}
```

这个例子展示有栈协程在executors库下的适配，这里简单点就展示斐波那契和斯特林数在协程下的交替求值过程

> 因为这个库里并不包含网络基础设施，我没法直接展示IO相关的操作，虽然协程本身是适配了网络IO。。。


### 示例10：actor

```C++
#include <vector>
#include <memory>
#include <cassert>
#include <iostream>
#include <syncstream>
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
            std::osyncstream cout(std::cout);
            cout << this << " init: [" << next << ", " << from << "]" << std::endl;
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
        alignas(std::osyncstream) std::byte raw_out[sizeof(std::osyncstream)];
        std::osyncstream *pcout;

        if constexpr (verbose) {
            pcout = new (&raw_out) std::osyncstream(std::cout);
            *pcout << this << " receives token: " << token;
        }

        if(token > 0) [[likely]] token--;
        Actor_address to = token ? _next : _final;

        if constexpr (verbose) {
            *pcout << ", to: " << to << std::endl;
            pcout->~basic_osyncstream();
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
```

用`executors`也可以封装[`actor`](https://en.wikipedia.org/wiki/Actor_model)框架。这里展示一下任意actor数目的击鼓传花

### 示例11：pipeline

```C++
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
```

如果并发任务间的输入输出互有依赖，可以使用[`pipeline`](https://en.wikipedia.org/wiki/Pipeline_(software))完成这个工作

话说这都完全看不出executor的模样了
