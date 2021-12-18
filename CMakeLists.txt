cmake_minimum_required(VERSION 3.20)
project(bad-compiler)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# set(LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1)
set(CMAKE_CXX_STANDARD 17)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../llvm-project-build/include)


add_executable(bad-compiler main.cpp implement/Yacc/Grammar.cpp include/Yacc/GrammarParser.h implement/Yacc/GrammarParser.cpp implement/Yacc/Items.cpp include/Yacc/Items.h implement/Yacc/Constructor.cpp include/Yacc/Constructor.h implement/Yacc/utils.cpp include/Yacc/utils.h include/Parser/CSTBuilder.h implement/Parser/CSTBuilder.cpp include/Parser/CST.h implement/Parser/CST.cpp include/AST/Decl.h include/AST/Stmt.h include/AST/Type.h include/AST/Expr.h include/AST/Specifiers.h include/AST/ASTBuilder.h test.cpp implement/AST/ASTBuilder.cpp include/AST/AbstractASTNode.h include/AST/RecursiveASTVisitor.h include/ASTConsumers/ASTDumper.h implement/AST/RecursiveASTVisitor.cpp include/ASTConsumers/IRGenerator.h implement/CodeGen/IRGenerator.cpp)

# add this line to solve 'undefined reference to llvm::EnableABIBreakingChecks'
# target_link_libraries(bad-compiler LLVMSupport)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(bad-compiler ${llvm_libs})