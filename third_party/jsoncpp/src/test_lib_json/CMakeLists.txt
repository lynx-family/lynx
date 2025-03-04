# vim: et ts=4 sts=4 sw=4 tw=0

ADD_EXECUTABLE( jsoncpp_test 
                jsontest.cpp
                jsontest.h
                main.cpp
                )


IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS( -DJSON_DLL )
    TARGET_LINK_LIBRARIES(jsoncpp_test jsoncpp_lib)
ELSE(BUILD_SHARED_LIBS)
    TARGET_LINK_LIBRARIES(jsoncpp_test jsoncpp_lib_static)
ENDIF()

# another way to solve issue #90
#set_target_properties(jsoncpp_test PROPERTIES COMPILE_FLAGS -ffloat-store)

# Run unit tests in post-build
# (default cmake workflow hides away the test result into a file, resulting in poor dev workflow?!?)
IF(JSONCPP_WITH_POST_BUILD_UNITTEST)
    IF(BUILD_SHARED_LIBS)
        # First, copy the shared lib, for Microsoft.
        # Then, run the test executable.
        ADD_CUSTOM_COMMAND( TARGET jsoncpp_test
                            POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:jsoncpp_lib> $<TARGET_FILE_DIR:jsoncpp_test>
                            COMMAND $<TARGET_FILE:jsoncpp_test>)
    ELSE(BUILD_SHARED_LIBS)
        # Just run the test executable.
        ADD_CUSTOM_COMMAND( TARGET jsoncpp_test
                            POST_BUILD
                            COMMAND $<TARGET_FILE:jsoncpp_test>)
    ENDIF()
ENDIF()

SET_TARGET_PROPERTIES(jsoncpp_test PROPERTIES OUTPUT_NAME jsoncpp_test) 
