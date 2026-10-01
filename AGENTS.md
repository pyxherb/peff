# General Instructions of SLKC for Agents

This instruction will instruct you to maintain and understand the project more efficiently.

## Rules

This library uses **both C++17 and C++20**, which means it enables features conditionally by checking the C++ standard version (via the `PEFF_CPLUSPLUS` macro constant in `peff/base/basedefs.h`). Use `.h` for C/C++ header files and use `.cc` for C++ source files by default.

For C++, use `snake_case` for functions, variables, namespaces and file names, `UpperCamelCase` for types, add `_` prefix for private members (except types and namespaces):

```cpp
namespace my_namespace {
    class MyClass {
    private:
        struct MyType {
        };

        using PrivateAlias = uint32_t;

        int _private_member;

        void _test_impl();

    public:
        inline int get_private_member() const noexcept {
            return this->_private_member;
        }

        inline void set_private_member(int private_member) noexcept {
            this->_private_member = private_member;
        }
    }

    void public_fn() {
        int local_var = 123;
        // ...
    }
}
```

Always use Doxygen-styled documentations in C++, and use US English in documentations and comments.

DO NOT USE **ANY** exception in this project, unless you are testing compatibility with exceptions. The neccessary functionalities of the library **must be available under exceptionless environment**. Add `noexcept` function modifiers if possible (must not cause aborting).

**RTTI** is ABSOLUTELY PROHIBITED, you **MUST NOT** implement anything in this library with RTTI.

Usually, we make a header file corresponds to a source file with the same name (e.g. `type.h` vs `type.cc`), some header-only ones have no source file, find sources by this pattern first.

**All** OOM errors should be handled and reported to the caller elegantly, aborting or suppressing errors are **ABSOLUTELY UNACCEPTABLE**, also, the library chosen for this project must support OOM error reporting (no matter if using exceptions or error codes, for former we allow to catch exceptions and convert them into errors but spreading it out is unallowed). If you are designing an API, prefer error codes/error objects.

DO NOT USE standard library containers involving memory allocations such as `std::vector`, `std::string`, `std::map` or any other container using memory allocations. Always use containers from PEFF (such as `peff::DynArray`, `peff::String`, `peff::Map`) instead. `std::function` should be noticed and prohibited because it will allocate memory unexpectly in some implementations and you cannot control the details of allocation. Use interface + virtual method mode or use generics instead if you have to use `std::function`. Also notice the types introducing indirect allocation details you cannot control such as `std::default_deleter` and `std::shared_ptr`. Containers without dynamic memory allocation are allowed, e.g. `std::bitset`, `std::string_view`, `std::span` and `std::array`.

Provide interfaces allowing the user to specify allocator to be used in the APIs. If it's not viable, stop immediately and let me make decisions.

DO NOT use primitive `new` and `delete` directly unless you are implementing an allocator providing primitve allocating and releasing functions. Placement new is allowed because it does not allocate memory.

Coroutine frame allocations also must not violate the memory management rules (you can rewrite the `operator new` and `operator delete` of the `promise_type` to override the default allocating and deallocating behaviors).

For C++ libraries, find library paths by locating variables named `<Library name>_DIR` in `CMakeCache.txt` in the build directory. You should use tools like `grep` to perform it unless you were fail to find. Variable definitions are like `peff_DIR=/path/to/library`.

If something was fail to perform, find another way to perform it but avoid risky operations, or report to me if it's unviable or you need my decision.

Because C++17 is the minimum supported standard, there's no coroutines for derecursion.

If you found your design/next operations/external libraries will violate the rules, stop and ask me to make decisions or let me give you suggestions or instructions.

**NEVER TOUCH GIT!** If you want to operate with Git, ask me for help.

## Project Status

PEFF currently has 5 components:

* **Base** (`peff/base`), included basic facilities to support anything based on these infrastructures, allocator is also included here.
* **Utilites** (`peff/utils`), included some utilities for common functionalities, such as bit manipulation (`peff/utils/bitops.h`), hash computing (`utils/hash.h`).
* **containers** (`peff/containers`), the most heavy and important one, just like `std::collections` in Rust or containers in `java.util`, it contains pre-implemented data structures for general purposes. They are all **exceptionless, even OOM is encountered, they don't throw `std::bad_alloc`**. You also **MUST NOT** suppress the OOM error silently, and **must report it to the caller**. Keep the containers' every single operation atomic, as more as possible.

## Building

Use `cmake -S . -B build` to configure the project and use `cmake --build build --target <Target you wanted> --config <Configuration type you wanted> ` to build a target with configuration type you wanted.

For GCC/Clang, you can set `ENABLE_ADDRESS_SANITIZER` cache variable of CMake to control if to enable AddressSanitizer. For MSVC, set `addressSanitizerEnabled` field of the configuration in `CMakeSettings.json` to control if the AddressSanitizer is enabled.

## Tricks

* When a C++ template error occurs, you must locate the line with the error marker such as `error:` rapidly and search around (forward or backward, only one direction is valid which is determined by the compiler, search with another direction if you found that the information is not related to the error) the line to confirm the type of error and the context.
