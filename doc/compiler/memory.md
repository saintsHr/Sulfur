# SA3M — Statically Assisted Manual Memory Management

SA3M is a memory management model built on a simple principle:

**The programmer controls memory. The compiler enforces the rules.**

Allocation, usage, and deallocation remain under the programmer's explicit control. The compiler acts as a static supervisor, analyzing memory operations at compile time without relying on runtime checks or automatic memory management.

**Proven violations are errors. Unproven risks are warnings.**

The compiler must reject operations conclusively proven to violate memory safety rules, such as double-free, use-after-free, or guaranteed null dereferences. When a potential violation cannot be conclusively established, the compiler issues a warning and may suggest a fix.

Warnings can be suppressed or promoted to errors through compiler flags. Mandatory rejections cannot be bypassed through warning configuration.

SA3M does not treat uncertainty as proof of failure, nor does it promise to detect every possible error. It preserves programmer freedom while enforcing what static analysis can conclusively establish.

**Manual control. Static supervision. Proof-based enforcement.**
