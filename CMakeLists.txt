cmake_minimum_required(VERSION 3.10)
project(OpenGLES_Examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW, GLAD, GLM 라이브러리 찾기
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# GLAD 소스 파일 추가 (보통 프로젝트에 포함되어 있음)
# glad 폴더 구조: glad/include/glad/glad.h, glad/include/KHR/khrplatform.h, glad/src/glad.c
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/glad")
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# 예제 실행 파일 생성
add_executable(gles_example gles_example.cpp)
add_executable(gles_obj_loader gles_obj_loader.cpp)
add_executable(simple_triangle simple_triangle.cpp)

# 라이브러리 링크
target_link_libraries(gles_example 
    glad
    glfw
    glm::glm
    ${CMAKE_DL_LIBS}
)

target_link_libraries(gles_obj_loader 
    glad
    glfw
    glm::glm
    ${CMAKE_DL_LIBS}
)

target_link_libraries(simple_triangle 
    glad
    glfw
    glm::glm
    ${CMAKE_DL_LIBS}
)

# Include 디렉토리 설정
target_include_directories(gles_example PRIVATE 
    ${GLAD_DIR}/include
)

target_include_directories(gles_obj_loader PRIVATE 
    ${GLAD_DIR}/include
)

target_include_directories(simple_triangle PRIVATE 
    ${GLAD_DIR}/include
)

# models 디렉토리를 빌드 디렉토리로 복사
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})