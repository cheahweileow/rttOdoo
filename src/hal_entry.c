/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2024-03-11     kurisaW       first version
 */

#include <rtthread.h>
#include "hal_data.h"
#include <rtdevice.h>
#include <board.h>

#if defined(BSP_USING_CEU_CAMERA)
#include "sensor.h"
#else
#include "arducam.h"
#include "camera_layer.h"
#include "camera_layer_config.h"
#endif

#include "models/model.h"
#include "pmu_ethosu.h"
#include "yolo/yolo_rtthread.h"

#if defined(RT_USING_LWIP) && defined(BSP_USING_ETH)
#include <sys/socket.h>
#include <netdb.h>
#endif

#if defined(BSP_USING_SDCARD_FATFS)
#include <dfs_file.h>
#include <sys/stat.h>
#endif

#if defined(BSP_USING_LCD)
#include <drv_lcd.h>
#endif

#define LED_PIN     BSP_IO_PORT_00_PIN_12 /* Onboard LED pins */

#define DISPLAY_SCREEN_WIDTH              (800)
#define DISPLAY_SCREEN_HEIGHT             (480)

#if defined(BSP_USING_CEU_CAMERA)
#define CAMERA_CAPTURE_IMAGE_WIDTH        (640)
#define CAMERA_CAPTURE_IMAGE_HEIGHT       (480)
#define CAMERA_IMAGE_BYTE_PER_PIXEL       (2)

extern sensor_t sensor;
static uint8_t camera_capture_image_rgb565[CAMERA_CAPTURE_IMAGE_WIDTH *
                                           CAMERA_CAPTURE_IMAGE_HEIGHT *
                                           CAMERA_IMAGE_BYTE_PER_PIXEL]
                                           BSP_PLACE_IN_SECTION(".ospi1_cs0_noinit")
                                           BSP_ALIGN_VARIABLE(8);
static volatile uint32_t camera_ceu_frame_count = 0;

static fsp_err_t camera_init(bool use_test_mode)
{
    RT_UNUSED(use_test_mode);

    if (sensor_init() != 0)
    {
        return FSP_ERR_ASSERTION;
    }

    if (sensor_reset() != 0)
    {
        return FSP_ERR_ASSERTION;
    }

    if (sensor_set_pixformat(PIXFORMAT_RGB565) != 0)
    {
        return FSP_ERR_ASSERTION;
    }

    if (sensor_set_framesize(FRAMESIZE_VGA) != 0)
    {
        return FSP_ERR_ASSERTION;
    }

    return FSP_SUCCESS;
}

static void camera_image_buffer_initialize(void)
{
    rt_memset(camera_capture_image_rgb565, 0, sizeof(camera_capture_image_rgb565));
}

static fsp_err_t camera_capture_start(void)
{
    return FSP_SUCCESS;
}

static uint32_t camera_data_ready_buffer_pointer_get(void)
{
    return (uint32_t) &camera_capture_image_rgb565[0];
}

static uint32_t camera_vin_frame_count_get(void)
{
    return camera_ceu_frame_count;
}

static uint32_t camera_mipi_callback_count_get(void)
{
    return 0;
}

static uint32_t camera_vin_last_buffer_get(void)
{
    return (uint32_t) &camera_capture_image_rgb565[0];
}

static uint32_t camera_vin_last_event_get(void)
{
    return 0;
}

static uint32_t camera_vin_last_status_get(void)
{
    return 0;
}

static uint32_t camera_capture_post_process(void)
{
    return 0;
}
#else
extern struct rt_completion ceu_completion;
#endif

static rt_bool_t camera_started = RT_FALSE;
static rt_bool_t camera_starting = RT_FALSE;
static rt_bool_t vin_started = RT_FALSE;
static rt_bool_t vin_starting = RT_FALSE;
static rt_bool_t npu_started = RT_FALSE;
static volatile rt_bool_t detect_loop_running = RT_FALSE;
static volatile rt_bool_t face_send_loop_running = RT_FALSE;
static volatile rt_bool_t face_upload_loop_running = RT_FALSE;
static volatile rt_bool_t lcd_live_loop_running = RT_FALSE;
static volatile rt_bool_t lcd_ai_live_loop_running = RT_FALSE;
static volatile rt_bool_t camera_processing_busy = RT_FALSE;
static struct rt_mutex camera_frame_mutex;
static rt_bool_t camera_frame_mutex_ready = RT_FALSE;

#define CAMERA_FRAME_WAIT_TICKS        rt_tick_from_millisecond(2000)
#define DETECT_POOL_BOX_COUNT          (GRID_SIZE_1 * GRID_SIZE_1 * ANCHORS + GRID_SIZE_2 * GRID_SIZE_2 * ANCHORS)
#define SERIAL_FRAME_CHUNK_BYTES       (1024)
#define HTTP_UPLOAD_CHUNK_BYTES        (1460)

#ifndef TITAN_AUTO_START_ENABLE
#define TITAN_AUTO_START_ENABLE        1
#endif

#ifndef TITAN_AUTO_LCD_AI_FPS_TEXT
#define TITAN_AUTO_LCD_AI_FPS_TEXT     "5"
#endif

#ifndef TITAN_AUTO_UPLOAD_INTERVAL_TEXT
#define TITAN_AUTO_UPLOAD_INTERVAL_TEXT "5"
#endif

#ifndef TITAN_AUTO_START_DELAY_MS
#define TITAN_AUTO_START_DELAY_MS      3000
#endif

static char upload_host[64] = "192.168.101.123";
static uint32_t upload_port = 5000;

#if defined(BSP_USING_SDCARD_FATFS)
static int save_camera_raw_frame(const char *path)
{
    uint8_t *p_img = (uint8_t *) camera_data_ready_buffer_pointer_get();
    int image_size = CAMERA_CAPTURE_IMAGE_WIDTH *
                     CAMERA_CAPTURE_IMAGE_HEIGHT *
                     CAMERA_IMAGE_BYTE_PER_PIXEL;
    struct dfs_file file;

    dfs_file_unlink(path);

    fd_init(&file);

    int result = dfs_file_open(&file, path, O_WRONLY | O_CREAT | O_TRUNC);
    if (result < 0)
    {
        rt_kprintf("open %s failed, result=%d\n", path, result);
        return -1;
    }

    int written_total = 0;
    while (written_total < image_size)
    {
        int chunk = image_size - written_total;
        if (chunk > (32 * 1024))
        {
            chunk = 32 * 1024;
        }

        int written = dfs_file_write(&file, p_img + written_total, chunk);
        if (written <= 0)
        {
            rt_kprintf("write %s failed, written=%d/%d, result=%d\n",
                       path, written_total, image_size, written);
            dfs_file_close(&file);
            return -1;
        }

        written_total += written;
    }

    result = dfs_file_flush(&file);
    if (result < 0)
    {
        rt_kprintf("flush %s failed, result=%d\n", path, result);
        dfs_file_close(&file);
        return -1;
    }

    result = dfs_file_close(&file);
    if (result < 0)
    {
        rt_kprintf("close %s failed, result=%d\n", path, result);
        return -1;
    }

    rt_kprintf("saved %s, size=%d bytes, format=RGB565, width=%d, height=%d\n",
               path,
               image_size,
               CAMERA_CAPTURE_IMAGE_WIDTH,
               CAMERA_CAPTURE_IMAGE_HEIGHT);

    struct stat st;
    result = dfs_file_stat(path, &st);
    if (result == 0)
    {
        rt_kprintf("verified %s exists, stat_size=%d\n", path, (int)st.st_size);
    }
    else
    {
        rt_kprintf("verify %s failed after save, result=%d\n", path, result);
    }

    return 0;
}
#endif

