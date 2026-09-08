include(FetchContent)

# Match the plugin's own runtime library setting on Windows/MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.18.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(unit-tests)
target_sources(
  unit-tests
  PRIVATE smoke-test.cpp timeline-test.cpp ${PROJECT_SOURCE_DIR}/src/core/timeline.cpp)
target_include_directories(unit-tests PRIVATE ${PROJECT_SOURCE_DIR}/src/core)
target_link_libraries(unit-tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit-tests)
