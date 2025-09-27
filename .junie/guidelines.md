# 1. General

- Keep the code and comments clean and readable, and in the same style as the rest of the project.
- The best code is kept as simple as possible. Requirements often change, so we don’t want to add code we don’t need.
- If there is doubt about the best way to proceed, stop and ask.
- Prefer to avoid duplicating code when fixing bugs and issues.
- Prefer composition over inheritance unless there is a clear type hierarchy.
- Avoid placing large functions in headers; prefer small inline helpers.
  - Where a large function is templated, consider creating a separate header file and implementation file.
  - Generate concrete instantiations in the implementation file with common types to reduce compile times.

# 2. Libraries and tools

- Logging: use spdlog and the default logger, with capitalized method names, e.g. `SPDLOG_INFO("Message {}", var);`
  - Initialize logging once per process (logger pattern, level, sinks). Avoid double-initialization in libraries.
  - Use levels consistently: TRACE (very noisy), DEBUG (debugging), INFO (state transitions), WARN (recoverable anomaly), ERROR (local failure), CRITICAL (process-level failure).
  - Add logging for critical failures and state transitions. Avoid logging the same error at multiple layers.
- Math: use Eigen for vector and matrix math.
- Time: prefer `std::chrono` for dealing with time. Use `steady_clock` for durations/intervals, `system_clock` for wall time, `utc_clock` when applicable.
- Imaging and file IO: use Ravl2 types for image processing and IO.
- JSON: use `nlohmann_json::json` for JSON processing.
- Filesystem: use `std::filesystem` for paths. All paths are UTF-8.
- Use cxxopts for command line parsing found in 'cxxopts.hpp'

# 3. Language and naming conventions

- Use C++23.
- Naming:
  - Methods and variables: camelCase.
  - Classes and structs: PascalCase.
  - Prefix member variables with `m` (e.g., `mPlanInterval`) and make sure they are initialised.
  - Constants: prefix with `k` and use PascalCase (e.g., `kMaxRetries`).
  - Enums: prefer `enum class` with PascalCase enumerators.
  - Use namespace Ravl2 as the project root. Avoid overly nested namespaces; prefer concise, descriptive names.
- Comments:
  - Use `//!` for C++ comments with Doxygen tags starting with `@`.
  - Include examples in the comments if the use of a function isn’t clear.
  - Document classes and class methods with Doxygen.
- Indentation is 2 spaces, no tabs.
- Use `#pragma once` instead of include guards.
- Avoid `using namespace ...` except to bring literals like `std::chrono_literals` into another scope.
- Keep code out of header files unless it is a template or a very small inline method.
- Make sure declarations and definitions are consistent between source and header files.
- Ownership and performance:
  - Prefer move semantics and pass by value when cheap, otherwise pass by `const&`.
  - Use `std::span` and `std::string_view` for non-owning views; document lifetime expectations.
  - Mark functions `noexcept` when they cannot throw.

# 4. Resource management and errors

- Properly manage resources with RAII principles. Favor `std::unique_ptr` and `std::shared_ptr` over raw pointers.
- Error handling:
  - For common/expected errors, prefer returning error information rather than throwing exceptions.
  - For rare or logical errors (e.g., corrupt config), throwing is acceptable.
  - The code has been updated to use C++23, so use `std::expected` in new code.
  - Do not both return and log the same error at multiple layers. Log errors at the boundary where action is taken.
  - Prefer error codes/enums over magic numbers. Map to readable messages at boundaries.

# 5. Concurrency

- If a function or class uses threading, be clear about which methods are thread safe.
- Prefer `std::jthread` with `std::stop_token` for cancellable tasks.
- Use `std::mutex`, `std::shared_mutex`, and `std::scoped_lock`/`std::lock_guard` as appropriate. Avoid manual `new` for synchronization primitives.
- Avoid data races by design; make immutability the default for shared data.
- Document lock ordering if multiple locks are used.
- For wait/wake, prefer condition variables with timeouts using `std::chrono`.

# 6. Time and scheduling

- Use `steady_clock` for measuring intervals and scheduling.
- Serialize times as ISO-8601 UTC if persisted or logged.
- Use `std::chrono_literals` for readability in small scopes only.

# 7. Files, paths, and configuration

