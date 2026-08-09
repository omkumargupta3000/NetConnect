add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE netconnect_core)

add_test(NAME UtilsTests COMMAND test_utils)
