#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <time.h>
#include <vector>
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

typedef struct JobParameters {
    std::string filename;
    std::string output_dir = "output";
    int seed = rand();
    int image_size = 256;
    int num_images = 10;
    int gpu_workers = 1;
    int cpu_workers = 2;
    int cpu_threads = 4;
    float cpu_speed = 0.60f;
    bool use_cpu = false;
    float cam_dist = 2.5;
    float yaw_min = -(M_PI / 2);
    float yaw_max = M_PI / 2;
    float pitch_min = -(M_PI / 2);
    float pitch_max = M_PI / 2;
    float roll_min = -(M_PI / 2);
    float roll_max = M_PI / 2;
    float x_min = -0.5;
    float x_max = 0.5;
    float y_min = -0.5;
    float y_max = 0.5;
    float z_min = -0.5;
    float z_max = 0.5;
    float floor_hue_min = 0.0;
    float floor_hue_max = 1.0;
    float spot_theta_min = 0.0;
    float spot_theta_max = M_PI / 4;
    float spot_phi_min = 0.0;
    float spot_phi_max = 2 * M_PI;
    float spot_hue_min = 0.0;
    float spot_hue_max = 1.0;
    std::string model_category = "02691156";
    std::string model_id = "";
} JobParams;

struct Job {
    std::string name;
    pid_t pid;
    std::atomic<int> finished = 0;
    int total = 0;
    JobParams JP;
    std::atomic<bool> running = false;
};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void startJob(Job& J) {
    setenv("ROCR_VISIBLE_DEVICES", "0", 1);

    FILE *fp;
    int status;
    char path[PATH_MAX];

    std::string env = J.JP.use_cpu
    ? "HIP_VISIBLE_DEVICES=-1 "
    : "ROCR_VISIBLE_DEVICES=0 ";

    std::string output_dir = "output/" + J.name;

    std::string cmd = "./blender/.venv/bin/blenderproc run " + J.JP.filename //This differs between us
        + " --output-dir " + output_dir
        + " --seed " + std::to_string(J.JP.seed)
        + " --image-size " + std::to_string(J.JP.image_size)
        + " --num-images " + std::to_string(J.JP.num_images)
        + " --camera-distance " + std::to_string(J.JP.cam_dist)
        + " --yaw-min " + std::to_string(J.JP.yaw_min)
        + " --yaw-max " + std::to_string(J.JP.yaw_max)
        + " --pitch-min " + std::to_string(J.JP.pitch_min)
        + " --pitch-max " + std::to_string(J.JP.pitch_max)
        + " --roll-min " + std::to_string(J.JP.roll_min)
        + " --roll-max " + std::to_string(J.JP.roll_max)
        + " --x-min " + std::to_string(J.JP.x_min)
        + " --x-max " + std::to_string(J.JP.x_max)
        + " --y-min " + std::to_string(J.JP.y_min)
        + " --y-max " + std::to_string(J.JP.y_max)
        + " --z-min " + std::to_string(J.JP.z_min)
        + " --z-max " + std::to_string(J.JP.z_max)
        + " --floor-hue-min " + std::to_string(J.JP.floor_hue_min)
        + " --floor-hue-max " + std::to_string(J.JP.floor_hue_max)
        + " --spot-theta-min " + std::to_string(J.JP.spot_theta_min)
        + " --spot-theta-max " + std::to_string(J.JP.spot_theta_max)
        + " --spot-phi-min " + std::to_string(J.JP.spot_phi_min)
        + " --spot-phi-max " + std::to_string(J.JP.spot_phi_max)
        + " --spot-hue-min " + std::to_string(J.JP.spot_hue_min)
        + " --spot-hue-max " + std::to_string(J.JP.spot_hue_max)
        + " --model-category " + J.JP.model_category
        + " --device " + (J.JP.use_cpu ? "cpu" : "gpu")
        + " --cpu-threads " + std::to_string(J.JP.cpu_threads);

    // Only pass --model-id if one was actually entered in the GUI.
    // i will eventually get around to fixing this, probably when i make chanegs to model selection in the GUI
    if (!J.JP.model_id.empty()) {
        cmd += " --model-id " + J.JP.model_id;
    }

    J.running = true;

    fp = popen(cmd.c_str(), "r");

    if (fp == NULL) {
        std::cerr << "Something went wrong." << std::endl;
        J.running = false;
        return; // fgets on a NULL FILE* would crash
    }

    while (fgets(path, PATH_MAX, fp) != NULL) {
        if (path[0] == '@') {
            J.finished++;
        }
    }

    J.running = false;
    status = pclose(fp);

    if (status == -1) {
        std::cerr << "Something went wrong." << std::endl;
    } else {
        std::cerr << "Should be ok" << std::endl;
    }
}

