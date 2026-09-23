#include <GLFW/glfw3.h>
#include <cstdint>
#include <fstream>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>


#include "Shader.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "imgui.h"

int screen_width  = 1280;
int screen_height = 800;

struct Pixel
{
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;
        bool         operator!=(const Pixel &other) const { return r != other.r || g != other.g || b != other.b || a != other.a; }
        bool         operator==(const Pixel &other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
};

std::vector< Pixel > pixels(screen_width * screen_height, {});
float                vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,
                                   -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f};

inline void framebuffer_size_callback(GLFWwindow *window, int width, int height) { glViewport(0, 0, width, height); }

void set_pixel(int x, int y, Pixel color)
{
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
        return;
    pixels[y * screen_width + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, Pixel color)
{
    int dx  = abs(x1 - x0);
    int dy  = abs(y1 - y0);
    int sx  = (x0 < x1) ? 1 : -1;
    int sy  = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;

        int error2 = err * 2;
        if (error2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (error2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

std::stack< std::pair< std::pair< int, int >, std::pair< Pixel, Pixel > > > fill_stack; // x, y, fill color, target color

void fill_pixels()
{
    while(!fill_stack.empty()){
    int   x            = fill_stack.top().first.first;
    int   y            = fill_stack.top().first.second;
    Pixel color        = fill_stack.top().second.first;
    Pixel target_color = fill_stack.top().second.second;
    fill_stack.pop();
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
        continue;
    if (pixels[y * screen_width + x] != target_color || pixels[y * screen_width + x] == color)
        continue;
    pixels[y * screen_width + x] = color;
    fill_stack.push({{x + 1, y}, {color, target_color}});
    fill_stack.push({{x - 1, y}, {color, target_color}});
    fill_stack.push({{x, y + 1}, {color, target_color}});
    fill_stack.push({{x, y - 1}, {color, target_color}});
    }
    return;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const char *glsl_version = "#version 330";
    float       main_scale   = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    GLFWwindow *window = glfwCreateWindow(screen_width, screen_height, "draw board", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsLight();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFont *font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 30, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(font != nullptr);

    ImVec4 background_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 draw_color       = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, screen_width, screen_height);

    Shader shader("shaders/vertexShader.vert", "shaders/fragmentShader.frag");

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    shader.use();
    shader.setInt("drawboardTexture", 0);

    int mouse_x = -1, mouse_y = -1;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        static int e = -1;
        {
            ImGui::Begin("工具");
            ImGui::ColorEdit3("背景颜色", &background_color.x);
            ImGui::RadioButton("笔", &e, 0);
            ImGui::SameLine();
            ImGui::RadioButton("油漆桶", &e, 1);
            ImGui::SameLine();
            ImGui::RadioButton("橡皮", &e, 2);
            ImGui::SameLine();
            if (ImGui::Button("清空画布", ImVec2(100, 30)))
            {
                std::fill(pixels.begin(), pixels.end(), Pixel{});
            }
            if (e == 0 || e == 1)
                ImGui::ColorEdit3("绘制颜色", &draw_color.x);

            ImGui::End();
        }

        ImGui::Render();

        glClearColor(background_color.x, background_color.y, background_color.z, background_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (e == 0 || e == 2)
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                {
                    double xpos, ypos;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    Pixel color = e == 0 ? Pixel{(std::uint8_t) (draw_color.x * 255), (std::uint8_t) (draw_color.y * 255),
                                                 (std::uint8_t) (draw_color.z * 255), 255} :
                                           Pixel{};
                    if (mouse_x != -1 && mouse_y != -1)
                    {
                        draw_line(mouse_x, mouse_y, (int) xpos, (int) ypos, color);
                    }
                    else
                    {
                        set_pixel((int) xpos, (int) ypos, color);
                    }
                    mouse_x = (int) xpos;
                    mouse_y = (int) ypos;
                }
                else
                    mouse_x = mouse_y = -1;

            else if (e == 1)
            {
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                {
                    double xpos, ypos;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    if(xpos < 0 || xpos >= screen_width || ypos < 0 || ypos >= screen_height)
                        continue;
                    Pixel color = {(std::uint8_t) (draw_color.x * 255), (std::uint8_t) (draw_color.y * 255),
                                   (std::uint8_t) (draw_color.z * 255), 255};

                    fill_stack.push(std::make_pair(std::make_pair((int) xpos, (int) ypos),
                                                   std::make_pair(color, pixels[(int) ypos * screen_width + (int) xpos])));
                    fill_pixels();
                }
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_width, screen_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        shader.use();
        
        shader.setInt("drawboardTexture", 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
