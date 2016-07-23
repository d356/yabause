cmake_minimum_required(VERSION 2.8)
project(CodeGen CXX)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG(-std=c++11 COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG(-std=c++0x COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(CodeGen
    src/AArch32Assembler.cpp
	src/AArch64Assembler.cpp
	src/Jitter_CodeGen_AArch32.cpp
	src/Jitter_CodeGen_AArch32_64.cpp
	src/Jitter_CodeGen_AArch32_Fpu.cpp
	src/Jitter_CodeGen_AArch32_Md.cpp
	src/Jitter_CodeGen_AArch64.cpp
	src/Jitter_CodeGen_AArch64_64.cpp
	src/Jitter_CodeGen_AArch64_Fpu.cpp
	src/Jitter_CodeGen_AArch64_Md.cpp
	src/X86Assembler.cpp
	src/X86Assembler_Fpu.cpp
	src/X86Assembler_Sse.cpp
	src/Jitter_CodeGen_x86_32.cpp
	src/Jitter_CodeGen_x86_64.cpp
	src/Jitter_CodeGen_x86.cpp
	src/Jitter_CodeGen_x86_Fpu.cpp
	src/Jitter_CodeGen_x86_Md.cpp
	src/CoffObjectFile.cpp
	src/Jitter_CodeGen.cpp
	src/Jitter_CodeGenFactory.cpp
	src/Jitter.cpp
	src/Jitter_Optimize.cpp
	src/Jitter_RegAlloc.cpp
	src/Jitter_Statement.cpp
	src/Jitter_SymbolTable.cpp
	src/MachoObjectFile.cpp
	src/MemoryFunction.cpp
	src/ObjectFile.cpp
    src/MemStream.cpp
    src/Stream.cpp
)

include_directories(include)