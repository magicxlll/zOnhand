#include "driver/gpio.h"
#include "metalio_i2c1.h"

extern "C" {

bool metalio_i2c1_lock(int /*timeout_ms*/) { return true; }
bool metalio_i2c1_lock_for_flash(int /*timeout_ms*/) { return true; }
void metalio_i2c1_unlock(void) {}
esp_err_t metalio_i2c1_init(gpio_num_t /*sda*/, gpio_num_t /*scl*/) { return ESP_OK; }
esp_err_t metalio_i2c1_deinit(void) { return ESP_OK; }
esp_err_t metalio_i2c1_device_locked(i2c_port_t /*port*/, uint8_t /*addr*/,
                                     i2c_master_dev_handle_t* device) {
    if (device != nullptr) *device = nullptr;
    return ESP_OK;
}
esp_err_t metalio_i2c1_probe_locked(i2c_port_t /*port*/, uint8_t /*addr*/, int /*timeout_ms*/) {
    return ESP_OK;
}
esp_err_t metalio_i2c1_transmit_locked(i2c_port_t, uint8_t, const uint8_t*, size_t, int) {
    return ESP_OK;
}
esp_err_t metalio_i2c1_receive_locked(i2c_port_t, uint8_t, uint8_t*, size_t, int) {
    return ESP_OK;
}
esp_err_t metalio_i2c1_transmit_receive_locked(i2c_port_t, uint8_t, const uint8_t*, size_t, uint8_t*, size_t, int) {
    return ESP_OK;
}
esp_err_t metalio_i2c1_write_reg_locked(i2c_port_t, uint8_t, uint8_t, const uint8_t*, size_t, int) {
    return ESP_OK;
}
esp_err_t metalio_i2c1_read_reg_stop_locked(i2c_port_t, uint8_t, uint8_t, uint8_t*, size_t, int) {
    return ESP_OK;
}

} // extern "C"
