## Dependencies

Note about install commands:
- for Windows, we use [choco](https://chocolatey.org/install).
- for macOS, we use [brew](https://brew.sh/).
- In case of an error in cmake, make sure that the dependencies are on the PATH.


### Too Long, Didn't Install

This is a really long list of dependencies, and it's easy to mess up. That's why:

#### Docker
We have a Docker image that's already set up for you. See the [Docker instructions](./README_docker.md).

#### Setup-cpp

We have [setup-cpp](https://github.com/aminya/setup-cpp) that is a cross-platform tool to install all the compilers and dependencies on the system.

Please check [the setup-cpp documentation](https://github.com/aminya/setup-cpp) for more information.

For example, on Windows, you can run the following to install llvm, cmake, ninja, ccache, and cppcheck.
```ps1
# windows example (open shell as admin)
curl -LJO "https://github.com/aminya/setup-cpp/releases/download/v0.5.7/setup_cpp_windows.exe"
./setup_cpp_windows --compiler llvm --cmake true --ninja true --ccache true --cppcheck true

RefreshEnv.cmd # reload the environment
```

### Necessary Dependencies
1. A C++ compiler that supports C++17.
See [cppreference.com](https://en.cppreference.com/w/cpp/compiler_support)
to see which features are supported by each compiler.
The following compilers should work:

  * [gcc 7+](https://gcc.gnu.org/)
	<details>
	<summary>Install command</summary>

	- Debian/Ubuntu:

			sudo apt install build-essential

	- Windows:

			choco install mingw -y

	- MacOS:

			brew install gcc
	</details>

  * [clang 6+](https://clang.llvm.org/)
	<details>
	<summary>Install command</summary>

	- Debian/Ubuntu:

			bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"

	- Windows:

		Visual Studio 2019 ships with LLVM (see the Visual Studio section). However, to install LLVM separately:

			choco install llvm -y

		llvm-utils for using external LLVM with Visual Studio generator:

			git clone https://github.com/zufuliu/llvm-utils.git
			cd llvm-utils/VS2017
			.\install.bat

	- MacOS:

			brew install llvm
	</details>

  * [Visual Studio 2019 or higher](https://visualstudio.microsoft.com/)
	<details>
	<summary>Install command + Environment setup</summary>

	On Windows, you need to install Visual Studio 2019 because of the SDK and libraries that ship with it.

  	Visual Studio IDE - 2019 Community (installs Clang too):

  	  	choco install -y visualstudio2019community --package-parameters "add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --includeOptional --passive --locale en-US"

	Put MSVC compiler, Clang compiler, and vcvarsall.bat on the path:

			choco install vswhere -y
			refreshenv

			# change to x86 for 32bit
			$clpath = vswhere -products * -latest -prerelease -find **/Hostx64/x64/*
			$clangpath = vswhere -products * -latest -prerelease -find **/Llvm/bin/*
			$vcvarsallpath =  vswhere -products * -latest -prerelease -find **/Auxiliary/Build/*

			$path = [System.Environment]::GetEnvironmentVariable("PATH", "User")
			[Environment]::SetEnvironmentVariable("Path", $path + ";$clpath" + ";$clangpath" + ";$vcvarsallpath", "User")
			refreshenv

	</details>


2. [CMake 3.21+](https://cmake.org/)
	<details>
	<summary>Install Command</summary>

	- Debian/Ubuntu:

			sudo apt-get install cmake

	- Windows:

			choco install cmake -y

	- MacOS:

			brew install cmake

	</details>

### Optional Dependencies
#### C++ Tools
  * [Doxygen](http://doxygen.nl/)
	<details>
	<summary>Install Command</summary>

	- Debian/Ubuntu:

			sudo apt-get install doxygen
			sudo apt-get install graphviz

	- Windows:

			choco install doxygen.install -y
			choco install graphviz -y

	- MacOS:

			brew install doxygen
	 		brew install graphviz

	</details>


  * [ccache](https://ccache.dev/)
	<details>
	<summary>Install Command</summary>

	- Debian/Ubuntu:

			sudo apt-get install ccache

	- Windows:

			choco install ccache -y

	- MacOS:

			brew install ccache

	</details>


  * [Cppcheck](http://cppcheck.sourceforge.net/)
	<details>
	<summary>Install Command</summary>

	- Debian/Ubuntu:

			sudo apt-get install cppcheck

	- Windows:

			choco install cppcheck -y

	- MacOS:

			brew install cppcheck

	</details>


  * [include-what-you-use](https://include-what-you-use.org/)
	<details>
	<summary>Install Command</summary>

	Follow instructions here:
	https://github.com/include-what-you-use/include-what-you-use#how-to-install
	</details>

### Pinned dependencies

Some libraries that are downloaded at configure time only publish a moving branch, not
releases. Fetching a branch means a fresh build can break whenever upstream changes (for
example a new warning in a header, which `-Werror` turns into a build failure), so these
are pinned to an exact commit instead.

**Review these pins from time to time** (roughly quarterly, or when a newer upstream
feature or fix is wanted), so the project doesn't drift too far behind upstream.

| Library | Tracks | Pinned commit | Pinned on | Where |
|---|---|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) | `docking` | `144727fad6de` | 2026-11-28 | `cmake/SetupImGUI.cmake` |
| [ImPlot](https://github.com/epezent/implot) | `master` | `72de27832d1e` | 2026-11-28 | `cmake/SetupImPlot.cmake` |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | `master` | `ff3c0732fbec` | held back, see below | `cmake/SetupImGUI.cmake`, `cmake/SetupImGuizmo.cmake` |
| [gpmf-parser](https://github.com/gopro/gpmf-parser) | `main` | `40a3826be167` | 2026-11-28 | `Dependencies.cmake` |

ImGuizmo is deliberately held at the last commit before upstream moved its sources into
`src/` and added a root `CMakeLists.txt` defining a competing `imguizmo` target. Updating it
needs the build changes described in commit ff287e7, not just a new hash.

Libraries with tagged releases (bgfx.cmake, fmt, Catch2, ...) are pinned to the tag in
`Dependencies.cmake` and should be reviewed at the same time.

To update a pin:
1. Find the current head of the tracked branch, e.g. `git ls-remote https://github.com/ocornut/imgui.git refs/heads/docking`.
2. Replace the `GIT_TAG` hash and its comment, and update the date, in the file listed above.
3. Do a clean configure and build with the display stack on (the default), then run `ctest`.
4. Update the table here.