static void print_frame_brightness(void)
{
    uint16_t *p_img = (uint16_t *) camera_data_ready_buffer_pointer_get();
    uint32_t sum = 0;
    uint32_t nonzero = 0;
    uint32_t pixels = CAMERA_CAPTURE_IMAGE_WIDTH * CAMERA_CAPTURE_IMAGE_HEIGHT;

    for (int y = 0; y < CAMERA_CAPTURE_IMAGE_HEIGHT; y++)
    {
        for (int x = 0; x < CAMERA_CAPTURE_IMAGE_WIDTH; x++)
        {
            uint16_t p = p_img[y * CAMERA_CAPTURE_IMAGE_WIDTH + x];
            if (p != 0)
            {
                nonzero++;
            }

            uint8_t r = (uint8_t)(((p >> 11) & 0x1F) << 3);
            uint8_t g = (uint8_t)(((p >> 5) & 0x3F) << 2);
            uint8_t b = (uint8_t)((p & 0x1F) << 3);
            uint8_t gray = (uint8_t)((r * 30 + g * 59 + b * 11) / 100);

            sum += gray;
        }
    }

    rt_kprintf("captured frame: %dx%d RGB565, avg_brightness=%d\n",
               CAMERA_CAPTURE_IMAGE_WIDTH,
               CAMERA_CAPTURE_IMAGE_HEIGHT,
               sum / pixels);
    rt_kprintf("nonzero_pixels=%d, first_words=%04x %04x %04x %04x\n",
               nonzero,
               p_img[0],
               p_img[1],
               p_img[2],
               p_img[3]);
}

static int run_face_detection(rt_bool_t print_boxes, rt_bool_t send_on_face);
static int upload_camera_frame_to_host(int16_t face_count);

static int camera_frame_lock(void)
{
    if (!camera_frame_mutex_ready)
    {
        return 0;
    }

    if (rt_mutex_take(&camera_frame_mutex, rt_tick_from_millisecond(3000)) != RT_EOK)
    {
        rt_kprintf("camera frame lock timeout\n");
        return -1;
    }

    return 0;
}

static void camera_frame_unlock(void)
{
    if (camera_frame_mutex_ready)
    {
        rt_mutex_release(&camera_frame_mutex);
    }
}

static void display_camera_frame_on_lcd(void)
{
#if defined(BSP_USING_LCD)
    lcd_draw_jpg(0,
                 0,
                 (const void *) camera_data_ready_buffer_pointer_get(),
                 CAMERA_CAPTURE_IMAGE_WIDTH,
                 CAMERA_CAPTURE_IMAGE_HEIGHT);
#endif
}

static void rgb565_draw_fill_rect(uint16_t *img,
                                  int width,
                                  int height,
                                  int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  uint16_t color)
{
    if ((img == RT_NULL) || (x2 < 0) || (y2 < 0) || (x1 >= width) || (y1 >= height))
    {
        return;
    }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= width) x2 = width - 1;
    if (y2 >= height) y2 = height - 1;

    for (int y = y1; y <= y2; y++)
    {
        uint16_t *row = img + (y * width);
        for (int x = x1; x <= x2; x++)
        {
            row[x] = color;
        }
    }
}

static void rgb565_draw_rect(uint16_t *img,
                             int width,
                             int height,
                             int x1,
                             int y1,
                             int x2,
                             int y2,
                             int thickness,
                             uint16_t color)
{
    for (int i = 0; i < thickness; i++)
    {
        rgb565_draw_fill_rect(img, width, height, x1, y1 + i, x2, y1 + i, color);
        rgb565_draw_fill_rect(img, width, height, x1, y2 - i, x2, y2 - i, color);
        rgb565_draw_fill_rect(img, width, height, x1 + i, y1, x1 + i, y2, color);
        rgb565_draw_fill_rect(img, width, height, x2 - i, y1, x2 - i, y2, color);
    }
}

