find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(image-viewer-tests
    image_test.cpp
)
target_link_libraries(image-viewer-tests PRIVATE viewer-core GTest::gtest_main)
target_compile_definitions(image-viewer-tests PRIVATE
    IMAGE_VIEWER_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
gtest_discover_tests(image-viewer-tests)

add_executable(image-viewer-capture capture.cpp)
target_link_libraries(image-viewer-capture PRIVATE viewer-ui)
target_compile_definitions(image-viewer-capture PRIVATE
    IMAGE_VIEWER_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
