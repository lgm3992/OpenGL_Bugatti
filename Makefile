# Makefile for OpenGL ES Examples
# glad, glfw, glm 라이브러리가 설치되어 있다고 가정

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
INCLUDES = -I./glad/include -I/usr/include
LIBS = -lglfw -lGL -ldl -lpthread

# GLAD 소스 파일
GLAD_SRC = ./glad/src/glad.c

# 기본 타겟
all: gles_example gles_obj_loader simple_triangle

# GLAD 오브젝트 파일 컴파일
glad.o: $(GLAD_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(GLAD_SRC) -o glad.o

# 메인 예제 컴파일 (내장 큐브)
gles_example: gles_example.cpp glad.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) gles_example.cpp glad.o $(LIBS) -o gles_example

# OBJ 파일 로더 예제
gles_obj_loader: gles_obj_loader.cpp glad.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) gles_obj_loader.cpp glad.o $(LIBS) -o gles_obj_loader

# 간단한 삼각형 예제
simple_triangle: simple_triangle.cpp glad.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) simple_triangle.cpp glad.o $(LIBS) -o simple_triangle

# 정리
clean:
	rm -f *.o gles_example gles_obj_loader simple_triangle

# 실행 (기본 큐브)
run: gles_example
	./gles_example

# OBJ 로더 실행
run-obj: gles_obj_loader
	./gles_obj_loader models/cube.obj

# 피라미드 실행
run-pyramid: gles_obj_loader
	./gles_obj_loader models/pyramid.obj

# 삼각형 실행
run-triangle: simple_triangle
	./simple_triangle

.PHONY: all clean run run-obj run-pyramid run-triangle
