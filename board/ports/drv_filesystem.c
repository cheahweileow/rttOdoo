/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-13     balanceTWK   add sdcard port file
 * 2021-05-10     Meco Man     fix a bug that cannot use fatfs in the main thread at starting up
 * 2021-07-28     Meco Man     implement romfs as the root filesystem
 */

#include <rtthread.h>

#if defined(BSP_USING_FILESYSTEM)
#include <dfs_romfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>

#if DFS_FILESYSTEMS_MAX < 4
#error "Please define DFS_FILESYSTEMS_MAX more than 4"
#endif
#if DFS_FILESYSTEM_TYPES_MAX < 4
#error "Please define DFS_FILESYSTEM_TYPES_MAX more than 4"
#endif

#define DBG_TAG "app.filesystem"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#ifdef BSP_USING_FS_AUTO_MOUNT
#ifdef BSP_USING_SDCARD_FATFS
static rt_err_t try_mount_sdcard_once(void)
{
    if (dfs_mount("sd0", "/sdcard", "elm", 0, 0) == RT_EOK)
    {
        LOG_I("SD card mount to '/sdcard' from sd0");
        return RT_EOK;
    }

    if (dfs_mount("sd", "/sdcard", "elm", 0, 0) == RT_EOK)
    {
        LOG_I("SD card mount to '/sdcard' from sd");
        return RT_EOK;
    }

    return -RT_ERROR;
}

static int onboard_sdcard_mount(void)
{
    int retry;

    for (retry = 0; retry < 10; retry++)
    {
        if (try_mount_sdcard_once() == RT_EOK)
        {
            return RT_EOK;
        }

        rt_thread_mdelay(500);
    }

    LOG_E("SD card mount to '/sdcard' failed!");
    rt_pin_write(0x000D, PIN_LOW);

    return -RT_ERROR;
}

static int sd_mount(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (try_mount_sdcard_once() == RT_EOK)
    {
        rt_kprintf("sdcard mounted at /sdcard\n");
    }
    else
    {
        rt_kprintf("sdcard mount failed\n");
    }

    return 0;
}
MSH_CMD_EXPORT(sd_mount, mount SD card to /sdcard);

static rt_uint32_t sd_rd_le32(const rt_uint8_t *p)
{
    return ((rt_uint32_t)p[0]) |
           ((rt_uint32_t)p[1] << 8) |
           ((rt_uint32_t)p[2] << 16) |
           ((rt_uint32_t)p[3] << 24);
}

static rt_uint16_t sd_rd_le16(const rt_uint8_t *p)
{
    return (rt_uint16_t)(((rt_uint16_t)p[0]) | ((rt_uint16_t)p[1] << 8));
}

static void sd_print_ascii_field(const char *name, const rt_uint8_t *p, int len)
{
    int i;

    rt_kprintf("%s='", name);
    for (i = 0; i < len; i++)
    {
        char c = (char)p[i];
        rt_kprintf("%c", (c >= 32 && c <= 126) ? c : '.');
    }
    rt_kprintf("'\n");
}

static void sd_dump_device(const char *dev_name)
{
    rt_uint8_t *buf;
    rt_device_t dev;
    rt_size_t result;

    dev = rt_device_find(dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("%s: not found\n", dev_name);
        return;
    }

    buf = (rt_uint8_t *)rt_malloc(512);
    if (buf == RT_NULL)
    {
        rt_kprintf("%s: no memory\n", dev_name);
        return;
    }

    rt_memset(buf, 0, 512);
    rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    result = rt_device_read(dev, 0, buf, 1);
    rt_kprintf("%s: read result=%d sig=%02x%02x\n", dev_name, result, buf[511], buf[510]);

    sd_print_ascii_field("  jump/oem", &buf[0], 11);
    rt_kprintf("  bps=%d spc=%d reserved=%d fats=%d root_entries=%d\n",
               sd_rd_le16(&buf[11]), buf[13], sd_rd_le16(&buf[14]), buf[16], sd_rd_le16(&buf[17]));
    rt_kprintf("  total16=%d total32=%d fatsz16=%d fatsz32=%d hidden=%d\n",
               sd_rd_le16(&buf[19]), sd_rd_le32(&buf[32]), sd_rd_le16(&buf[22]),
               sd_rd_le32(&buf[36]), sd_rd_le32(&buf[28]));
    sd_print_ascii_field("  fs16", &buf[54], 8);
    sd_print_ascii_field("  fs32", &buf[82], 8);
    rt_kprintf("  mbr part0 type=0x%02x start=%d size=%d\n",
               buf[0x1BE + 4], sd_rd_le32(&buf[0x1BE + 8]), sd_rd_le32(&buf[0x1BE + 12]));

    rt_free(buf);
}

