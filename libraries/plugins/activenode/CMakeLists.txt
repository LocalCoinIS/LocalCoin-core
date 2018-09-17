file(GLOB HEADERS "include/graphene/activenode/*.hpp")

add_library( graphene_activenode
             activenode.cpp
           )

target_link_libraries( graphene_activenode graphene_chain graphene_app )
target_include_directories( graphene_activenode
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_activenode

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
