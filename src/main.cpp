#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <time.h>
#include <vector>
#include <thread>
#include <future>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

pid_t startJob(std::string filename, std::string outputDir, int seed, int image_size, int num_images, float cam_dist, int offset) {
    pid_t pid = fork(); // 0 for child, child pid in parent
    
    if (pid == 0) {
        execl("./blender/.venv/bin/blenderproc",
                "blenderproc",
                "run",
                filename.c_str(),
                "--output-dir",
                outputDir.c_str(),
                "--seed",
                std::to_string(seed).c_str(),
                "--image-size",
                std::to_string(image_size).c_str(),
                "--num-images",
                std::to_string(num_images).c_str(),
                "--camera-distance",
                std::to_string(cam_dist).c_str(),
                "--num-offset",
                std::to_string(offset).c_str(),
                (char *)nullptr);

        perror("Failed to launch BlenderProc");
        exit(EXIT_FAILURE);
    } else {
        return pid;
    }

}

bool waitForProcesses(std::vector<pid_t> v) {
    for (pid_t pid : v) {
        int status;
        waitpid(pid, &status, 0);
    }

    return true;
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Select GL version + let the backend select a GLSL version
    const char* glsl_version = nullptr;
    
    #if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100 (WebGL 1.0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(IMGUI_IMPL_OPENGL_ES3)
        // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(__APPLE__)
        // GL 3.2 + generally GLSL 150
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    #else
        // GL 3.0 + generally GLSL 130
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    #endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Dataset Generation GUI", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //style.FontSizeBase = 16.0f;           // Default to 13.0f when unspecified, which allows crisp legacy font for non-4K screens.
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;

    // Setup scaling
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            std::vector<pid_t> pids;

            
            static bool running = false;
            static float progress = 0.0f;

            static int process_count = 1;
            static int img_res = 256;
            static int num_images = 10;
            static float cam_dist = 2.5f;

            static std::string output_dir { "output" };
            static std::future<bool> fut;
            
            ImGui::Begin("Dataset Generation");

            ImGui::InputText("Output Directory", &output_dir);
            ImGui::InputInt("Image Resolution (W & H)", &img_res);
            ImGui::InputInt("# Images (will be split across processes)", &num_images);
            ImGui::InputFloat("Camera Distance", &cam_dist);

            ImGui::InputInt("# of processes", &process_count);

            ImGui::BeginDisabled(running);
            if (ImGui::Button("Start Jobs")) {
                running = true;

                for (int i = 0; i < process_count; ++i) {
                    pid_t pid = startJob("./blender/test_render.py", "output/" + output_dir, rand(), img_res, num_images / process_count, cam_dist, i);
                    pids.push_back(pid);
                }

                fut = std::async(std::launch::async, waitForProcesses, std::move(pids));
            }
            ImGui::EndDisabled();

            if (running && fut.valid() && fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                bool ok = fut.get();
                running = false;
                progress = 1;
                std::cout << "FINISHED THEM ALL" << std::endl;
            } 

            if (running) {
                ImGui::ProgressBar(progress);
                progress += 1 / progress;

                ImGui::Text("Running!!!");
            }

                       
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

//clang++ -std=c++17 -pthread src/main.cpp -o generator
//./generator