static int sd_dump(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    sd_dump_device("sd");
    sd_dump_device("sd0");
    return 0;
}
MSH_CMD_EXPORT(sd_dump, dump SD and SD0 first sector);
#endif /* BSP_USING_SDCARD_FATFS */
#endif /* BSP_USING_FS_AUTO_MOUNT */

#ifdef BSP_USING_FLASH_FS_AUTO_MOUNT
#ifdef BSP_USING_FLASH_FATFS
#define FS_PARTITION_NAME "filesystem"

static int onboard_fal_mount(void)
{
    /* 初始化 fal 功能 */
    extern int fal_init(void);
    extern struct rt_device* fal_mtd_nor_device_create(const char *parition_name);
    fal_init ();
    /* 在 ospi flash 中名为 "filesystem" 的分区上创建一个块设备 */
    struct rt_device *mtd_dev = fal_mtd_nor_device_create (FS_PARTITION_NAME);
    if (mtd_dev == NULL)
    {
        LOG_E("Can't create a mtd device on '%s' partition.", FS_PARTITION_NAME);
        return -RT_ERROR;
    }
    else
    {
        LOG_D("Create a mtd device on the %s partition of flash successful.", FS_PARTITION_NAME);
    }

    /* 挂载 ospi flash 中名为 "filesystem" 的分区上的文件系统 */
    if (dfs_mount (FS_PARTITION_NAME, "/fal", "lfs", 0, 0) == 0)
    {
        LOG_I("Filesystem initialized!");
    }
    else
    {
        dfs_mkfs ("lfs", FS_PARTITION_NAME);
        if (dfs_mount ("filesystem", "/fal", "lfs", 0, 0) == 0)
        {
            LOG_I("Filesystem initialized!");
        }
        else
        {
            LOG_E("Failed to initialize filesystem!");
            rt_pin_write(0x000D, PIN_LOW);
        }
    }

    return RT_EOK;
}
#endif /*BSP_USING_FLASH_FATFS*/
#endif /*BSP_USING_FLASH_FS_AUTO_MOUNT*/

const struct romfs_dirent _romfs_root[] =
{
#ifdef BSP_USING_SDCARD_FATFS
    {ROMFS_DIRENT_DIR, "sdcard", RT_NULL, 0},
#endif

#ifdef BSP_USING_FLASH_FATFS
  { ROMFS_DIRENT_DIR, "fal", RT_NULL, 0 },
#endif
        };

const struct romfs_dirent romfs_root =
{
ROMFS_DIRENT_DIR, "/", (rt_uint8_t*) _romfs_root, sizeof(_romfs_root) / sizeof(_romfs_root[0])
};

static int filesystem_mount(void)
{

#ifdef RT_USING_DFS_ROMFS
    if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) != 0)
    {
        LOG_E("rom mount to '/' failed!");
    }

    /* 确保块设备注册成功之后再挂载文件系统 */
    rt_thread_delay(500);
#endif
#ifdef BSP_USING_FS_AUTO_MOUNT
    onboard_sdcard_mount();
#endif /* BSP_USING_FS_AUTO_MOUNT */

#ifdef BSP_USING_FLASH_FS_AUTO_MOUNT
    onboard_fal_mount ();
#endif

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(filesystem_mount);
#endif /* defined(BSP_USING_FILESYSTEM)*/
