# General

Keep the code and comments clean and readable, and in the same style as the rest of the project.
The best code is kept as simple as possible. Requirements often change, so we don't want to add code we don't need.
If there is doubt about the best way to proceed, stop and ask.
Prefer to avoid duplicating code when fixing bugs and issues.

## Libraries and tools

Logging, use spdlog and the default logger, with capitalized method names, e.g. SPDLOG_INFO("Message {}", var);
Add logging for critical failures and state transitions
The Eigen libray is used for vector and matrix math.
Prefer std::chrono for dealing with time.
Use Ravl2 types for image processing, in particular for image IO and file IO.
Use nlohmann_json::json for json processing.
Use libfmt, fmt::format("{}",var) for formating strings and printing, avoid std::cerr and std::cout.

## Langauge and Naming Conventions:

Use camelCase for method names and variables.
Use PascalCase for class names.
For all new code prefix member variables with 'm' (e.g., mPlanInterval) and make sure they are initialised.
Code uses C++20 standards.
Use descriptive names that indicate purpose and type.
Use comment style //! for c++ code with doxygen comments starting with @.
Include examples in the comments if the use of a function isn't clear.
Document classes and class methods with doxygen.
Intending is 2 spaces, no tabs.
Use '#pragma once' instead of include guards.
Properly manage resources with RAII principles.
Prefer modern C++ features std::span
Use smart pointers (std::shared_ptr, std::unique_ptr) over raw pointers
If function or class uses threading, be clear what methods are thread safe.
Try and keep code out of header files unless it is a template or very small inline methods.
Avoid 'using namespace ...' except to bring literals like std::chrono_literals into another scope.
C++ source files use the '.cc' extension, and C++ headers use '.hh'.

## Unit Testing

Write concise tests that cover required functionality with as few tests as possible.
Use catch2 test framework.
Include both normal and edge cases.
In this project, unit tests should be in a 'tests' subdirectory of the implementation.

