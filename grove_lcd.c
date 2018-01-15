/*
 * grove_lcd_driver.c
 * Copyright (C) 2017 Yuhei Horibe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This is a driver for Grove Pi LCD color display via i2c.
 *
 */

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
//#include <linux/regulator/consumer.h>
//#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/bitops.h>

#include "grove_lcd_cmd.h"


// ******************************************
// Address and register definitions
// ******************************************
// Addresses
#define GROVE_LCD_TEXT        0x3e
#define GROVE_LCD_BACK_LIGHT  0x62

// Registers
// Character registers
#define TXT_REG_DISPLAY           0x80
#define TXT_REG_LETTERS           0x40

// Back light
#define BKL_REG_MODE1             0x00
#define BKL_REG_MODE2             0x01
#define BKL_REG_PWM_BLUE          0x02
#define BKL_REG_PWM_GREEN         0x03
#define BKL_REG_PWM_RED           0x04
#define BKL_REG_LED_OUT           0x05


// ******************************************
// i2c device definitions
// ******************************************
// Device name
#define GROVE_LCD_NAME "grove_lcd"

// i2c client
static int                grove_lcd_major;  // Major number
static struct i2c_client* grove_lcd_client;
static struct class*      grove_lcd_class = NULL;
static struct device*     grove_lcd_data  = NULL;

// Grove Pi LCD device
struct grove_lcd_device
{
    // Mutex for i2c client device
    struct mutex update_lock;
    int          grove_lcd_major;
};

// Mutex for this device (file)
static DEFINE_MUTEX(grove_lcd_mutex);

// ******************************************
// Sub functions
// ******************************************
int grove_lcd_read_val(struct i2c_client* client, u8 reg)
{
    struct grove_lcd_device* data = i2c_get_clientdata(client);
    int    ret_val = 0;

    dev_info(&client->dev, "%s\n", __FUNCTION__);

    mutex_lock(&data->update_lock);
    ret_val = i2c_smbus_read_byte_data(client, reg);
    mutex_unlock(&data->update_lock);

    dev_info(&client->dev, "%s : read reg [%02x] returned [%d]\n", __FUNCTION__, reg, ret_val);

    return ret_val;
}

int grove_lcd_write_val(struct i2c_client* client, u8 reg, u16 value)
{
    struct grove_lcd_device* data = i2c_get_clientdata(client);
    int    ret_val = 0;

    dev_info(&client->dev, "%s\n", __FUNCTION__);

    mutex_lock(&data->update_lock);
    ret_val = i2c_smbus_write_byte_data(client, reg, value);
    mutex_unlock(&data->update_lock);

    dev_info(&client->dev, "%s : write reg [%02x] with val [%02x] returned [%d]\n", __FUNCTION__, reg, value, ret_val);

    return ret_val;
}

static int grove_lcd_init(struct i2c_client* client)
{
    //union grove_lcd_cmd cmd;
    dev_info(&client->dev, "%s\n", __FUNCTION__);

    // Initialize backlight
    client->addr = GROVE_LCD_BACK_LIGHT;
    grove_lcd_write_val(client, BKL_REG_MODE1,     0x00);
    grove_lcd_write_val(client, BKL_REG_MODE2,     0x00);
    grove_lcd_write_val(client, BKL_REG_PWM_BLUE,  0x7F); // Blue
    grove_lcd_write_val(client, BKL_REG_PWM_GREEN, 0x7F); // Green
    grove_lcd_write_val(client, BKL_REG_PWM_RED,   0x7F); // Red
    grove_lcd_write_val(client, BKL_REG_LED_OUT,   0xAA);
    return 0;
}

// ******************************************
// Define file operations
// ******************************************
static int grove_lcd_open(struct inode* inode, struct file* file)
{
    // Check if the other process is using it or not
    if (!mutex_trylock(&grove_lcd_mutex)) {
        printk("%s: Device currently in use!\n", __FUNCTION__);
        return -EBUSY;
    }

    // Make sure if the i2c client driver is loaded or not
    if(grove_lcd_client == NULL) {
        printk("%s: i2c client device is not ready\n", __FUNCTION__);
        return -ENODEV;
    }
    return 0;
}

// Device release
static int grove_lcd_release(struct inode* inode, struct file* file)
{
    printk("%s: Freeing /dev resource\n", __FUNCTION__);
    mutex_unlock(&grove_lcd_mutex);
    return 0;
}

// Read funcion
static ssize_t grove_lcd_read(struct file *fp, char __user* buf, size_t count, loff_t* offset)
{
    // TODO
    return 0;
}

// Write function
static ssize_t grove_lcd_write(struct file* fp, const char* __user buff, size_t count, loff_t* offset)
{
    // TODO
    return 0;
}

// File operations
static const struct file_operations grove_lcd_fops  = {
    .owner   = THIS_MODULE,
    .llseek  = no_llseek,
    .write   = grove_lcd_write,
    .read    = grove_lcd_read,
    .open    = grove_lcd_open,
    .release = grove_lcd_release
};

