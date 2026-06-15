CXX = g++
CXXFLAGS = -std=c++14 -O2 \
           -I. \
           -Ilibs/imgui-master \
           -Ilibs/imgui-master/backends

LDFLAGS = -lGL -lglut -lGLEW -lGLU -lglfw

SRCS = \
	main.cpp \
	matrixHelper.cpp \
	Model.cpp \
	Camera.cpp \
	common/GLShader.cpp \
	libs/imgui-master/imgui.cpp \
	libs/imgui-master/imgui_draw.cpp \
	libs/imgui-master/imgui_tables.cpp \
	libs/imgui-master/imgui_widgets.cpp \
	libs/imgui-master/backends/imgui_impl_glfw.cpp \
	libs/imgui-master/backends/imgui_impl_opengl3.cpp

OBJS = $(SRCS:.cpp=.o)

main: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main