static void tiny_font_get_rows(char ch, uint8_t rows[7])
{
    static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t glyph_0[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    static const uint8_t glyph_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const uint8_t glyph_2[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    static const uint8_t glyph_3[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    static const uint8_t glyph_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    static const uint8_t glyph_5[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    static const uint8_t glyph_6[7] = {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    static const uint8_t glyph_7[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    static const uint8_t glyph_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    static const uint8_t glyph_9[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
    static const uint8_t glyph_f[7] = {0x07, 0x08, 0x08, 0x1E, 0x08, 0x08, 0x08};
    static const uint8_t glyph_a[7] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
    static const uint8_t glyph_c[7] = {0x00, 0x00, 0x0E, 0x10, 0x10, 0x10, 0x0E};
    static const uint8_t glyph_e[7] = {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E};
    static const uint8_t glyph_colon[7] = {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
    static const uint8_t glyph_percent[7] = {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03};
    const uint8_t *src = blank;

    switch (ch)
    {
        case '0': src = glyph_0; break;
        case '1': src = glyph_1; break;
        case '2': src = glyph_2; break;
        case '3': src = glyph_3; break;
        case '4': src = glyph_4; break;
        case '5': src = glyph_5; break;
        case '6': src = glyph_6; break;
        case '7': src = glyph_7; break;
        case '8': src = glyph_8; break;
        case '9': src = glyph_9; break;
        case 'f': src = glyph_f; break;
        case 'a': src = glyph_a; break;
        case 'c': src = glyph_c; break;
        case 'e': src = glyph_e; break;
        case ':': src = glyph_colon; break;
        case '%': src = glyph_percent; break;
        default: src = blank; break;
    }

    for (int i = 0; i < 7; i++)
    {
        rows[i] = src[i];
    }
}

static void rgb565_draw_text(uint16_t *img,
                             int width,
                             int height,
                             int x,
                             int y,
                             const char *text,
                             int scale,
                             uint16_t color)
{
    while (*text != '\0')
    {
        uint8_t rows[7];
        tiny_font_get_rows(*text, rows);

        for (int gy = 0; gy < 7; gy++)
        {
            for (int gx = 0; gx < 5; gx++)
            {
                if (rows[gy] & (1U << (4 - gx)))
                {
                    rgb565_draw_fill_rect(img,
                                          width,
                                          height,
                                          x + gx * scale,
                                          y + gy * scale,
                                          x + gx * scale + scale - 1,
                                          y + gy * scale + scale - 1,
                                          color);
                }
            }
        }

        x += 6 * scale;
        text++;
    }
}

static void draw_face_overlay(det_box_t *boxes, int16_t count)
{
#if defined(BSP_USING_LCD)
    uint16_t *img = (uint16_t *) camera_data_ready_buffer_pointer_get();
    const uint16_t cyan = 0x07FF;
    const uint16_t black = 0x0000;
    char label[16];

    for (int16_t i = 0; i < count; i++)
    {
        int score = (int) (boxes[i].score * 100.0f);
        int label_x = boxes[i].x1;
        int label_y = boxes[i].y1 - 18;
        int label_w;

        if (score < 0) score = 0;
        if (score > 99) score = 99;

        rt_snprintf(label, sizeof(label), "face:%d%%", score);
        label_w = (int) rt_strlen(label) * 12 + 4;

        if (label_y < 0)
        {
            label_y = boxes[i].y1 + 2;
        }

        rgb565_draw_rect(img,
                         CAMERA_CAPTURE_IMAGE_WIDTH,
                         CAMERA_CAPTURE_IMAGE_HEIGHT,
                         boxes[i].x1,
                         boxes[i].y1,
                         boxes[i].x2,
                         boxes[i].y2,
                         3,
                         cyan);
        rgb565_draw_fill_rect(img,
                              CAMERA_CAPTURE_IMAGE_WIDTH,
                              CAMERA_CAPTURE_IMAGE_HEIGHT,
                              label_x,
                              label_y,
                              label_x + label_w,
                              label_y + 17,
                              cyan);
        rgb565_draw_text(img,
                         CAMERA_CAPTURE_IMAGE_WIDTH,
                         CAMERA_CAPTURE_IMAGE_HEIGHT,
                         label_x + 2,
                         label_y + 2,
                         label,
                         2,
                         black);
    }

    if (count > 0)
    {
        display_camera_frame_on_lcd();
    }
#else
    RT_UNUSED(boxes);
    RT_UNUSED(count);
#endif
}

static int wait_for_camera_frame(void)
{
    int result = -1;

#if defined(BSP_USING_CEU_CAMERA)
    if (!camera_started)
    {
        rt_kprintf("camera not started. Run: cam_start\n");
        return -1;
    }

    if (camera_frame_lock() != 0)
    {
        return -1;
    }

    result = sensor_snapshot(&sensor, camera_capture_image_rgb565, 0);
    if (result != 0)
    {
        rt_kprintf("CEU snapshot failed, result=%d\n", result);
        camera_frame_unlock();
        return -1;
    }

    camera_ceu_frame_count++;
    display_camera_frame_on_lcd();
    camera_frame_unlock();
    return result;
#else
    uint32_t frames_before;
    uint32_t frames_after;

    if (!camera_started)
    {
        rt_kprintf("camera not started. Run: cam_start\n");
        return -1;
    }

    if (!vin_started)
    {
        rt_kprintf("VIN capture not started. Run: cam_vin_start\n");
        return -1;
    }

    if (camera_frame_lock() != 0)
    {
        return -1;
    }

    frames_before = camera_vin_frame_count_get();

#ifndef VIN_CFG_USE_RUNTIME_BUFFER
    rt_err_t wait_result = rt_completion_wait(&ceu_completion, CAMERA_FRAME_WAIT_TICKS);
    frames_after = camera_vin_frame_count_get();
    if (wait_result != RT_EOK)
    {
        rt_kprintf("frame wait timeout: no VIN frame-complete interrupt\n");
        rt_kprintf("check MIPI CSI/VIN pins, clocks, HyperRAM buffer, and camera stream\n");
        camera_frame_unlock();
        return -1;
    }

    if (frames_after == frames_before)
    {
        rt_kprintf("frame wait returned but VIN frame count did not increase\n");
        rt_kprintf("vin_frames=%d, vin_last_status=0x%08x\n",
                   frames_after,
                   camera_vin_last_status_get());
        camera_frame_unlock();
        return -1;
    }

    camera_capture_post_process();
    display_camera_frame_on_lcd();
#else
    rt_thread_mdelay(100);
    frames_after = camera_vin_frame_count_get();
    if (frames_after == frames_before)
    {
        rt_kprintf("no VIN frame arrived yet\n");
        camera_frame_unlock();
        return -1;
    }
#endif

    camera_frame_unlock();
    return 0;
#endif
}

static int ensure_npu_started(void)
{
    if (npu_started)
    {
        return 0;
    }

    fsp_err_t status = RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);
    if (status != FSP_SUCCESS)
    {
        rt_kprintf("NPU open failed, status=%d\n", status);
        return -1;
    }

    npu_started = RT_TRUE;
    rt_kprintf("NPU initialized\n");

    return 0;
}

static void cam_start_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    rt_kprintf("camera init begin\n");

    fsp_err_t fsp_status = camera_init(false);
    if (FSP_SUCCESS != fsp_status)
    {
        rt_kprintf("camera_init fail, fsp_status=%d\n", fsp_status);
        camera_starting = RT_FALSE;
        return;
    }

    camera_image_buffer_initialize();

    camera_started = RT_TRUE;
    camera_starting = RT_FALSE;
#if defined(BSP_USING_CEU_CAMERA)
    vin_started = RT_TRUE;
    rt_kprintf("CEU camera initialized. Run: cam_capture\n");
#else
    rt_kprintf("camera initialized. Starting VIN capture now...\n");

    if (!vin_started && !vin_starting)
    {
        vin_starting = RT_TRUE;
        rt_kprintf("VIN start begin\n");
        fsp_err_t err = camera_capture_start();
        if (FSP_SUCCESS != err)
        {
            vin_starting = RT_FALSE;
            rt_kprintf("VIN capture start failed, err=%d\n", err);
            return;
        }

        vin_started = RT_TRUE;
        vin_starting = RT_FALSE;
        rt_kprintf("VIN capture started. Run: cam_capture\n");
    }
#endif
}

static int cam_start(int argc, char **argv)
{
    rt_thread_t tid;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (camera_started)
    {
        rt_kprintf("camera already started\n");
        return 0;
    }

    if (camera_starting)
    {
        rt_kprintf("camera start already in progress\n");
        return 0;
    }

    camera_starting = RT_TRUE;

    tid = rt_thread_create("cam_start",
                           cam_start_thread_entry,
                           RT_NULL,
                           4096,
                           20,
                           10);
    if (tid == RT_NULL)
    {
        camera_starting = RT_FALSE;
        rt_kprintf("create camera start thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);
#if defined(BSP_USING_CEU_CAMERA)
    rt_kprintf("CEU camera start running in background. Wait for: CEU camera initialized\n");
#else
    rt_kprintf("camera/VIN start running in background. Wait for: VIN capture started\n");
#endif

    return 0;
}
MSH_CMD_EXPORT(cam_start, initialize camera and start capture);

static void cam_vin_start_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    rt_kprintf("VIN start begin\n");
    fsp_err_t err = camera_capture_start();
    if (FSP_SUCCESS != err)
    {
        vin_starting = RT_FALSE;
        rt_kprintf("VIN capture start failed, err=%d\n", err);
        return;
    }

    vin_started = RT_TRUE;
    vin_starting = RT_FALSE;
    rt_kprintf("VIN capture started. Run: cam_capture\n");
}

static int cam_vin_start(int argc, char **argv)
{
    rt_thread_t tid;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!camera_started)
    {
        rt_kprintf("camera not initialized. Run: cam_start\n");
        return -1;
    }

    if (vin_started)
    {
        rt_kprintf("VIN capture already started\n");
        return 0;
    }

    if (vin_starting)
    {
        rt_kprintf("VIN start already in progress\n");
        return 0;
    }

    vin_starting = RT_TRUE;

    tid = rt_thread_create("vin_start",
                           cam_vin_start_thread_entry,
                           RT_NULL,
                           4096,
                           25,
                           10);
    if (tid == RT_NULL)
    {
        vin_starting = RT_FALSE;
        rt_kprintf("create VIN start thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);
    rt_kprintf("VIN start running in background. Use: cam_status\n");

    return 0;
}
MSH_CMD_EXPORT(cam_vin_start, start VIN capture after camera init);

#if !defined(BSP_USING_CEU_CAMERA)
static int cam_probe(int argc, char **argv)
{
    uint8_t id_high = 0;
    uint8_t id_low = 0;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!rdSensorReg16_8(0x300a, &id_high))
    {
        rt_kprintf("read camera ID high failed\n");
        return -1;
    }

    if (!rdSensorReg16_8(0x300b, &id_low))
    {
        rt_kprintf("read camera ID low failed\n");
        return -1;
    }

    rt_kprintf("camera ID = 0x%02x 0x%02x\n", id_high, id_low);

    return 0;
}
MSH_CMD_EXPORT(cam_probe, read OV5640 camera product ID);
#endif

static rt_bool_t parse_hex_or_dec(const char *text, uint32_t *value)
{
    uint32_t result = 0;
    int base = 10;

    if ((text == RT_NULL) || (value == RT_NULL))
    {
        return RT_FALSE;
    }

    if ((text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X')))
    {
        base = 16;
        text += 2;
    }

    if (*text == '\0')
    {
        return RT_FALSE;
    }

    while (*text != '\0')
    {
        uint8_t digit;

        if ((*text >= '0') && (*text <= '9'))
        {
            digit = (uint8_t)(*text - '0');
        }
        else if ((*text >= 'a') && (*text <= 'f'))
        {
            digit = (uint8_t)(*text - 'a' + 10);
        }
        else if ((*text >= 'A') && (*text <= 'F'))
        {
            digit = (uint8_t)(*text - 'A' + 10);
        }
        else
        {
            return RT_FALSE;
        }

        if (digit >= base)
        {
            return RT_FALSE;
        }

        result = (result * (uint32_t) base) + digit;
        text++;
    }

    *value = result;
    return RT_TRUE;
}

#if !defined(BSP_USING_CEU_CAMERA)
static int cam_rd(int argc, char **argv)
{
    uint32_t reg;
    uint8_t value = 0;

    if (argc != 2)
    {
        rt_kprintf("usage: cam_rd 0x300e\n");
        return -1;
    }

    if (!parse_hex_or_dec(argv[1], &reg) || (reg > 0xffff))
    {
        rt_kprintf("bad register address: %s\n", argv[1]);
        return -1;
    }

    if (!rdSensorReg16_8((uint16_t) reg, &value))
    {
        rt_kprintf("read 0x%04x failed\n", reg);
        return -1;
    }

    rt_kprintf("OV5640[0x%04x] = 0x%02x\n", reg, value);
    return 0;
}
MSH_CMD_EXPORT(cam_rd, read OV5640 register: cam_rd 0x300e);

static int cam_wr(int argc, char **argv)
{
    uint32_t reg;
    uint32_t value;

    if (argc != 3)
    {
        rt_kprintf("usage: cam_wr 0x300e 0x45\n");
        return -1;
    }

    if (!parse_hex_or_dec(argv[1], &reg) || (reg > 0xffff))
    {
        rt_kprintf("bad register address: %s\n", argv[1]);
        return -1;
    }

    if (!parse_hex_or_dec(argv[2], &value) || (value > 0xff))
    {
        rt_kprintf("bad register value: %s\n", argv[2]);
        return -1;
    }

    if (!wrSensorReg16_8((uint16_t) reg, (uint8_t) value))
    {
        rt_kprintf("write 0x%04x failed\n", reg);
        return -1;
    }

    rt_kprintf("OV5640[0x%04x] <= 0x%02x\n", reg, value);
    return 0;
}
MSH_CMD_EXPORT(cam_wr, write OV5640 register: cam_wr 0x300e 0x45);

static int cam_sensor_status(int argc, char **argv)
{
    static const uint16_t regs[] =
    {
        0x300a, 0x300b, 0x300e, 0x3019,
        0x3035, 0x3036, 0x3037, 0x3108,
        0x4202, 0x4300, 0x4800, 0x4814,
        0x4837, 0x501f
    };

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    for (uint32_t i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++)
    {
        uint8_t value = 0;

        if (rdSensorReg16_8(regs[i], &value))
        {
            rt_kprintf("OV5640[0x%04x] = 0x%02x\n", regs[i], value);
        }
        else
        {
            rt_kprintf("OV5640[0x%04x] read failed\n", regs[i]);
        }
    }

    return 0;
}
MSH_CMD_EXPORT(cam_sensor_status, dump key OV5640 registers);
#endif

static int cam_capture(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (wait_for_camera_frame() != 0)
    {
        return -1;
    }

#if defined(BSP_USING_SDCARD_FATFS)
    print_frame_brightness();
    save_camera_raw_frame("/sdcard/frame.raw");
#else
    print_frame_brightness();
    rt_kprintf("SD save disabled: frame is in RAM at %p\n", (void *) camera_data_ready_buffer_pointer_get());
#endif

    return 0;
}
MSH_CMD_EXPORT(cam_capture, capture one frame);

static void cam_lcd_live_thread_entry(void *parameter)
{
    uint32_t fps = (uint32_t) parameter;
    uint32_t delay_ms;

    if (fps == 0)
    {
        fps = 10;
    }

    if (fps > 30)
    {
        fps = 30;
    }

    delay_ms = 1000U / fps;
    if (delay_ms == 0)
    {
        delay_ms = 1;
    }

    rt_kprintf("LCD live view started, fps=%d\n", fps);

    while (lcd_live_loop_running)
    {
        if (!camera_processing_busy)
        {
            wait_for_camera_frame();
        }
        rt_thread_mdelay(delay_ms);
    }

    rt_kprintf("LCD live view stopped\n");
}

static int cam_lcd_live(int argc, char **argv)
{
    rt_thread_t tid;
    uint32_t fps = 10;

    if (argc >= 2)
    {
        if (!parse_hex_or_dec(argv[1], &fps) || (fps == 0))
        {
            rt_kprintf("usage: cam_lcd_live [fps]\n");
            return -1;
        }
    }

#if !defined(BSP_USING_LCD)
    rt_kprintf("LCD support disabled: enable BSP_USING_LCD first\n");
    return -1;
#endif

    if (!camera_started)
    {
        rt_kprintf("camera not started. Run: cam_start\n");
        return -1;
    }

    if (lcd_live_loop_running)
    {
        rt_kprintf("LCD live view already running\n");
        return 0;
    }

    lcd_live_loop_running = RT_TRUE;

    tid = rt_thread_create("lcd_live",
                           cam_lcd_live_thread_entry,
                           (void *) fps,
                           4096,
                           27,
                           10);
    if (tid == RT_NULL)
    {
        lcd_live_loop_running = RT_FALSE;
        rt_kprintf("create LCD live thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(cam_lcd_live, show live camera preview on LCD: cam_lcd_live [fps]);

static int cam_lcd_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!lcd_live_loop_running && !lcd_ai_live_loop_running)
    {
        rt_kprintf("LCD live view is not running\n");
        return 0;
    }

    lcd_live_loop_running = RT_FALSE;
    lcd_ai_live_loop_running = RT_FALSE;
    rt_kprintf("stopping LCD live view...\n");

    return 0;
}
MSH_CMD_EXPORT(cam_lcd_stop, stop LCD live preview);

static void cam_lcd_ai_live_thread_entry(void *parameter)
{
    uint32_t fps = (uint32_t) parameter;
    uint32_t delay_ms;

    if (fps == 0)
    {
        fps = 5;
    }

    if (fps > 15)
    {
        fps = 15;
    }

    delay_ms = 1000U / fps;
    if (delay_ms == 0)
    {
        delay_ms = 1;
    }

    rt_kprintf("LCD AI live view started, fps=%d\n", fps);

    while (lcd_ai_live_loop_running)
    {
        run_face_detection(RT_FALSE, RT_FALSE);
        rt_thread_mdelay(delay_ms);
    }

    rt_kprintf("LCD AI live view stopped\n");
}

static int cam_lcd_ai_live(int argc, char **argv)
{
    rt_thread_t tid;
    uint32_t fps = 5;

    if (argc >= 2)
    {
        if (!parse_hex_or_dec(argv[1], &fps) || (fps == 0))
        {
            rt_kprintf("usage: cam_lcd_ai_live [fps]\n");
            return -1;
        }
    }

#if !defined(BSP_USING_LCD)
    rt_kprintf("LCD support disabled: enable BSP_USING_LCD first\n");
    return -1;
#endif

    if (!camera_started)
    {
        rt_kprintf("camera not started. Run: cam_start\n");
        return -1;
    }

    if (lcd_ai_live_loop_running)
    {
        rt_kprintf("LCD AI live view already running\n");
        return 0;
    }

    lcd_live_loop_running = RT_FALSE;
    lcd_ai_live_loop_running = RT_TRUE;

    tid = rt_thread_create("lcd_ai",
                           cam_lcd_ai_live_thread_entry,
                           (void *) fps,
                           8192,
                           26,
                           10);
    if (tid == RT_NULL)
    {
        lcd_ai_live_loop_running = RT_FALSE;
        rt_kprintf("create LCD AI live thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(cam_lcd_ai_live, show live camera preview with face boxes on LCD: cam_lcd_ai_live [fps]);

static int cam_detect(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    return run_face_detection(RT_TRUE, RT_FALSE);
}
MSH_CMD_EXPORT(cam_detect, capture one frame and run NPU face detection);

static int send_camera_frame_to_host(int16_t face_count)
{
    rt_device_t console;
    uint8_t *p_img = (uint8_t *) camera_data_ready_buffer_pointer_get();
    int image_size = CAMERA_CAPTURE_IMAGE_WIDTH *
                     CAMERA_CAPTURE_IMAGE_HEIGHT *
                     CAMERA_IMAGE_BYTE_PER_PIXEL;
    int sent_total = 0;

    console = rt_console_get_device();
    if (console == RT_NULL)
    {
        rt_kprintf("serial frame send failed: no console device\n");
        return -1;
    }

    rt_kprintf("\n@@FRAME_BEGIN width=%d height=%d format=RGB565 size=%d faces=%d@@\n",
               CAMERA_CAPTURE_IMAGE_WIDTH,
               CAMERA_CAPTURE_IMAGE_HEIGHT,
               image_size,
               face_count);

    while (sent_total < image_size)
    {
        int chunk = image_size - sent_total;
        if (chunk > SERIAL_FRAME_CHUNK_BYTES)
        {
            chunk = SERIAL_FRAME_CHUNK_BYTES;
        }

        rt_device_write(console, 0, p_img + sent_total, chunk);
        sent_total += chunk;
    }

    rt_kprintf("\n@@FRAME_END@@\n");
    rt_kprintf("serial frame sent, bytes=%d\n", sent_total);

    return 0;
}

static int run_face_detection(rt_bool_t print_boxes, rt_bool_t send_on_face)
{
    static int8_t *in_i8 = RT_NULL;
    static float *out_f1 = RT_NULL;
    static float *out_f2 = RT_NULL;
    static det_box_t boxes[DETECT_POOL_BOX_COUNT];
    rt_tick_t start;
    rt_tick_t end;
    int16_t total = 0;
    int16_t kept = 0;
    int result = -1;

    camera_processing_busy = RT_TRUE;

    if (wait_for_camera_frame() != 0)
    {
        goto exit;
    }

    if (ensure_npu_started() != 0)
    {
        goto exit;
    }

    if (in_i8 == RT_NULL)
    {
        in_i8 = (int8_t *) rt_malloc(INPUT_SIZE * sizeof(int8_t));
    }

    if (out_f1 == RT_NULL)
    {
        out_f1 = (float *) rt_malloc(output1_len * sizeof(float));
    }

    if (out_f2 == RT_NULL)
    {
        out_f2 = (float *) rt_malloc(output2_len * sizeof(float));
    }

    if ((in_i8 == RT_NULL) || (out_f1 == RT_NULL) || (out_f2 == RT_NULL))
    {
        rt_kprintf("face detect malloc failed\n");
        goto exit;
    }

    start = rt_tick_get();

    print_frame_brightness();

    rgb565_to_gray_resize_192_and_quantization((uint16_t *) camera_data_ready_buffer_pointer_get(),
                                               CAMERA_CAPTURE_IMAGE_WIDTH,
                                               CAMERA_CAPTURE_IMAGE_HEIGHT,
                                               in_i8);

    rt_memcpy(GetModelInputPtr_serving_default_image_input_0(), in_i8, INPUT_SIZE);
    RunModel(true);

    dequantize_int8(GetModelOutputPtr_StatefulPartitionedCall_0_70273(),
                    out_f1,
                    output1_len,
                    scale_out1,
                    zero_point_out1);
    dequantize_int8(GetModelOutputPtr_StatefulPartitionedCall_1_70283(),
                    out_f2,
                    output2_len,
                    scale_out2,
                    zero_point_out2);

    total += decode_output_layer(out_f1,
                                 GRID_SIZE_1,
                                 0,
                                 CAMERA_CAPTURE_IMAGE_WIDTH,
                                 CAMERA_CAPTURE_IMAGE_HEIGHT,
                                 CONF_THRESH,
                                 boxes + total,
                                 (int16_t) (DETECT_POOL_BOX_COUNT - total));
    total += decode_output_layer(out_f2,
                                 GRID_SIZE_2,
                                 1,
                                 CAMERA_CAPTURE_IMAGE_WIDTH,
                                 CAMERA_CAPTURE_IMAGE_HEIGHT,
                                 CONF_THRESH,
                                 boxes + total,
                                 (int16_t) (DETECT_POOL_BOX_COUNT - total));

    kept = nms_filter(boxes, total, NMS_THRESH);
    if (kept > MAX_BOXES)
    {
        kept = MAX_BOXES;
    }

    end = rt_tick_get();

    if (print_boxes)
    {
        rt_kprintf("face detect boxes=%d, raw_boxes=%d, threshold=%d%%, time=%d ms\n",
                   kept,
                   total,
                   (int) (CONF_THRESH * 100.0f),
                   (end - start) * (1000 / RT_TICK_PER_SECOND));
    }

    if (print_boxes)
    {
        for (int16_t i = 0; i < kept; i++)
        {
            rt_kprintf("face[%d]: x1=%d y1=%d x2=%d y2=%d score=%d%%\n",
                       i,
                       boxes[i].x1,
                       boxes[i].y1,
                       boxes[i].x2,
                       boxes[i].y2,
                       (int) (boxes[i].score * 100.0f));
        }
    }

    draw_face_overlay(boxes, kept);

    if (send_on_face && (kept > 0))
    {
        send_camera_frame_to_host(kept);
    }

    result = kept;

exit:
    camera_processing_busy = RT_FALSE;
    return result;
}

static void cam_detect_loop_thread_entry(void *parameter)
{
    uint32_t interval_seconds = (uint32_t) parameter;

    if (interval_seconds == 0)
    {
        interval_seconds = 5;
    }

    rt_kprintf("face detect loop started, interval=%d seconds\n", interval_seconds);

    while (detect_loop_running)
    {
        cam_detect(0, RT_NULL);

        for (uint32_t i = 0; (i < interval_seconds * 10U) && detect_loop_running; i++)
        {
            rt_thread_mdelay(100);
        }
    }

    rt_kprintf("face detect loop stopped\n");
}

static int cam_detect_loop(int argc, char **argv)
{
    rt_thread_t tid;
    uint32_t interval_seconds = 5;

    if (argc >= 2)
    {
        if (!parse_hex_or_dec(argv[1], &interval_seconds) || (interval_seconds == 0))
        {
            rt_kprintf("usage: cam_detect_loop [seconds]\n");
            return -1;
        }
    }

    if (detect_loop_running)
    {
        rt_kprintf("face detect loop already running\n");
        return 0;
    }

    detect_loop_running = RT_TRUE;

    tid = rt_thread_create("detect_loop",
                           cam_detect_loop_thread_entry,
                           (void *) interval_seconds,
                           8192,
                           26,
                           10);
    if (tid == RT_NULL)
    {
        detect_loop_running = RT_FALSE;
        rt_kprintf("create face detect loop thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(cam_detect_loop, run face detection repeatedly: cam_detect_loop [seconds]);

static int cam_detect_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!detect_loop_running)
    {
        rt_kprintf("face detect loop is not running\n");
        return 0;
    }

    detect_loop_running = RT_FALSE;
    rt_kprintf("stopping face detect loop...\n");

    return 0;
}
MSH_CMD_EXPORT(cam_detect_stop, stop face detection loop);

static void cam_face_send_loop_thread_entry(void *parameter)
{
    uint32_t interval_seconds = (uint32_t) parameter;

    if (interval_seconds == 0)
    {
        interval_seconds = 5;
    }

    rt_kprintf("face-send loop started, interval=%d seconds\n", interval_seconds);
    rt_kprintf("close normal terminal and use Python receiver for binary frames\n");

    while (face_send_loop_running)
    {
        int faces = run_face_detection(RT_TRUE, RT_TRUE);
        if (faces <= 0)
        {
            rt_kprintf("no face, nothing sent\n");
        }

        for (uint32_t i = 0; (i < interval_seconds * 10U) && face_send_loop_running; i++)
        {
            rt_thread_mdelay(100);
        }
    }

    rt_kprintf("face-send loop stopped\n");
}

static int cam_face_send_loop(int argc, char **argv)
{
    rt_thread_t tid;
    uint32_t interval_seconds = 5;

    if (argc >= 2)
    {
        if (!parse_hex_or_dec(argv[1], &interval_seconds) || (interval_seconds == 0))
        {
            rt_kprintf("usage: cam_face_send_loop [seconds]\n");
            return -1;
        }
    }

    if (face_send_loop_running)
    {
        rt_kprintf("face-send loop already running\n");
        return 0;
    }

    face_send_loop_running = RT_TRUE;

    tid = rt_thread_create("face_send",
                           cam_face_send_loop_thread_entry,
                           (void *) interval_seconds,
                           8192,
                           26,
                           10);
    if (tid == RT_NULL)
    {
        face_send_loop_running = RT_FALSE;
        rt_kprintf("create face-send loop thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(cam_face_send_loop, detect faces repeatedly and send matched frames over serial);

static int cam_face_send_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!face_send_loop_running)
    {
        rt_kprintf("face-send loop is not running\n");
        return 0;
    }

    face_send_loop_running = RT_FALSE;
    rt_kprintf("stopping face-send loop...\n");

    return 0;
}
MSH_CMD_EXPORT(cam_face_send_stop, stop serial face-send loop);

static int cam_upload_set(int argc, char **argv)
{
    uint32_t port;

    if (argc < 2)
    {
        rt_kprintf("upload target: http://%s:%d/upload\n", upload_host, upload_port);
        rt_kprintf("usage: cam_upload_set <host-ip> [port]\n");
        return 0;
    }

    rt_strncpy(upload_host, argv[1], sizeof(upload_host) - 1);
    upload_host[sizeof(upload_host) - 1] = '\0';

    if (argc >= 3)
    {
        if (!parse_hex_or_dec(argv[2], &port) || (port == 0) || (port > 65535))
        {
            rt_kprintf("bad port: %s\n", argv[2]);
            return -1;
        }

        upload_port = port;
    }

    rt_kprintf("upload target set: http://%s:%d/upload\n", upload_host, upload_port);

    return 0;
}
MSH_CMD_EXPORT(cam_upload_set, set Python API upload host: cam_upload_set 192.168.101.123 5000);

static void cam_face_upload_loop_thread_entry(void *parameter)
{
    uint32_t interval_seconds = (uint32_t) parameter;

    if (interval_seconds == 0)
    {
        interval_seconds = 5;
    }

    rt_kprintf("face-upload loop started, interval=%d seconds, target=http://%s:%d/upload\n",
               interval_seconds,
               upload_host,
               upload_port);

    while (face_upload_loop_running)
    {
        int faces = run_face_detection(RT_TRUE, RT_FALSE);
        if (faces > 0)
        {
            upload_camera_frame_to_host((int16_t) faces);
        }
        else
        {
            rt_kprintf("no face, nothing uploaded\n");
        }

        for (uint32_t i = 0; (i < interval_seconds * 10U) && face_upload_loop_running; i++)
        {
            rt_thread_mdelay(100);
        }
    }

    rt_kprintf("face-upload loop stopped\n");
}

static int cam_face_upload_loop(int argc, char **argv)
{
    rt_thread_t tid;
    uint32_t interval_seconds = 5;

    if (argc >= 2)
    {
        if (!parse_hex_or_dec(argv[1], &interval_seconds) || (interval_seconds == 0))
        {
            rt_kprintf("usage: cam_face_upload_loop [seconds]\n");
            return -1;
        }
    }

    if (face_upload_loop_running)
    {
        rt_kprintf("face-upload loop already running\n");
        return 0;
    }

    face_upload_loop_running = RT_TRUE;

    tid = rt_thread_create("face_http",
                           cam_face_upload_loop_thread_entry,
                           (void *) interval_seconds,
                           8192,
                           26,
                           10);
    if (tid == RT_NULL)
    {
        face_upload_loop_running = RT_FALSE;
        rt_kprintf("create face-upload loop thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(cam_face_upload_loop, detect faces repeatedly and upload matched frames by Ethernet HTTP);

static int cam_face_upload_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!face_upload_loop_running)
    {
        rt_kprintf("face-upload loop is not running\n");
        return 0;
    }

    face_upload_loop_running = RT_FALSE;
    rt_kprintf("stopping face-upload loop...\n");

    return 0;
}
MSH_CMD_EXPORT(cam_face_upload_stop, stop Ethernet face-upload loop);

#if defined(RT_USING_LWIP) && defined(BSP_USING_ETH)
static int socket_send_all(int sock, const uint8_t *data, int len)
{
    int sent_total = 0;

    while (sent_total < len)
    {
        int sent = send(sock, data + sent_total, len - sent_total, 0);
        if (sent <= 0)
        {
            return -1;
        }

        sent_total += sent;
    }

    return 0;
}

static int upload_camera_frame_to_host(int16_t face_count)
{
    int sock = -1;
    int image_size = CAMERA_CAPTURE_IMAGE_WIDTH *
                     CAMERA_CAPTURE_IMAGE_HEIGHT *
                     CAMERA_IMAGE_BYTE_PER_PIXEL;
    uint8_t *p_img = (uint8_t *) camera_data_ready_buffer_pointer_get();
    struct hostent *host;
    struct sockaddr_in server_addr;
    char header[384];
    int header_len;
    int sent_total = 0;
    int timeout = 5000;

    host = gethostbyname(upload_host);
    if (host == RT_NULL)
    {
        rt_kprintf("upload failed: resolve host %s failed\n", upload_host);
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
        rt_kprintf("upload failed: socket error\n");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) upload_port);
    server_addr.sin_addr = *((struct in_addr *) host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0)
    {
        rt_kprintf("upload failed: connect %s:%d failed\n", upload_host, upload_port);
        closesocket(sock);
        return -1;
    }

    header_len = rt_snprintf(header,
                             sizeof(header),
                             "POST /upload HTTP/1.1\r\n"
                             "Host: %s:%d\r\n"
                             "Content-Type: application/octet-stream\r\n"
                             "Content-Length: %d\r\n"
                             "X-Width: %d\r\n"
                             "X-Height: %d\r\n"
                             "X-Format: RGB565\r\n"
                             "X-Faces: %d\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             upload_host,
                             upload_port,
                             image_size,
                             CAMERA_CAPTURE_IMAGE_WIDTH,
                             CAMERA_CAPTURE_IMAGE_HEIGHT,
                             face_count);

    if ((header_len <= 0) || (header_len >= (int) sizeof(header)) ||
        (socket_send_all(sock, (const uint8_t *) header, header_len) != 0))
    {
        rt_kprintf("upload failed: send header failed\n");
        closesocket(sock);
        return -1;
    }

    while (sent_total < image_size)
    {
        int chunk = image_size - sent_total;
        if (chunk > HTTP_UPLOAD_CHUNK_BYTES)
        {
            chunk = HTTP_UPLOAD_CHUNK_BYTES;
        }

        if (socket_send_all(sock, p_img + sent_total, chunk) != 0)
        {
            rt_kprintf("upload failed: sent=%d/%d\n", sent_total, image_size);
            closesocket(sock);
            return -1;
        }

        sent_total += chunk;
    }

    rt_kprintf("uploaded frame to http://%s:%d/upload, bytes=%d, faces=%d\n",
               upload_host,
               upload_port,
               sent_total,
               face_count);

    closesocket(sock);
    return 0;
}
#else
static int upload_camera_frame_to_host(int16_t face_count)
{
    RT_UNUSED(face_count);
    rt_kprintf("Ethernet upload disabled: enable BSP_USING_ETH + RT_USING_LWIP/SAL/NETDEV first\n");
    return -1;
}
#endif

static int cam_status(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("camera_started=%d, frame=%dx%d RGB565, buffer=%p\n",
               camera_started,
               CAMERA_CAPTURE_IMAGE_WIDTH,
               CAMERA_CAPTURE_IMAGE_HEIGHT,
               (void *) camera_data_ready_buffer_pointer_get());
    rt_kprintf("camera_starting=%d\n", camera_starting);
    rt_kprintf("vin_starting=%d\n", vin_starting);
    rt_kprintf("vin_started=%d\n", vin_started);
    rt_kprintf("npu_started=%d\n", npu_started);
    rt_kprintf("detect_loop_running=%d\n", detect_loop_running);
    rt_kprintf("face_send_loop_running=%d\n", face_send_loop_running);
    rt_kprintf("lcd_live_loop_running=%d\n", lcd_live_loop_running);
    rt_kprintf("lcd_ai_live_loop_running=%d\n", lcd_ai_live_loop_running);
    rt_kprintf("camera_processing_busy=%d\n", camera_processing_busy);
    rt_kprintf("face_upload_loop_running=%d, upload_target=http://%s:%d/upload\n",
               face_upload_loop_running,
               upload_host,
               upload_port);
#if defined(BSP_USING_CEU_CAMERA)
    rt_kprintf("ceu_frames=%d, buffer=%p\n",
               camera_vin_frame_count_get(),
               (void *) camera_vin_last_buffer_get());
#else
    rt_kprintf("vin_frames=%d, vin_last_buf=%p, vin_last_event=%d, vin_last_status=0x%08x\n",
               camera_vin_frame_count_get(),
               (void *) camera_vin_last_buffer_get(),
               camera_vin_last_event_get(),
               camera_vin_last_status_get());
    rt_kprintf("mipi_callbacks=%d\n", camera_mipi_callback_count_get());

    if (vin_started)
    {
        capture_status_t vin_status;
        fsp_err_t err;

        rt_memset(&vin_status, 0, sizeof(vin_status));
        err = R_VIN_StatusGet(&g_cam_vin_ctrl, &vin_status);
        rt_kprintf("vin_status_get err=%d, state=%d, data_size=%d, active_buf=%p\n",
                   err,
                   vin_status.state,
                   vin_status.data_size,
                   vin_status.p_buffer);
    }
#endif

#if defined(BSP_USING_SDCARD_FATFS)
    rt_kprintf("SD save path: /sdcard/frame.raw\n");
#else
    rt_kprintf("SD save disabled\n");
#endif

    return 0;
}
MSH_CMD_EXPORT(cam_status, show camera status);

#if !defined(BSP_USING_CEU_CAMERA)
static int cam_hw_status(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("MIPI_PHY: DPHYSFR=0x%08x, DPHYOCR=0x%08x\n",
               R_MIPI_PHY->DPHYSFR,
               R_MIPI_PHY->DPHYOCR);
    rt_kprintf("MIPI_CSI: MCG=0x%08x, MCT0=0x%08x, MCT2=0x%08x, MCT3=0x%08x, RTST=0x%08x\n",
               R_MIPI_CSI->MCG,
               R_MIPI_CSI->MCT0,
               R_MIPI_CSI->MCT2,
               R_MIPI_CSI->MCT3,
               R_MIPI_CSI->RTST);
    rt_kprintf("MIPI_CSI: MIST=0x%08x, RXIE=0x%08x, DLIE0=0x%08x, DLIE1=0x%08x\n",
               R_MIPI_CSI->MIST,
               R_MIPI_CSI->RXIE,
               R_MIPI_CSI->DLIE0,
               R_MIPI_CSI->DLIE1);
    rt_kprintf("MIPI_CSI: DLST0=0x%08x, DLST1=0x%08x, VCST0=0x%08x, DTEL=0x%08x\n",
               R_MIPI_CSI->DLST0,
               R_MIPI_CSI->DLST1,
               R_MIPI_CSI->VCST0,
               R_MIPI_CSI->DTEL);
    rt_kprintf("VIN: MC=0x%08x, FC=0x%08x, MS=0x%08x, INTS=0x%08x, IE=0x%08x, LC=%d\n",
               R_VIN->MC,
               R_VIN->FC,
               R_VIN->MS,
               R_VIN->INTS,
               R_VIN->IE,
               R_VIN->LC);
    rt_kprintf("VIN: MB1=0x%08x, MB2=0x%08x, MB3=0x%08x\n",
               R_VIN->MB1,
               R_VIN->MB2,
               R_VIN->MB3);

    return 0;
}
MSH_CMD_EXPORT(cam_hw_status, dump VIN and MIPI CSI hardware registers);
#endif

static void titan_auto_start_thread_entry(void *parameter)
{
    uint32_t timeout_ms = 15000;
    uint32_t elapsed_ms = 0;
    char *lcd_ai_argv[] = {"cam_lcd_ai_live", TITAN_AUTO_LCD_AI_FPS_TEXT};
    char *upload_argv[] = {"cam_face_upload_loop", TITAN_AUTO_UPLOAD_INTERVAL_TEXT};

    RT_UNUSED(parameter);

    rt_kprintf("Titan auto-start waiting %d ms before camera start...\n", TITAN_AUTO_START_DELAY_MS);
    rt_thread_mdelay(TITAN_AUTO_START_DELAY_MS);

    rt_kprintf("Titan auto-start: starting camera/VIN\n");
    cam_start(0, RT_NULL);

    while ((!camera_started || !vin_started) && (elapsed_ms < timeout_ms))
    {
        rt_thread_mdelay(200);
        elapsed_ms += 200;
    }

    if (!camera_started || !vin_started)
    {
        rt_kprintf("Titan auto-start: camera/VIN not ready after %d ms, stop auto sequence\n", timeout_ms);
        return;
    }

    rt_kprintf("Titan auto-start: camera ready, starting LCD AI live\n");
    cam_lcd_ai_live(2, lcd_ai_argv);

    rt_kprintf("Titan auto-start: starting face upload loop\n");
    cam_face_upload_loop(2, upload_argv);
}

static int cam_auto_start(int argc, char **argv)
{
    rt_thread_t tid;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    tid = rt_thread_create("auto_cam",
                           titan_auto_start_thread_entry,
                           RT_NULL,
                           4096,
                           28,
                           10);
    if (tid == RT_NULL)
    {
        rt_kprintf("create Titan auto-start thread failed\n");
        return -1;
    }

    rt_thread_startup(tid);
    rt_kprintf("Titan auto-start thread created\n");

    return 0;
}
MSH_CMD_EXPORT(cam_auto_start, start camera LCD AI live and face upload loop);

void hal_entry(void)
{
    if (!camera_frame_mutex_ready)
    {
        rt_mutex_init(&camera_frame_mutex, "camfrm", RT_IPC_FLAG_PRIO);
        camera_frame_mutex_ready = RT_TRUE;
    }

    rt_kprintf ("\nHello Titan Board!\n");
    rt_kprintf ("===========================================================\n");
#if defined(BSP_USING_CEU_CAMERA)
    rt_kprintf ("This example project is a CEU camera capture routine!\n");
#else
    rt_kprintf ("This example project is an mipi-csi camera capture routine!\n");
#endif
    rt_kprintf ("===========================================================\n");
    rt_kprintf ("Type: cam_start\n");
#if defined(BSP_USING_CEU_CAMERA)
    rt_kprintf ("Then: cam_lcd_live 10       (plain live LCD)\n");
    rt_kprintf ("Or:   cam_lcd_ai_live 5     (LCD with face boxes)\n");
    rt_kprintf ("Also: cam_capture / cam_detect / cam_face_upload_loop 5\n");
#else
    rt_kprintf ("Then wait for VIN capture started\n");
    rt_kprintf ("Then: cam_capture\n");
#endif
#if TITAN_AUTO_START_ENABLE
    rt_kprintf ("Auto: camera + LCD AI + face upload will start after boot\n");
    cam_auto_start(0, RT_NULL);
#else
    rt_kprintf ("Auto: disabled. Type cam_auto_start to start all\n");
#endif
}
