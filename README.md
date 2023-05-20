# Legacy Executors

## Overview

这个项目是针对C++标准库提案[P0443R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0443r9.html)和[N4242](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4242.html)提出的unified executors模型的C++20复刻实现兼低配版。

目前标准库executors提案仅P0443已经迭代到[R14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)版本(2020-09-15)，并且随后又出现了长度惊人的[P2300R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2300r6.html)(2023-01-19)。

很显然，对比之下，目前这份实现是个披着C++20皮，却被标准库抛弃的实现。但是仍有可取之处：
- 进不了标准，也能自己用
- `asio`一直在使用且迭代该模型
- **足够简单且高可扩展**（这很重要）

相比高度抽象且尚未标准化的`std::execution`，我觉得使用这套方案嵌入于任何三方项目中都是开发友好的。

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

## 概念说明

> TL;DR 你可以直接看提案。

关键字：
* customization point
* executor / execution context / execution agent
* property / applicable property
* require / prefer / query

TODO
