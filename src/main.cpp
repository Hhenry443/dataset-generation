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
#include <math.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

pid_t startJob(std::string filename, std::string outputDir, int seed, int image_size, int num_images, float cam_dist, 
        float yaw_min, float yaw_max, float pitch_min, float pitch_max, float roll_min, float roll_max, 
        float x_min, float x_max, float y_min, float y_max, float z_min, float z_max,
        float floor_hue_min, float floor_hue_max, float spot_theta_min, float spot_theta_max, float spot_phi_min, float spot_phi_max, float spot_hue_min, float spot_hue_max,
        std::string model_category, std::string model_id = "") {
    pid_t pid = fork(); // 0 for child, child pid in parent
    
    if (pid == 0) {
        if (model_id != "") {
            execl("./.venv/bin/blenderproc", //This differs between us
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
                "--yaw-min",
                std::to_string(yaw_min).c_str(),
                "--yaw-max",
                std::to_string(yaw_max).c_str(),
                "--pitch-min",
                std::to_string(pitch_min).c_str(),
                "--pitch-max",
                std::to_string(pitch_max).c_str(),
                "--roll-min",
                std::to_string(roll_min).c_str(),
                "--roll-max",
                std::to_string(roll_max).c_str(),
                "--x-min",
                std::to_string(x_min).c_str(),
                "--x-max",
                std::to_string(x_max).c_str(),
                "--y-min",
                std::to_string(y_min).c_str(),
                "--y-max",
                std::to_string(y_max).c_str(),
                "--z-min",
                std::to_string(z_min).c_str(),
                "--z-max",
                std::to_string(z_max).c_str(),
                "--floor-hue-min",
                std::to_string(floor_hue_min).c_str(),
                "--floor-hue-max",
                std::to_string(floor_hue_max).c_str(),
                "--spot-theta-min",
                std::to_string(spot_theta_min).c_str(),
                "--spot-theta-max",
                std::to_string(spot_theta_max).c_str(),
                "--spot-phi-min",
                std::to_string(spot_phi_min).c_str(),
                "--spot-phi-max",
                std::to_string(spot_phi_max).c_str(),
                "--spot-hue-min",
                std::to_string(spot_hue_min).c_str(),
                "--spot-hue-max",
                std::to_string(spot_hue_max).c_str(),
                "--model-category",
                model_category.c_str(),
                "--model-id",
                model_id.c_str(),
                (char *)nullptr);
        } else {
            execl("./.venv/bin/blenderproc", //This differs between us
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
                "--yaw-min",
                std::to_string(yaw_min).c_str(),
                "--yaw-max",
                std::to_string(yaw_max).c_str(),
                "--pitch-min",
                std::to_string(pitch_min).c_str(),
                "--pitch-max",
                std::to_string(pitch_max).c_str(),
                "--roll-min",
                std::to_string(roll_min).c_str(),
                "--roll-max",
                std::to_string(roll_max).c_str(),
                "--x-min",
                std::to_string(x_min).c_str(),
                "--x-max",
                std::to_string(x_max).c_str(),
                "--y-min",
                std::to_string(y_min).c_str(),
                "--y-max",
                std::to_string(y_max).c_str(),
                "--z-min",
                std::to_string(z_min).c_str(),
                "--z-max",
                std::to_string(z_max).c_str(),
                "--floor-hue-min",
                std::to_string(floor_hue_min).c_str(),
                "--floor-hue-max",
                std::to_string(floor_hue_max).c_str(),
                "--spot-theta-min",
                std::to_string(spot_theta_min).c_str(),
                "--spot-theta-max",
                std::to_string(spot_theta_max).c_str(),
                "--spot-phi-min",
                std::to_string(spot_phi_min).c_str(),
                "--spot-phi-max",
                std::to_string(spot_phi_max).c_str(),
                "--spot-hue-min",
                std::to_string(spot_hue_min).c_str(),
                "--spot-hue-max",
                std::to_string(spot_hue_max).c_str(),
                "--model-category",
                model_category.c_str(),
                (char *)nullptr);

        }
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

    srand(time(nullptr)); //Newly added to test, also present on line 373

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

            // PARSER ARGUMENTS
            static int process_count = 1;
            static int img_res = 256;
            static int num_images = 10;
            static float cam_dist = 2.5f;
            static std::string output_dir { "output" };

            // ROTATION ARGUMENT RANGES
            static float yaw_min = -(M_PI / 2);
            static float yaw_max = M_PI / 2;
            
            static float pitch_min = -(M_PI / 2);
            static float pitch_max = M_PI / 2;

            static float roll_min = -(M_PI / 2);
            static float roll_max = M_PI / 2;

            // Translation Argument Ranges
            static float x_min = -0.5;
            static float x_max = 0.5;
            
            static float y_min = -0.5;
            static float y_max = 0.5;

            static float z_min = -0.5;
            static float z_max = 0.5;
            
            // Lighting Argument Ranges
            static float floor_hue_min = 0.0f;
            static float floor_hue_max = 1.0f;

            static float spot_theta_min = 0.0f;
            static float spot_theta_max = M_PI / 4;

            static float spot_phi_min = .0f;
            static float spot_phi_max = 2 * M_PI;

            static float spot_hue_min = 0.0f;
            static float spot_hue_max = 1.0f;

            // Model Arguments
            static std::string model_category = "02691156";
            static std::string model_id;

            static std::future<bool> fut;
            
            ImGui::Begin("Dataset Generation");

            if (ImGui::CollapsingHeader("Parser Arguments")) {
                ImGui::InputText("Output Directory", &output_dir);
                ImGui::InputInt("Image Resolution (W & H)", &img_res);
                ImGui::InputInt("# Images (will be split across processes)", &num_images);
                ImGui::InputFloat("Camera Distance", &cam_dist);
                ImGui::InputInt("# of processes", &process_count);
            }
            
            if (ImGui::CollapsingHeader("Rotation Argument Ranges")) {
                ImGui::InputFloat("Yaw Min", &yaw_min);
                ImGui::InputFloat("Yaw Max", &yaw_max);

                ImGui::InputFloat("Pitch Min", &pitch_min);
                ImGui::InputFloat("Pitch Max", &pitch_max);

                ImGui::InputFloat("Roll Min", &roll_min);
                ImGui::InputFloat("Roll Max", &roll_max);

            }

            if (ImGui::CollapsingHeader("Translation Argument Ranges")) {
                ImGui::InputFloat("X Min", &x_min);
                ImGui::InputFloat("X Max", &x_max);

                ImGui::InputFloat("Y Min", &y_min);
                ImGui::InputFloat("Y Max", &y_max);

                ImGui::InputFloat("Z Min", &z_min);
                ImGui::InputFloat("Z Max", &z_max);
            }

            if (ImGui::CollapsingHeader("Lighting Argument Ranges")) {
                ImGui::InputFloat("Floor Hue Min", &floor_hue_min);
                ImGui::InputFloat("Floor Hue Max", &floor_hue_max);

                ImGui::InputFloat("Spot Theta Min", &spot_theta_min);
                ImGui::InputFloat("Spot Theta Max", &spot_theta_max);

                ImGui::InputFloat("Spot Phi Min", &spot_phi_min);
                ImGui::InputFloat("Spot Phi Max", &spot_phi_max);

                ImGui::InputFloat("Spot Hue Min", &spot_hue_min);
                ImGui::InputFloat("Spot Hue Max", &spot_hue_max);
            }

            if (ImGui::CollapsingHeader("Model Arguments")) {
                ImGui::InputText("Model Category", &model_category);
                ImGui::InputText("Model ID", &model_id);
            }

            ImGui::BeginDisabled(running);
            if (ImGui::Button("Start Jobs")) {
                running = true;

                for (int i = 0; i < process_count; ++i) {

                    pid_t pid;
                    if (!model_id.empty()) {
                        pid = startJob("./blender/test_render.py", "output/" + output_dir + std::to_string(i), rand(), img_res, num_images, cam_dist, yaw_min, yaw_max, pitch_min, pitch_max, roll_min, roll_max, x_min, x_max, y_min, y_max, z_min, z_max, floor_hue_min, floor_hue_max, spot_theta_min, spot_theta_max, spot_phi_min, spot_phi_max, spot_hue_min, spot_hue_max, model_category, model_id);
                    } else {
                        pid = startJob("./blender/test_render.py", "output/" + output_dir + std::to_string(i), rand(), img_res, num_images, cam_dist, yaw_min, yaw_max, pitch_min, pitch_max, roll_min, roll_max, x_min, x_max, y_min, y_max, z_min, z_max, floor_hue_min, floor_hue_max, spot_theta_min, spot_theta_max, spot_phi_min, spot_phi_max, spot_hue_min, spot_hue_max, model_category);
                    }
                    pids.push_back(pid);
                }

                fut = std::async(std::launch::async, waitForProcesses, std::move(pids));
            }
            ImGui::EndDisabled();

            if (running && fut.valid() && fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
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

// cmake -S . -B build
// cmake --build build
// ./build/ds-gen

// cmake --build build && ./build/ds-gen