- Use `std::filesystem::path` for file paths and join/normalize with `operator/`.
- All files are UTF-8 encoded. Avoid BOM.
- Configuration files:
  - Use JSON via `nlohmann_json`. Provide a version field and validate required keys.
  - Treat corrupt or incompatible configs as exceptions (rare/logical error).
  - Provide migration or defaults as appropriate.

# 8. JSON usage (nlohmann_json)

- Validate presence and type of required fields. Provide reasonable defaults for optional fields.
- Keep JSON schemas stable; if breaking changes are necessary, bump the schema version.

# 9. Imaging and Ravl2 specifics

- Prefer Ravl2 image containers and IO.
- Be explicit about color space and pixel layout (e.g., RGB, BGR, RGBA) in function docs.
- Document ownership of buffers passed into Ravl2 interop.

# 10. Build and CMake

- Use target-based CMake with modern commands:
  - `add_library(NAME ...)`, `add_executable(NAME ...)`.
  - `target_compile_features(NAME PRIVATE cxx_std_23)`.
  - `target_link_libraries(NAME PRIVATE ... PUBLIC ... INTERFACE ...)`.
  - Prefer `PRIVATE` for most dependencies; expose via `PUBLIC` only if required by the interface.
- Organize tests as separate `tests` targets in the top level directory. Do not add tests to production targets.
- Enable warnings and treat as errors in CI profiles. Example: `-Wall -Wextra -Wpedantic`.
- Guard optional dependencies behind options (e.g., `OPTION(ENABLE_FOXGLOVE OFF)`), and use `if(ENABLE_...)` blocks.
- If using make projects, use parallel compiles.
- 
# 11. Static analysis and formatting

- Use `clang-format` with a project config. Format all code before commit.
- Use `clang-tidy` in CI with a curated checks list. Fix newly introduced warnings.
- Include order:
  1. Standard library headers
  2. Third-party headers
  3. Local project headers
  4. Corresponding header
- Do not rely on transitive includes; include what you use.

# 12. Testing

- Write concise tests that cover required functionality with as few tests as possible.
- Give priority to writing tests that cover overall functionality over individual functions.
- Use Catch2 test framework.
- Include both normal and edge cases.
- In this project, unit tests are under the top-level `test` directory.
- Naming:
  - Test files: `<component>_tests.cc`.
  - Test cases: `TEST_CASE("<Component> <behavior>")`.
- Fast unit tests should run under a minute locally. Long-running or integration tests go under a separate target.

# 13. Documentation

- Each public class/method should have Doxygen:
  - What it does, inputs/outputs, error cases, thread-safety notes.
  - Example usage if not obvious.
- Example template:

```cpp
//! Parses configuration for Foo from JSON.
//! @param js JSON object containing keys: name (string), retryCount (int)
//! @return Result<FooConfig> with error code on failure
//! @threadsafe No. Call from init thread only.
//! @example
//!   auto cfgRes = FooConfig::fromJson(js);
//!   if (!cfgRes.ok()) { SPDLOG_ERROR("Config error: {}", cfgRes.error().message); }
```

# 14. Logging examples

- Initialization (once per process):
- To avoid duplicate logs output error messages where the error is detected.

```cpp
// e.g., at main()
spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [%t] %v");
spdlog::set_level(spdlog::level::info); // use debug in dev profiles
```

- Usage:

```cpp
SPDLOG_INFO("Loaded {} items in {} ms", count, durationMs);
SPDLOG_WARN("Retry {} due to {}", attempt, errorMsg);
SPDLOG_ERROR("Failed to open file: {}", path.string());
```

# 15. Ownership and API design examples

- Use `std::span<const T>` for read-only buffers; document that the caller retains ownership.
- Prefer `std::unique_ptr<T>` for factory functions to express ownership transfer.
- For objects that are expensive to copy, consider using `std::shared_ptr<T>` or `std::weak_ptr<T>`.
- Accept `std::string_view` for read-only string parameters; copy into `std::string` only if you need to store it.
- Mark move-only types explicitly and default special members when appropriate.

# Tool notes
- For detailed notes on specific tools, see the companion documents next to this file.
- If a tool is particularly unclear, add a new notes file and put a reference here.
- cxxopts: see cxxopts_notes.md for patterns, examples, and error-handling guidance used in this repository.
