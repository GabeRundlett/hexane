#include <daxa/daxa.hpp>
#include <daxa/types.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <iostream>
#include <span>
#include <variant>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#define GLM_SWIZZLE
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi
#include <glm/gtc/type_ptr.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#if !defined(DAXA_SHADER_INCLUDE_DIR)
#define DAXA_SHADER_INCLUDE_DIR "include"
#endif

daxa::NativeWindowHandle get_native_handle(GLFWwindow * glfw_window_ptr)
{
#if defined(_WIN32)
    return glfwGetWin32Window(glfw_window_ptr);
#elif defined(__linux__)
    return reinterpret_cast<daxa::NativeWindowHandle>(glfwGetX11Window(glfw_window_ptr));
#endif
}

daxa::NativeWindowPlatform get_native_platform(GLFWwindow *)
{
#if defined(_WIN32)
    return daxa::NativeWindowPlatform::WIN32_API;
#elif defined(__linux__)
    return daxa::NativeWindowPlatform::XLIB_API;
#endif
}

struct WindowInfo
{
    daxa::u32 width, height;
    bool swapchain_out_of_date = false;
};

#include <hexane/shared.inl>

void raytrace_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::RasterPipeline> & raytrace_pipeline,
    std::shared_ptr<daxa::ComputePipeline> & prepare_pipeline,
    daxa::BufferId perframe_id,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::ImageId swapchain_image,
    daxa::u32 width, daxa::u32 height
);
void upload_perframe_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    daxa::BufferId buffer_id,
    Perframe perframe
);
void queue_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & queue_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
);
void brush_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & brush_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
);
void compressor_palettize_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_palettize_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
);
void compressor_allocate_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_allocate_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
);
void compressor_write_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_write_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
);
void clear_workspace_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    daxa::ImageId workspace_id
);

static bool locked = false;
static bool skip = true;
static double last_x_pos = 0;
static double last_y_pos = 0;
static double x_move = 0;
static double y_move = 0;