// ******************************************
// i2c driver definition
// ******************************************
static int grove_lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int                      retval; // i2c status
    struct grove_lcd_device* data = NULL;

    // Debug
    printk("grove_lcd: %s\n", __FUNCTION__);

    // Allocate memory for device data
    data = devm_kzalloc(&client->dev, sizeof(struct grove_lcd_device), GFP_KERNEL);

    if(data == NULL){
        printk("%s: Failed to allocate memory for device data.", GROVE_LCD_NAME);
        return -ENOMEM;
    }

    // Check if SMBUS operation is functional or not
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE))
        printk("%s: i2c device doesn't have capability of SMBUS.", GROVE_LCD_NAME);
        return -EIO;

    // Initialize i2c client data
    i2c_set_clientdata(client, data);

    // Initialize the mutex
    mutex_init(&data->update_lock);

    // i2c hardware initialization
    grove_lcd_init(client);

    // Need to use i2c client
    grove_lcd_client  = client;

    // Register device
    grove_lcd_major   = register_chrdev(0, GROVE_LCD_NAME, &grove_lcd_fops);
    if(grove_lcd_major < 0){
        printk("%s: Failed to register character device.\n", GROVE_LCD_NAME);
        return 0;
    }

    // Create class
    grove_lcd_class   = class_create(THIS_MODULE, GROVE_LCD_NAME);
    if(IS_ERR(grove_lcd_class)){
        retval  = PTR_ERR(grove_lcd_data);
    }

    // Create device
    grove_lcd_data = device_create( grove_lcd_class,
                                    NULL,
                                    MKDEV(grove_lcd_major, 0),
                                    NULL,
                                    GROVE_LCD_NAME); 
    if(IS_ERR(grove_lcd_data)) {
        retval = PTR_ERR(grove_lcd_data);
        printk("%s: Failed to create device\n", __FUNCTION__);
        goto unreg_class;
    }

    // Initialize the mutex for /dev fops clients
    mutex_init(&grove_lcd_mutex);

    return 0;

// Cleanup for failed operation
unreg_class:
    class_unregister(grove_lcd_class);
    class_destroy(grove_lcd_class);
//unreg_chrdev:
    unregister_chrdev(grove_lcd_major, GROVE_LCD_NAME);
    printk("%s: Driver initialization failed\n", __FUNCTION__);
//out:
    return retval;
}

static int grove_lcd_remove(struct i2c_client * client)
{
    printk("grove_lcd: %s\n", __FUNCTION__);
 
    grove_lcd_client = NULL;
 
    // Destroy device
    device_destroy(grove_lcd_class, MKDEV(grove_lcd_major, 0));

    // Release class
    class_unregister(grove_lcd_class);
    class_destroy(grove_lcd_class);

    // Unregistering device
    unregister_chrdev(grove_lcd_major, GROVE_LCD_NAME);
 
    return 0;
}

// This function is used to detect this device.
// However, the i2c address is fixed, so actually there's nothing to detect
static int grove_lcd_detect(struct i2c_client* client, struct i2c_board_info* info)
{
    struct i2c_adapter* adapter = client->adapter;
    int                 address = client->addr;
    const char*         name    = NULL;
    
    printk("grove_lcd: %s!\n", __FUNCTION__);
    
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;
    
    // Since our address is hardwired to 0x21
    // we update the name of the driver. This must
    // match the name of the chip_driver struct below
    // in order for this driver to be loaded.
    if (address == GROVE_LCD_TEXT) {
        name = GROVE_LCD_NAME;
        dev_info(&adapter->dev, "grove_lcd_device found at 0x%02x\n", address);
    }
    
    else
        return -ENODEV;
    
    /* Upon successful detection, we coup the name of the
     * driver to the info struct.
     **/
    strlcpy(info->type, name, I2C_NAME_SIZE);
    return 0;
}

// i2c address list for this device
static const unsigned short grove_lcd_addr_list[] = 
{
    GROVE_LCD_TEXT,
    GROVE_LCD_BACK_LIGHT, 
    I2C_CLIENT_END
};

// Device ID table
static const struct i2c_device_id   grove_lcd_id_tbl[]  = 
{
    {GROVE_LCD_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, grove_lcd_id_tbl);

// i2c device driver
static struct i2c_driver grove_lcd_driver = {
    .driver = {
        .name       = GROVE_LCD_NAME,
    },
    .class          = I2C_CLASS_HWMON,
    .probe          = grove_lcd_probe,
    .remove         = grove_lcd_remove,
    .detect         = grove_lcd_detect,
    .id_table       = grove_lcd_id_tbl,
    .address_list   = grove_lcd_addr_list,
};

// i2c devices doesn't enumerate. Need to instantiate it.
static struct i2c_board_info grove_lcd_board_info[] __initdata = 
{
    {
        I2C_BOARD_INFO(GROVE_LCD_NAME, GROVE_LCD_TEXT),
    },
    {
        I2C_BOARD_INFO(GROVE_LCD_NAME, GROVE_LCD_BACK_LIGHT),
    },
};

static int __init grove_lcd_init_test(void)
{
    printk("%s: init.\n", GROVE_LCD_NAME);
 
    // Register i2c board info
    i2c_register_board_info(1, grove_lcd_board_info, ARRAY_SIZE(grove_lcd_board_info));
    return i2c_add_driver(&grove_lcd_driver);
}
module_init(grove_lcd_init_test);
 
static void __exit grove_lcd_cleanup(void)
{
    printk("%s: exit.\n", GROVE_LCD_NAME);
 
    return i2c_del_driver(&grove_lcd_driver);
}
module_exit(grove_lcd_cleanup);

// init and exit for this device
//module_i2c_driver(grove_lcd_driver);

MODULE_AUTHOR("Yuhei Horibe <yuhei1.horibe@gmail.com>");
MODULE_DESCRIPTION("Grove Pi color LCD driver.");
MODULE_LICENSE("GPL v2");

