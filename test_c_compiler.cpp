// Test C Compiler Interface
#include "core/c_compiler_interface.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::cout << "Testing C Compiler Interface...\n\n";
    
    // Create test directory
    fs::path test_dir = fs::temp_directory_path() / "aria_make_c_test";
    fs::create_directories(test_dir);
    
    // Create simple C source file
    fs::path source_file = test_dir / "test.c";
    {
        std::ofstream out(source_file);
        out << "#include <stdio.h>\n";
        out << "int add(int a, int b) {\n";
        out << "    return a + b;\n";
        out << "}\n";
        out << "int main() {\n";
        out << "    printf(\"5 + 3 = %d\\n\", add(5, 3));\n";
        out << "    return 0;\n";
        out << "}\n";
    }
    std::cout << "Created test.c\n";
    
    try {
        // Test 1: Compile to executable
        aria_make::CCompilerInterface gcc("/usr/bin/gcc", false);
        
        std::cout << "\nGCC version:\n" << gcc.get_version() << "\n\n";
        
        aria_make::CCompilerInterface::CompileTask compile_task;
        compile_task.sources = {source_file.string()};
        compile_task.output = (test_dir / "test_exe").string();
        compile_task.compile_only = false;
        
        std::cout << "Compiling test.c -> test_exe...\n";
        auto result = gcc.compile(compile_task);
        
        if (result.exit_code == 0) {
            std::cout << "✅ Compilation successful (" << result.duration.count() << "ms)\n";
        } else {
            std::cout << "❌ Compilation failed (exit " << result.exit_code << ")\n";
            std::cout << "STDERR:\n" << result.stderr_output << "\n";
            return 1;
        }
        
        // Test 2: Compile to object file
        aria_make::CCompilerInterface::CompileTask obj_task;
        obj_task.sources = {source_file.string()};
        obj_task.output = (test_dir / "test.o").string();
        obj_task.compile_only = true;
        
        std::cout << "\nCompiling test.c -> test.o...\n";
        result = gcc.compile(obj_task);
        
        if (result.exit_code == 0) {
            std::cout << "✅ Object file created (" << result.duration.count() << "ms)\n";
        } else {
            std::cout << "❌ Object compilation failed (exit " << result.exit_code << ")\n";
            std::cout << "STDERR:\n" << result.stderr_output << "\n";
            return 1;
        }
        
        // Test 3: Create static library
        aria_make::CCompilerInterface::LibraryTask lib_task;
        lib_task.objects = {(test_dir / "test.o").string()};
        lib_task.output = (test_dir / "libtest.a").string();
        
        std::cout << "\nCreating static library libtest.a...\n";
        result = gcc.create_static_library(lib_task);
        
        if (result.exit_code == 0) {
            std::cout << "✅ Static library created (" << result.duration.count() << "ms)\n";
        } else {
            std::cout << "❌ Static library creation failed (exit " << result.exit_code << ")\n";
            std::cout << "STDERR:\n" << result.stderr_output << "\n";
            return 1;
        }
        
        std::cout << "\n✅ All tests passed!\n";
        
        // Cleanup
        fs::remove_all(test_dir);
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        fs::remove_all(test_dir);
        return 1;
    }
}