static void cursor_position_callback(GLFWwindow* window, double x_pos, double y_pos)
{  
    if(!skip && locked) {
        x_move += x_pos - last_x_pos;
        y_move += y_pos - last_y_pos;
    }
    skip = false;
    last_x_pos = x_pos;
    last_y_pos = y_pos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        locked = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

int main()
{
    auto window_info = WindowInfo{.width = 800, .height = 600};
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto glfw_window_ptr = glfwCreateWindow(
        static_cast<daxa::i32>(window_info.width),
        static_cast<daxa::i32>(window_info.height),
        "Hexane", nullptr, nullptr);
    glfwSetWindowUserPointer(glfw_window_ptr, &window_info);
    glfwSetWindowSizeCallback(
        glfw_window_ptr,
        [](GLFWwindow * glfw_window, int width, int height)
        {
            auto & window_info_ref = *reinterpret_cast<WindowInfo *>(glfwGetWindowUserPointer(glfw_window));
            window_info_ref.swapchain_out_of_date = true;
            window_info_ref.width = static_cast<daxa::u32>(width);
            window_info_ref.height = static_cast<daxa::u32>(height);
        });
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(glfw_window_ptr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetCursorPosCallback(glfw_window_ptr, cursor_position_callback);
    glfwSetMouseButtonCallback(glfw_window_ptr, mouse_button_callback);
    auto native_window_handle = get_native_handle(glfw_window_ptr);

    daxa::Context context = daxa::create_context({.enable_validation = false});

    daxa::Device device = context.create_device({
        .selector = [](daxa::DeviceProperties const & device_props) -> daxa::i32
        {
            daxa::i32 score = 0;
            switch (device_props.device_type)
            {
            case daxa::DeviceType::DISCRETE_GPU: score += 10000; break;
            case daxa::DeviceType::VIRTUAL_GPU: score += 1000; break;
            case daxa::DeviceType::INTEGRATED_GPU: score += 100; break;
            default: break;
            }
            score += static_cast<daxa::i32>(device_props.limits.max_memory_allocation_count / 100000);
            return score;
        },
        .debug_name = "my device",
    });

    daxa::Swapchain swapchain = device.create_swapchain({
        .native_window = native_window_handle,
        .native_window_platform = get_native_platform(glfw_window_ptr),
        .surface_format_selector = [](daxa::Format format)
        {
            switch (format)
            {
            case daxa::Format::R8G8B8A8_UINT: return 100;
            default: return daxa::default_format_score(format);
            }
        },
        .present_mode = daxa::PresentMode::FIFO,
        .image_usage = daxa::ImageUsageFlagBits::TRANSFER_DST,
        .debug_name = "my swapchain",
    });

    auto pipeline_manager = daxa::PipelineManager({
        .device = device,
        .shader_compile_options = {
            .root_paths = {
	    	    DAXA_SHADER_INCLUDE_DIR,
                "include",
                "src",
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .debug_name = "my pipeline manager",
    });
    
    std::shared_ptr<daxa::RasterPipeline> raytrace_pipeline;
    {
        auto result = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {.source = daxa::ShaderFile{"raytrace.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"RAYTRACE_VERT"}}}},
            .fragment_shader_info = {.source = daxa::ShaderFile{"raytrace.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"RAYTRACE_FRAG"}}}},
            .color_attachments = {{.format = swapchain.get_format()}},
            .raster = {
                .front_face_winding = daxa::FrontFaceWinding::COUNTER_CLOCKWISE,
            },
            .push_constant_size = sizeof(RaytraceDrawPush),
            .debug_name = "my pipeline"
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        raytrace_pipeline = result.value();
    } 
    
    std::shared_ptr<daxa::ComputePipeline> prepare_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"raytrace.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"RAYTRACE_PREPARE"}}}},
            .push_constant_size = sizeof(RaytracePreparePush),
            .debug_name = "prepare_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        prepare_pipeline = result.value();
    }

    std::shared_ptr<daxa::ComputePipeline> queue_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"queue.glsl"}},
            .push_constant_size = sizeof(QueuePush),
            .debug_name = "queue_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        queue_pipeline = result.value();
    }

    std::shared_ptr<daxa::ComputePipeline> brush_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"base_terrain.glsl"}},
            .push_constant_size = sizeof(BrushPush),
            .debug_name = "brush_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        brush_pipeline = result.value();
    }
    
    std::shared_ptr<daxa::ComputePipeline> compressor_palettize_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"compressor.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"COMPRESSOR_PALETTIZE"}}}},
            .push_constant_size = sizeof(CompressorPush),
            .debug_name = "compressor_palettize_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        compressor_palettize_pipeline = result.value();
    }

    std::shared_ptr<daxa::ComputePipeline> compressor_allocate_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"compressor.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"COMPRESSOR_ALLOCATE"}}}},
            .push_constant_size = sizeof(CompressorPush),
            .debug_name = "compressor_allocate_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        compressor_allocate_pipeline = result.value();
    }

    std::shared_ptr<daxa::ComputePipeline> compressor_write_pipeline;
    {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"compressor.glsl"}, .compile_options = {.defines = {daxa::ShaderDefine{"COMPRESSOR_WRITE"}}}},
            .push_constant_size = sizeof(CompressorPush),
            .debug_name = "compressor_write_pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            return -1;
        }
        compressor_write_pipeline = result.value();
    }

    //TODO Create buffers
    auto perframe_buffer = device.create_buffer({
        .size = sizeof(Perframe),
        .debug_name = "perframe",
    });

    auto volume_buffer = device.create_buffer({
        .size = sizeof(Volume),
        .debug_name = "volume",
    });

    auto heap_buffer = device.create_buffer({
        .size = sizeof(Heap),
        .debug_name = "heap",
    });

    auto specs_buffer = device.create_buffer({
        .size = sizeof(Specs),
        .debug_name = "specs",
    });

    auto workspace_image = device.create_image({
            .dimensions = 3,
            .format = daxa::Format::R32_UINT,
            .size = {AXIS_WORKSPACE_SIZE * AXIS_CHUNK_SIZE, AXIS_WORKSPACE_SIZE * AXIS_CHUNK_SIZE, AXIS_WORKSPACE_SIZE * AXIS_CHUNK_SIZE},
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_WRITE | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = "workspace",
        });
    
    auto loop_task_list = daxa::TaskList({
        .device = device,
        .swapchain = swapchain,
        .debug_name = "my task list",
    });

    auto task_swapchain_image = loop_task_list.create_task_image({.swapchain_image = true, .debug_name = "my task swapchain image"});
    auto swapchain_image = daxa::ImageId{};
    loop_task_list.add_runtime_image(task_swapchain_image, swapchain_image);

    auto task_perframe_buffer = loop_task_list.create_task_buffer({.initial_access = daxa::AccessConsts::VERTEX_SHADER_READ, .debug_name = "my task buffer"});
    loop_task_list.add_runtime_buffer(task_perframe_buffer, perframe_buffer);

    auto task_volume_buffer = loop_task_list.create_task_buffer({.initial_access = daxa::AccessConsts::COMPUTE_SHADER_READ, .debug_name = "my task buffer"});
    loop_task_list.add_runtime_buffer(task_volume_buffer, volume_buffer);

    auto task_specs_buffer = loop_task_list.create_task_buffer({.initial_access = daxa::AccessConsts::COMPUTE_SHADER_READ, .debug_name = "my task buffer"});
    loop_task_list.add_runtime_buffer(task_specs_buffer, specs_buffer);

    auto task_heap_buffer = loop_task_list.create_task_buffer({.initial_access = daxa::AccessConsts::COMPUTE_SHADER_READ, .debug_name = "my task buffer"});
    loop_task_list.add_runtime_buffer(task_heap_buffer, heap_buffer);

    auto task_workspace_image = loop_task_list.create_task_image({.debug_name = "my task swapchain image"});
    loop_task_list.add_runtime_image(task_workspace_image, workspace_image);

    Perframe perframe;
    
    loop_task_list.add_task({
        .used_buffers = {
            {task_volume_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
            {task_specs_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_WRITE_ONLY}
        },
        .task = [
            task_volume_buffer, 
            task_heap_buffer, 
            task_specs_buffer, 
            task_workspace_image,
            &queue_pipeline
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            queue_task(
                task_runtime.get_device(), cmd_list, 
                queue_pipeline,
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0], 
                task_runtime.get_buffers(task_specs_buffer)[0], 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_images = {
            {task_workspace_image, daxa::TaskImageAccess::TRANSFER_WRITE, daxa::ImageMipArraySlice{}},
        },
        .task = [
            task_workspace_image
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            clear_workspace_task(
                task_runtime.get_device(), cmd_list, 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_buffers = {
            {task_specs_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY}
        },
        .used_images = {
            {task_workspace_image, daxa::TaskImageAccess::COMPUTE_SHADER_READ_WRITE, daxa::ImageMipArraySlice{}},
        },
        .task = [
            task_volume_buffer, 
            task_heap_buffer, 
            task_specs_buffer, 
            task_workspace_image,
            &brush_pipeline
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            brush_task(
                task_runtime.get_device(), cmd_list, 
                brush_pipeline,
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0], 
                task_runtime.get_buffers(task_specs_buffer)[0], 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_buffers = {
            {task_volume_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
            {task_specs_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY}
        }, 
        .used_images = {
            {task_workspace_image, daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY, daxa::ImageMipArraySlice{}},
        },
        .task = [
            task_volume_buffer, 
            task_heap_buffer, 
            task_specs_buffer, 
            task_workspace_image,
            &compressor_palettize_pipeline
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            compressor_palettize_task(
                task_runtime.get_device(), cmd_list, 
                compressor_palettize_pipeline,
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0], 
                task_runtime.get_buffers(task_specs_buffer)[0], 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_buffers = {
            {task_volume_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
            {task_heap_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
            {task_specs_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY}
        },
        .used_images = {
            {task_workspace_image, daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY, daxa::ImageMipArraySlice{}},
        },
        .task = [
            task_volume_buffer, 
            task_heap_buffer, 
            task_specs_buffer, 
            task_workspace_image,
            &compressor_allocate_pipeline
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            compressor_allocate_task(
                task_runtime.get_device(), cmd_list,
                compressor_allocate_pipeline,
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0], 
                task_runtime.get_buffers(task_specs_buffer)[0], 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });
    loop_task_list.add_task({
        .used_buffers = {
            {task_volume_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY},
            {task_heap_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_WRITE_ONLY},
            {task_specs_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY}
        },
        .used_images = {
            {task_workspace_image, daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY, daxa::ImageMipArraySlice{}},
        },
        .task = [
            task_volume_buffer, 
            task_heap_buffer, 
            task_specs_buffer, 
            task_workspace_image,
            &compressor_write_pipeline
        ](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            compressor_write_task(
                task_runtime.get_device(), cmd_list,
                compressor_write_pipeline,
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0], 
                task_runtime.get_buffers(task_specs_buffer)[0], 
                task_runtime.get_images(task_workspace_image)[0]
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_buffers = {
            {task_perframe_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE}
        },
        .task = [task_perframe_buffer, &perframe](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            upload_perframe_task(
                task_runtime.get_device(), cmd_list, 
                task_runtime.get_buffers(task_perframe_buffer)[0], 
                perframe
            );
        },
        .debug_name = "my draw task",
    });

    loop_task_list.add_task({
        .used_buffers = {
            {task_perframe_buffer, daxa::TaskBufferAccess::VERTEX_SHADER_READ_ONLY}
        },
        .used_images = {
            {task_swapchain_image, daxa::TaskImageAccess::COLOR_ATTACHMENT, daxa::ImageMipArraySlice{}},
        },
        .task = [task_swapchain_image, task_perframe_buffer, task_volume_buffer, task_heap_buffer, &raytrace_pipeline, &prepare_pipeline, &window_info](daxa::TaskRuntimeInterface task_runtime)
        {
            auto cmd_list = task_runtime.get_command_list();
            
            raytrace_task(
                task_runtime.get_device(), cmd_list, 
                raytrace_pipeline, prepare_pipeline,
                task_runtime.get_buffers(task_perframe_buffer)[0],
                task_runtime.get_buffers(task_volume_buffer)[0],
                task_runtime.get_buffers(task_heap_buffer)[0],
                task_runtime.get_images(task_swapchain_image)[0],
                window_info.width, window_info.height
            );
        },
        .debug_name = "my draw task",
    });
    
    auto submit_info = daxa::CommandSubmitInfo{};
    loop_task_list.submit(&submit_info);

    loop_task_list.present({});
    loop_task_list.complete();

    glm::vec3 translation = glm::vec3(0.0, 0.0, 10.0);
    glm::vec2 rotation = glm::vec2(0.0);

    std::chrono::milliseconds last_tick = duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
    daxa_f32 delta_time = 0.0;

    while (true)
    {
        std::chrono::milliseconds current_tick = duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );

        delta_time = daxa_f32((current_tick - last_tick).count()) / 1000;

        last_tick = current_tick;

        glfwPollEvents();
        if (glfwWindowShouldClose(glfw_window_ptr))
        {
            break;
        }

        {
            if(glfwGetKey(glfw_window_ptr, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                locked = false;
                glfwSetInputMode(glfw_window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            daxa_f32 sensitivity = 2e-3;

            rotation.x -= sensitivity * daxa_f32(x_move);
            rotation.y -= sensitivity * daxa_f32(y_move);
            rotation.y = std::clamp(rotation.y, daxa_f32(0.0), daxa_f32(2.0 * M_PI));
            x_move = 0;
            y_move = 0;
        }

        {
            bool forward = false, backward = false, left = false, right = false, up = false, down = false;

            if (locked) {
                forward = glfwGetKey(glfw_window_ptr, GLFW_KEY_W) == GLFW_PRESS;
                backward = glfwGetKey(glfw_window_ptr, GLFW_KEY_S) == GLFW_PRESS;
                left = glfwGetKey(glfw_window_ptr, GLFW_KEY_A) == GLFW_PRESS;
                right = glfwGetKey(glfw_window_ptr, GLFW_KEY_D) == GLFW_PRESS;
                up = glfwGetKey(glfw_window_ptr, GLFW_KEY_SPACE) == GLFW_PRESS;
                down = glfwGetKey(glfw_window_ptr, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
            }

            glm::vec3 direction = glm::vec3(
                daxa_f32(right) - daxa_f32(left),
                daxa_f32(forward) - daxa_f32(backward),
                daxa_f32(up) - daxa_f32(down)
            );

            glm::mat4 swivel = glm::rotate(glm::mat4(1.0), rotation.x, glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec3 movement = (swivel * glm::vec4(direction.x, direction.y, 0.0, 0.0)).xyz;

            if(glm::length(movement) != 0) {
                movement = glm::normalize(movement);
            }

            movement.z = direction.z;

            daxa_f32 speed = 1.0;

            movement *= speed * delta_time;

            translation += movement;
        }

        {
            glm::mat4 projection = glm::perspective(
                glm::pi<float>() * 0.25f, 
                daxa_f32(window_info.width) / daxa_f32(window_info.height), 
                0.1f, 
                100.f
            );
            projection[1][1] *= -1;

            glm::mat4 inv_projection = glm::inverse(projection);
            
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
	        transform = glm::rotate(transform, rotation.x, glm::vec3(0.0f, 0.0f, 1.0f));
	        transform = glm::rotate(transform, rotation.y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::mat4 view = glm::inverse(transform);

            perframe.camera = {
                .projection = daxa::math_operators::mat_from_span<daxa_f32, 4, 4>(
                    std::span<daxa_f32, 16> { glm::value_ptr(projection), 16 }
                ),
                .inv_projection = daxa::math_operators::mat_from_span<daxa_f32, 4, 4>(
                    std::span<daxa_f32, 16> { glm::value_ptr(inv_projection), 16 }
                ),
                .view = daxa::math_operators::mat_from_span<daxa_f32, 4, 4>(
                    std::span<daxa_f32, 16> { glm::value_ptr(view), 16 }
                ),
                .transform = daxa::math_operators::mat_from_span<daxa_f32, 4, 4>(
                    std::span<daxa_f32, 16> { glm::value_ptr(transform), 16 }
                )  
            };
        }

        if (window_info.swapchain_out_of_date)
        {
            swapchain.resize();
            window_info.swapchain_out_of_date = false;
        }

        loop_task_list.remove_runtime_image(task_swapchain_image, swapchain_image);
        
        swapchain_image = swapchain.acquire_next_image();
        
        loop_task_list.add_runtime_image(task_swapchain_image, swapchain_image);
        if (swapchain_image.is_empty())
        {
            continue;
        }

        loop_task_list.execute({});
    }

    device.wait_idle();
    device.destroy_buffer(perframe_buffer);
    device.destroy_buffer(volume_buffer);
    device.destroy_buffer(specs_buffer);
    device.destroy_buffer(heap_buffer);
    device.destroy_image(workspace_image);
    device.collect_garbage();
}

void raytrace_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::RasterPipeline> & raytrace_pipeline,
    std::shared_ptr<daxa::ComputePipeline> & prepare_pipeline,
    daxa::BufferId perframe_id,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::ImageId swapchain_image,
    daxa::u32 width, daxa::u32 height
) {
    auto write_indirect_id = device.create_buffer({
        .size = sizeof(DrawIndirect),
        .debug_name = "write_indirect",
    });

    auto indirect_id = device.create_buffer({
        .size = sizeof(DrawIndirect),
        .debug_name = "indirect",
    });


    cmd_list.destroy_buffer_deferred(write_indirect_id);
    cmd_list.destroy_buffer_deferred(indirect_id);

    cmd_list.set_pipeline(*prepare_pipeline);
    cmd_list.push_constant(RaytracePreparePush {
        .volume = device.get_device_address(volume_id),
        .indirect = device.get_device_address(write_indirect_id),
    });
    cmd_list.dispatch(1, 1, 1);

    cmd_list.copy_buffer_to_buffer({
        .src_buffer = write_indirect_id,
        .dst_buffer = indirect_id,
        .size = sizeof(DrawIndirect),
    });

    cmd_list.begin_renderpass({
        .color_attachments = {
            {
                .image_view = swapchain_image.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = std::array<daxa::f32, 4>{0.0f, 0.0f, 0.0f, 1.0f},
            },
        },
        .render_area = {.x = 0, .y = 0, .width = width, .height = height},
    });
    
    cmd_list.set_pipeline(*raytrace_pipeline);

    cmd_list.push_constant(RaytraceDrawPush{
        .volume = device.get_device_address(volume_id),
        .perframe = device.get_device_address(perframe_id),
        .heap = device.get_device_address(heap_id)
    });
    
    cmd_list.draw_indirect({
        .draw_command_buffer = indirect_id
    });
    cmd_list.end_renderpass();
}

void upload_perframe_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    daxa::BufferId buffer_id,
    Perframe perframe
) {
    auto staging_buffer_id = device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .size = sizeof(Perframe),
        .debug_name = "my staging buffer",
    });
    
    cmd_list.destroy_buffer_deferred(staging_buffer_id);
    
    auto * buffer_ptr = device.get_host_address_as<Perframe>(staging_buffer_id);
    *buffer_ptr = perframe;

    cmd_list.copy_buffer_to_buffer({
        .src_buffer = staging_buffer_id,
        .dst_buffer = buffer_id,
        .size = sizeof(Perframe),
    });
}

void clear_workspace_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    daxa::ImageId workspace_id
) {
    cmd_list.clear_image({
        .clear_value = std::array<daxa_u32, 4>({0, 0, 0, 0}),
        .dst_image = workspace_id
    });
}

void queue_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & queue_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
) {
    cmd_list.set_pipeline(*queue_pipeline);
    cmd_list.push_constant(QueuePush {
        .volume = device.get_device_address(volume_id),
        .specs = device.get_device_address(specs_id),
    });
    cmd_list.dispatch(1,1,1);
}

void brush_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & brush_pipeline,    
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
) {

    cmd_list.set_pipeline(*brush_pipeline);
    cmd_list.push_constant(BrushPush {
        .workspace = workspace_id,
        .specs = device.get_device_address(specs_id),
    });
    cmd_list.dispatch(
        AXIS_WORKSPACE_SIZE,   
        AXIS_WORKSPACE_SIZE,
        AXIS_WORKSPACE_SIZE
    );
}

void compressor_palettize_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_palettize_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
) {
    cmd_list.set_pipeline(*compressor_palettize_pipeline);
    cmd_list.push_constant(CompressorPush {
        .workspace = workspace_id,
        .specs = device.get_device_address(specs_id),
        .volume = device.get_device_address(volume_id),
        .heap = device.get_device_address(heap_id)
    });
    cmd_list.dispatch(AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE);

}

void compressor_allocate_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_allocate_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
) {

    cmd_list.set_pipeline(*compressor_allocate_pipeline);
    cmd_list.push_constant(CompressorPush {
        .workspace = workspace_id,
        .specs = device.get_device_address(specs_id),
        .volume = device.get_device_address(volume_id),
        .heap = device.get_device_address(heap_id)
    });
    cmd_list.dispatch(AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE);
}

void compressor_write_task(
    daxa::Device & device,
    daxa::CommandList & cmd_list,
    std::shared_ptr<daxa::ComputePipeline> & compressor_write_pipeline,
    daxa::BufferId volume_id,
    daxa::BufferId heap_id,
    daxa::BufferId specs_id,
    daxa::ImageId workspace_id
) {
    cmd_list.set_pipeline(*compressor_write_pipeline);
    cmd_list.push_constant(CompressorPush {
        .workspace = workspace_id,
        .specs = device.get_device_address(specs_id),
        .volume = device.get_device_address(volume_id),
        .heap = device.get_device_address(heap_id)
    });
    cmd_list.dispatch(AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE, AXIS_WORKSPACE_SIZE);
}