bool checkFinished(std::vector<std::future<void>> &f) {
    for (auto &F : f) {
        if (F.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return false;
        }
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

    std::vector<std::unique_ptr<Job>> jobs;
    std::vector<std::future<void>> futures;
    std::time_t begin_time;

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
            static JobParams JP {};

            static bool running = false;
            static float progress = 0.0f;

            static std::future<bool> fut;
            
            ImGui::Begin("Dataset Generation");

            if (ImGui::CollapsingHeader("Parser Arguments")) {
                ImGui::InputText("Output Directory", &JP.output_dir);
                ImGui::InputInt("Image Resolution (W & H)", &JP.image_size);
                ImGui::InputInt("# Images (will be split across processes)", &JP.num_images);
                ImGui::InputFloat("Camera Distance", &JP.cam_dist);
                ImGui::InputInt("# GPU Workers", &JP.gpu_workers);
                ImGui::InputInt("# CPU Workers", &JP.cpu_workers);
                ImGui::InputInt("# CPU Threads per Job (keep low)", &JP.cpu_threads);
                ImGui::InputFloat("CPU Speed (0.5 being half as good as the GPU)", &JP.cpu_speed);
                // ImGui::InputInt("# of processes", &JP.process_count);
            }
            
            if (ImGui::CollapsingHeader("Rotation Argument Ranges")) {
                ImGui::InputFloat("Yaw Min", &JP.yaw_min);
                ImGui::InputFloat("Yaw Max", &JP.yaw_max);

                ImGui::InputFloat("Pitch Min", &JP.pitch_min);
                ImGui::InputFloat("Pitch Max", &JP.pitch_max);

                ImGui::InputFloat("Roll Min", &JP.roll_min);
                ImGui::InputFloat("Roll Max", &JP.roll_max);

            }

            if (ImGui::CollapsingHeader("Translation Argument Ranges")) {
                ImGui::InputFloat("X Min", &JP.x_min);
                ImGui::InputFloat("X Max", &JP.x_max);

                ImGui::InputFloat("Y Min", &JP.y_min);
                ImGui::InputFloat("Y Max", &JP.y_max);

                ImGui::InputFloat("Z Min", &JP.z_min);
                ImGui::InputFloat("Z Max", &JP.z_max);
            }

            if (ImGui::CollapsingHeader("Lighting Argument Ranges")) {
                ImGui::InputFloat("Floor Hue Min", &JP.floor_hue_min);
                ImGui::InputFloat("Floor Hue Max", &JP.floor_hue_max);

                ImGui::InputFloat("Spot Theta Min", &JP.spot_theta_min);
                ImGui::InputFloat("Spot Theta Max", &JP.spot_theta_max);

                ImGui::InputFloat("Spot Phi Min", &JP.spot_phi_min);
                ImGui::InputFloat("Spot Phi Max", &JP.spot_phi_max);

                ImGui::InputFloat("Spot Hue Min", &JP.spot_hue_min);
                ImGui::InputFloat("Spot Hue Max", &JP.spot_hue_max);
            }

            if (ImGui::CollapsingHeader("Model Arguments")) {
                ImGui::InputText("Model Category", &JP.model_category);
                ImGui::InputText("Model ID", &JP.model_id);
            }

            ImGui::BeginDisabled(running);
            if (ImGui::Button("Start Jobs")) {
                begin_time = std::time(nullptr);

                running = true;
                JP.filename = "./blender/test_render.py";
                
                int n_workers = JP.cpu_workers + JP.gpu_workers;
                float total_weight = JP.gpu_workers + JP.cpu_workers * JP.cpu_speed;
                int assigned = 0;

                for (int i = 0; i < n_workers; ++i) {
                    JobParams curr_jp = JP;
                    curr_jp.use_cpu = (i < JP.cpu_workers);
                    curr_jp.output_dir = curr_jp.output_dir + std::to_string(i);
                    curr_jp.seed = JP.seed + 1;

                    float w = curr_jp.use_cpu ? JP.cpu_speed : 1.0f;
                    curr_jp.num_images = (i == n_workers - 1) ? JP.num_images - assigned : (int)(JP.num_images * w / total_weight);
                    assigned += curr_jp.num_images;

                    if (curr_jp.num_images == 0) continue;

                    std::string name = (curr_jp.use_cpu ? "CPU-" : "GPU-") + std::to_string(i);

                    auto j = std::make_unique<Job>(name ,0, 0, curr_jp.num_images, std::move(curr_jp));
                    
                    futures.push_back(std::async(std::launch::async, startJob, std::ref(*j)));
                    jobs.push_back(std::move(j)); // we want to hand over ownership of the Job to the vector
                }
            }
            ImGui::EndDisabled();

            if (running && checkFinished(futures)) {
                jobs.clear();
                futures.clear();

                running = false;
                progress = 1;
            } 

            if (running) {

                int finished_all = 0;
                int total_all = 0;
                // progress is equal to 
                for (auto& j: jobs) {
                    finished_all += j->finished;
                    total_all += j->total;
                }

                progress = (float)finished_all / (float)total_all;

                ImGui::ProgressBar(progress);
                ImGui::Text("Time Elapsed: %d s", (int)std::difftime(std::time(nullptr), begin_time));
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
