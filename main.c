#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
// #include "lv_font_Chinese_siyun_blod.c"
// #include "lv_font_Chinese_siyun_songti_regular.c"
#include "lv_font_Chinese_siyun_heiti_light.c"

#include <signal.h>
#include "lvgl/examples/porting/lv_port_disp_template.h"

// 获取环境变量或使用默认值
static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ?: dflt;
}

#if LV_USE_EVDEV
static void lv_linux_init_input_pointer(lv_display_t *disp)
{
    // Enables a pointer (touchscreen/mouse) input device
    // Use 'evtest' to find the correct input device. /dev/input/by-id/ is recommeded if possible
    // Use /dev/input/by-id/my-mouse-or-touchscreen or /dev/input/eventX
    const char *input_device = getenv_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/by-id/my-mouse-or-touchscreen");
    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, input_device);
    lv_indev_set_display(touch, disp);

    // Disable this if you want no cursor
    LV_IMAGE_DECLARE(mouse_cursor_icon);                        /*Declare the image source.*/
    lv_obj_t *cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
    lv_image_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(touch, cursor_obj);                     /*Connect the image  object to the driver*/
}
#endif

/**
 *  1.获取帧缓冲设备路径（环境变量 LV_LINUX_FBDEV_DEVICE，如果没有则默认 /dev/fb0）。
    2.创建一个 LVGL 显示设备（lv_display_t）。
    3.绑定帧缓冲设备文件，确保 LVGL 输出到指定设备。
 */
#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t *disp = lv_linux_fbdev_create();

#if LV_USE_EVDEV
    lv_linux_init_input_pointer(disp);
#endif

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t *disp = lv_linux_drm_create();

#if LV_USE_EVDEV
    lv_linux_init_input_pointer(disp);
#endif

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_SDL_VIDEO_WIDTH") ?: "800");
    const int height = atoi(getenv("LV_SDL_VIDEO_HEIGHT") ?: "480");

    lv_sdl_window_create(width, height);
}
#else
#error Unsupported configuration
#endif

// 用于存储当前标签的 y 坐标，保证每次输出都会在上一行下方
static int y_offset = 20;

// 解析指纹状态并更新 LVGL 界面
// void update_lvgl_display(int finger_id, int status)
// {
//     LV_FONT_DECLARE(lv_font_Chinese_siyun_heiti_lightv);

//     char buf[128];
//     time_t now = time(NULL);
//     struct tm *tm_info = localtime(&now);
//     strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);

//     if (label == NULL)
//     {
//         label = lv_label_create(lv_scr_act());

//         // 设置 label 的位置，每次往下移动
//         lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y_offset);
//     }

//     // 更新标签文本，避免多次创建
//     lv_label_set_text_fmt(label, "%s", status ? "指纹采集成功" : "指纹采集失败，请重试");
//     lv_obj_set_style_text_font(label, &lv_font_Chinese_siyun_heiti_light, LV_STATE_DEFAULT);

//     // 更新 y_offset，保证下一个 label 距离前一个 label 一定距离
//     y_offset += 40; // 根据字体大小调整间距

//     // 限制最大高度，避免标签超出屏幕
//     if (y_offset > LV_VER_RES - 40)
//     {
//         // 在底部时，执行上滑效果
//         lv_obj_t *scr = lv_scr_act();
//         lv_timer_create(slide_up, 10, scr); // 创建任务，执行上滑效果
//         y_offset = 20;                         // 重置 y_offset
//     }
// }

void update_lvgl_display(int finger_id, int status)
{
    LV_FONT_DECLARE(lv_font_Chinese_siyun_heiti_lightv);

    char buf[128];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);

    lv_obj_t *label = lv_label_create(lv_scr_act());

    // 设置 label 的位置，每次往下移动
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y_offset);

    // 更新标签文本，避免多次创建
    lv_label_set_text_fmt(label, "%s", status ? "指纹采集成功" : "指纹采集失败");
    lv_obj_set_style_text_font(label, &lv_font_Chinese_siyun_heiti_light, LV_STATE_DEFAULT);
    
    // 更新 y_offset，保证下一个 label 距离前一个 label 一定距离
    y_offset += 40; // 根据字体大小调整间距

    // 限制最大高度，避免标签超出屏幕
    if (y_offset > LV_VER_RES - 40) {
        y_offset = 20; // 如果超过屏幕高度，则重置 y_offset

        // 创建一个新的空白屏幕
        lv_obj_t *new_scr = lv_obj_create(NULL);

        // 切换到新的屏幕
        lv_scr_load(new_scr);
    }
}

