antlr_target(FegenLexer FegenLexer.g4
            PACKAGE fegen
            LEXER
            )

antlr_target(FegenParser FegenParser.g4
            PACKAGE fegen
            DEPENDS_ANTLR FegenLexer
            PARSER
            LISTENER
            VISITOR
            COMPILE_FLAGS -lib 
            ${ANTLR_FegenLexer_OUTPUT_DIR}
            )

include_directories(${ANTLR_FegenLexer_OUTPUT_DIR}) 
set(ANTLR_FegenLexer_OUTPUT_DIR ${ANTLR_FegenLexer_OUTPUT_DIR} CACHE STRING "ANTLR_FegenLexer_OUTPUT_DIR")
include_directories(${ANTLR_FegenParser_OUTPUT_DIR})
set(ANTLR_FegenParser_OUTPUT_DIR ${ANTLR_FegenParser_OUTPUT_DIR} CACHE STRING "ANTLR_FegenParser_OUTPUT_DIR")

add_library(fegen_antlr_generated 
    ${ANTLR_FegenLexer_CXX_OUTPUTS} 
    ${ANTLR_FegenParser_CXX_OUTPUTS}
)
add_dependencies(fegen_antlr_generated antlr4_runtime)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
add_library(FegenSupport
    FegenManager.cpp
    Scope.cpp
)
add_dependencies(FegenSupport fegen_antlr_generated)

llvm_map_components_to_libnames(llvm_libs support)

target_link_libraries(FegenSupport
    PRIVATE
    ${llvm_libs}
)

add_library(fegenVisitor
    FegenVisitor.cpp
)

target_link_libraries(fegenVisitor
    PUBLIC
    fegen_antlr_generated
    antlr4_static
    FegenSupport
)