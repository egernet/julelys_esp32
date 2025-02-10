#include "oled_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

extern "C" {
#include "driver/i2c.h"
#include "esp_err.h"
}

#include "esp_log.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_sh1107.h"

#define LV_CONF_INCLUDE_SIMPLE 1

static const char *TAG = "Jylelys.OLED";

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           2
#define EXAMPLE_PIN_NUM_SCL           1
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3C
#define EXAMPLE_LCD_H_RES              132
#define EXAMPLE_LCD_V_RES              64

#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

static lv_style_t style_title;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t * disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

static lv_disp_t* initialise() 
{
    ESP_LOGI(TAG, "Initialize I2C bus");
    
    i2c_port_t i2c_host = I2C_NUM_0;
    
    i2c_config_t i2c_conf = {};
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = EXAMPLE_PIN_NUM_SDA;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_io_num = EXAMPLE_PIN_NUM_SCL;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ;

    ESP_ERROR_CHECK(i2c_param_config(i2c_host, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_host, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {};
    io_config.dev_addr = EXAMPLE_I2C_HW_ADDR;
    io_config.control_phase_bytes = 1;               // According to SSD1306 datasheet
    io_config.lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS;   // According to SSD1306 datasheet
    io_config.lcd_param_bits = EXAMPLE_LCD_CMD_BITS; // According to SSD1306 datasheet
    io_config.dc_bit_offset = 0;                     // According to SH1107 datasheet
    io_config.flags = {};
    io_config.flags.disable_control_phase = 1;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)i2c_host, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.bits_per_pixel = 1;
    panel_config.reset_gpio_num = EXAMPLE_PIN_NUM_RST;

    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };

    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);
    /* Register done callback for IO */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };

    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

    return disp;
}

LV_FONT_DECLARE(lv_font_montserrat_12);

static lv_obj_t* log_ui(lv_disp_t *disp) {   
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_12);

    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, "Run default sequence rainbow");
    
    lv_obj_add_style(label, &style_title, 0);
    
    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -18);

    return label;
}

static lv_obj_t* ip_ui(lv_disp_t *disp, const char* ip_address)
{
    char full_text[128];
    snprintf(full_text, sizeof(full_text), "IP: %s", ip_address);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); /* Circular scroll */
    lv_label_set_text(label, full_text);
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);

    return label;
}

static lv_obj_t* host_ip_ui(lv_disp_t *disp, const char* ip_address)
{
    char full_text[128];
    snprintf(full_text, sizeof(full_text), "IP: %s", ip_address);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); /* Circular scroll */
    lv_label_set_text(label, full_text);
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);

    return label;
}

lv_disp_t *disp;
lv_obj_t *log_label;
lv_obj_t *ip_label;
lv_obj_t *host_ip_label;

OLEDController::OLEDController() 
{
    disp = initialise();
    this->ip_address = "0.0.0.0";
    this->host_ip_address = "0.0.0.0";

    ip_label = ip_ui(disp, this->ip_address);
    host_ip_label = host_ip_ui(disp, this->host_ip_address);
    log_label = log_ui(disp);
}

void OLEDController::update() {
    char full_text[128];
    snprintf(full_text, sizeof(full_text), "IP: %s", this->ip_address);
    lv_label_set_text(ip_label, full_text);

    snprintf(full_text, sizeof(full_text), "Host IP: %s", this->host_ip_address);
    lv_label_set_text(host_ip_label, full_text);
}

void OLEDController::setLocalIPAddress(const char* ip_address) {
    this->ip_address = ip_address;
    update();
}

void OLEDController::setHostIPAddress(const char* ip_address) {
    this->host_ip_address = ip_address;
    update();
}