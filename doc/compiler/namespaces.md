## Namespaces

Namespaces group related declarations under a common name, avoiding name
collisions and keeping large codebases organized.

A namespace can contain anything that is allowed at the top level, except imports.
See [top-level](top-level.md) for details.

```
// namespace syntax:
// namespace [name] { [declarations] }

namespace math {
    u64 add(u64 a, u64 b) {
        return a + b;
    }
}
```

# Accessing members

Members are accessed from outside the namespace with the scope resolution
operator `::`.

```
u64 x = math::add(1, 2);
```

Inside a namespace, its own members can be used without qualification.

```
namespace math {
    u64 add(u64 a, u64 b) {
        return a + b;
    }

    u64 add_one(u64 n) {
        return add(n, 1); // no need for math::add
    }
}
```

# Global namespace

A leading `::` refers to the global namespace. This is useful to access a
member by its full path, even when a local name would otherwise shadow it.

```
namespace foo {
    void bar() { }
}

namespace other {
    namespace foo { }

    void test() {
        foo::bar();    // error: resolves to other::foo
        ::foo::bar();  // OK: resolves to the global foo
    }
}
```

# Nested namespaces

Namespaces can be nested inside other namespaces. Nested namespaces can be
declared either one inside the other, or in a single declaration using `::`.

```
namespace engine {
    namespace render {
        void draw() { }
    }
}

// equivalent:
namespace engine::render {
    void draw() { }
}

engine::render::draw();
```

# Reopening

A namespace can be declared multiple times, even across different files. All
declarations with the same name contribute to the same namespace.

```
namespace math {
    u64 add(u64 a, u64 b) { return a + b; }
}

namespace math {
    u64 sub(u64 a, u64 b) { return a - b; }
}

// both math::add and math::sub exist
```
