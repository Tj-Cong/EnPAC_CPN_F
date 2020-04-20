cmake_minimum_required(VERSION 3.13)
project(CPN_PNML_PARSE)

set(CMAKE_CXX_STANDARD 14)
set(LINK_DIR /usr/local/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread")

link_directories(${LINK_DIR})
link_libraries(tcmalloc)

include_directories(.)

add_executable(
        EnPAC_CPN_F
        main.cpp
        CPN.h
        CPN.cpp
        condition_tree.cpp
        condition_tree.h
        BA/tinystr.cpp
        BA/tinystr.h
        BA/tinyxml.cpp
        BA/tinyxml.h
        BA/tinyxmlerror.cpp
        BA/tinyxmlparser.cpp
        CPN_RG.h
        CPN_RG.cpp
        CPN_Product.h
        CPN_Product.cpp
        BA/Data_Structures.h
        BA/Data_Structures.cpp
        BA/Lexer.h
        BA/Lexer.cpp
        BA/SBA.h
        BA/SBA.cpp
        BA/Syntax_Tree.h
        BA/Syntax_Tree.cpp
        BA/TBA.h
        BA/TBA.cpp
        BA/TGBA.h
        BA/TGBA.cpp
        BA/xml2ltl.h
        BA/xml2ltl.cpp
        base.h)

target_link_libraries(EnPAC_CPN_F tcmalloc)