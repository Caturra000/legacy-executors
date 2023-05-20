#pragma once
namespace co {

class Context;

extern "C" __attribute__((noinline, weak, optimize("O3")))
void contextSwitch(Context* prev /*%rdi*/, Context *next /*%rsi*/) {
    asm volatile(R"(
        movq %rsp, %rax
        movq %rax, 104(%rdi)
        movq %rbx, 96(%rdi)
        movq %rcx, 88(%rdi)
        movq %rdx, 80(%rdi)
        movq 0(%rax), %rax
        movq %rax, 72(%rdi)
        movq %rsi, 64(%rdi)
        movq %rdi, 56(%rdi)
        movq %rbp, 48(%rdi)
        movq %r8, 40(%rdi)
        movq %r9, 32(%rdi)
        movq %r12, 24(%rdi)
        movq %r13, 16(%rdi)
        movq %r14, 8(%rdi)
        movq %r15, (%rdi)

        movq 48(%rsi), %rbp
        movq 104(%rsi), %rsp
        movq (%rsi), %r15
        movq 8(%rsi), %r14
        movq 16(%rsi), %r13
        movq 24(%rsi), %r12
        movq 32(%rsi), %r9
        movq 40(%rsi), %r8
        movq 56(%rsi), %rdi
        movq 72(%rsi), %rax
        movq 80(%rsi), %rdx
        movq 88(%rsi), %rcx
        movq 96(%rsi), %rbx

        movq 64(%rsi), %rsi

        movq %rax, (%rsp)
        xorq %rax, %rax
    )");
}

extern "C" __attribute__((noinline, weak, optimize("O3")))
void contextSwitchOnly(Context *next/*%rdi*/) {
    asm volatile(R"(
        movq 48(%rdi), %rbp
        movq 104(%rdi), %rsp
        movq (%rdi), %r15
        movq 8(%rdi), %r14
        movq 16(%rdi), %r13
        movq 24(%rdi), %r12
        movq 32(%rdi), %r9
        movq 40(%rdi), %r8
        movq 64(%rdi), %rsi
        movq 72(%rdi), %rax
        movq 80(%rdi), %rdx
        movq 88(%rdi), %rcx
        movq 96(%rdi), %rbx

        movq 56(%rdi), %rdi

        movq %rax, (%rsp)
        xorq %rax, %rax
    )");
}

} // co