// void update_lvgl_display(int finger_id, int status)
// {
//     LV_FONT_DECLARE(lv_font_Chinese_siyun_heiti_lightv);

//     char buf[128];
//     time_t now = time(NULL);
//     struct tm *tm_info = localtime(&now);
//     strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);

//     // 创建新标签
//     lv_obj_t *label = lv_label_create(lv_scr_act());
//     lv_label_set_text_fmt(label, "%s", status ? "指纹采集成功" : "指纹采集失败");
//     lv_obj_set_style_text_font(label, &lv_font_Chinese_siyun_heiti_light, LV_STATE_DEFAULT);
//     lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y_offset);

//     // 渐隐并删除旧 label（先确保它存在）
//     lv_obj_t *prev_label = lv_obj_get_child(lv_scr_act(), -2);
//     if (prev_label) {
//         lv_obj_fade_out(prev_label, 500, 0);
//         lv_async_call((lv_async_cb_t)lv_obj_del, prev_label); // 使用异步删除
//     }

//     // 更新 y_offset
//     y_offset += 40;
//     if (y_offset > LV_VER_RES - 40) {
//         y_offset = 20;
//     }
// }

// void signal_handler(int sig, siginfo_t *info, void *context)
// {
//     if (sig == SIGUSR1)
//     {
//         printf("监听到指纹模块发送来的信号\n");
//         int data = info->si_value.sival_int;
//         int status = (data >> 16) & 0xFFFF;
//         int finger_id = data & 0xFFFF;

//         // 不能直接调用 LVGL API，改用异步方式
//         static struct {
//             int finger_id;
//             int status;
//         } *param = NULL;

//         param = malloc(sizeof(*param));
//         if (param) {
//             param->finger_id = finger_id;
//             param->status = status;
//             lv_async_call((lv_async_cb_t)update_lvgl_display, param);
//         }
//     }
// }


// 信号处理函数
void signal_handler(int sig, siginfo_t *info, void *context)
{
    if (sig == SIGUSR1)
    {
        printf("监听到指纹模块发送来的信号\n");
        int data = info->si_value.sival_int;
        int status = (data >> 16) & 0xFFFF; // 提取状态
        int finger_id = data & 0xFFFF;      // 提取指纹 ID
        update_lvgl_display(finger_id, status);
    }
}

// 注册信号监听
void setup_signal_listener()
{
    lv_init();
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigaction(SIGUSR1, &sa, NULL);
}

int main(void)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    lv_port_disp_init();

    // lv_example_label_init();  // 初始化界面

    /*Create a Demo*/
    // lv_demo_widgets();
    // lv_demo_widgets_start_slideshow();
    lv_demo_music();

    // lv_obj_t *label = lv_label_create(lv_scr_act());
    // lv_label_set_text(label, "你好");

    // lv_example_anim_233();

    // lv_example_arc_2();

    // setup_signal_listener();

    // lv_display_t *disp = lv_display_get_default(); // 获取默认显示设备
    // if (disp == NULL) {
    //     printf("无法获取显示设备\n");
    //     return;
    // }

    // lv_coord_t width = lv_display_get_horizontal_resolution(disp);
    // lv_coord_t height = lv_display_get_vertical_resolution(disp);

    // printf("width = %d, height = %d\n", width, height);

    /*
    // 启用双缓冲
    // 720 * 1280为屏幕的分辨率
    static lv_color_t buf1[720 * 100];  // 第一个缓冲区
    static lv_color_t buf2[1280 * 100];  // 第二个缓冲区 (双缓冲)

    lv_display_t *disp = lv_display_create(720, 1280);

    // 设置双缓冲，LV_DISPLAY_RENDER_MODE_PARTIAL 指部分刷新模式，适合大分辨率
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    */

    // 获取线程id
    // pid_t pid;
    // pthread_t tid;
    // pid = getpid();
    // tid = pthread_self();
    // printf("lvgl: pid %u tid %u (0x%x)\n", (unsigned int)pid,
    //         (unsigned int)tid, (unsigned int)tid); /* tid是unsigned long int,这里只是方便转换 */

    /*Handle LVGL tasks*/
    while (1)
    {
        // while(setup_signal_listener()) ????
        // uint32_t idle_time = lv_timer_handler(); /*Returns the time to the next timer execution*/
        // lv_task_handler();  /* 让 LVGL 处理任务 */
        // usleep(10 * 1000);

        uint32_t idle_time = lv_timer_handler();
        usleep(idle_time * 1000);
    }

    return 0;
}
