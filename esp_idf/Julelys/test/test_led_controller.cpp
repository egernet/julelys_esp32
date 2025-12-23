#include <iostream>
#include <cassert>
#include <cstring>

// Include mocks before the actual implementation
#include "mocks/esp_types.h"
#include "mocks/freertos_mock.h"
#include "mocks/led_strip_mock.h"
#include "mocks/gpio_mock.h"
#include "mocks/esp_log_mock.h"

// Global mock state definitions
LedStripMockState g_led_strip_mock = {};
GpioMockState g_gpio_mock = {};

// Include the actual LED controller implementation
#include "../components/led_controller/led_controller.cpp"

// Test utilities
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    memset(&g_led_strip_mock, 0, sizeof(g_led_strip_mock)); \
    memset(&g_gpio_mock, 0, sizeof(g_gpio_mock)); \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        tests_failed++; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        throw std::runtime_error("Expected " #expected " == " #actual); \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("Expected " #cond " to be true"); \
    } \
} while(0)

// ============ Test Cases ============

TEST(buffer_initialization) {
    LedController controller(7, 8, 55);

    // Verify dimensions
    ASSERT_EQ(8, controller.matrixWidth);
    ASSERT_EQ(55, controller.matrixHeight);

    // imageHaveChange should be set after init
    ASSERT_TRUE(controller.imageHaveChange);
}

TEST(set_pixel_basic) {
    LedController controller(7, 8, 55);

    // Set a pixel in the back buffer
    RgbwColor color(100, 150, 200, 50);
    controller.setPixel(0, 0, color);

    // Back buffer should have the new color
    // We can verify by swapping and checking the mock
    controller.swapBuffers();

    // Now refresh should send our pixel to the mock
    // Reset mock counters
    g_led_strip_mock.pixels_set = 0;
    controller.refresh();

    // Check that pixels were set
    ASSERT_TRUE(g_led_strip_mock.pixels_set > 0);
}

TEST(set_pixel_rgba_overload) {
    LedController controller(7, 8, 55);

    controller.setPixel(1, 2, 10, 20, 30, 40);
    controller.swapBuffers();

    // Verify via refresh
    controller.refresh();
    ASSERT_TRUE(g_led_strip_mock.pixels_set > 0);
}

TEST(swap_buffers_sets_flag) {
    LedController controller(7, 8, 55);

    // Clear the flag
    controller.imageHaveChange = false;

    // Swap should set the flag
    controller.swapBuffers();

    ASSERT_TRUE(controller.imageHaveChange);
}

TEST(swap_buffers_actually_swaps) {
    LedController controller(7, 8, 55);

    // Set pixel in back buffer
    RgbwColor red(255, 0, 0, 0);
    RgbwColor green(0, 255, 0, 0);

    controller.setPixel(0, 0, red);
    controller.swapBuffers();

    // Now back buffer is clean, set different color
    controller.setPixel(0, 0, green);

    // Refresh reads from front (which should have red)
    g_led_strip_mock.pixels_set = 0;
    controller.refresh();

    // First pixel should be red (255, 0, 0, 0)
    ASSERT_EQ(255, g_led_strip_mock.pixels[0][0]);
    ASSERT_EQ(0, g_led_strip_mock.pixels[0][1]);
}

TEST(clean_sets_all_red) {
    LedController controller(7, 8, 55);

    // Set some pixels to non-red colors
    controller.setPixel(0, 0, RgbwColor(0, 255, 0, 0));
    controller.setPixel(1, 1, RgbwColor(0, 0, 255, 0));
    controller.swapBuffers();

    // Now clean
    controller.clean();

    // Refresh and check first pixel is red
    g_led_strip_mock.pixels_set = 0;
    controller.refresh();

    ASSERT_EQ(255, g_led_strip_mock.pixels[0][0]); // red
    ASSERT_EQ(0, g_led_strip_mock.pixels[0][1]);   // green
    ASSERT_EQ(0, g_led_strip_mock.pixels[0][2]);   // blue
}

TEST(matrix_bounds_corner_cases) {
    LedController controller(7, 8, 55);

    // Test corners
    controller.setPixel(0, 0, RgbwColor(1, 2, 3, 4));      // Top-left
    controller.setPixel(0, 54, RgbwColor(5, 6, 7, 8));     // Top-right
    controller.setPixel(7, 0, RgbwColor(9, 10, 11, 12));   // Bottom-left
    controller.setPixel(7, 54, RgbwColor(13, 14, 15, 16)); // Bottom-right

    controller.swapBuffers();

    // Verify no crash and data is accessible
    controller.refresh();
    ASSERT_TRUE(g_led_strip_mock.pixels_set > 0);
}

TEST(refresh_uses_gpio_multiplexer) {
    LedController controller(7, 8, 55);

    controller.refresh();

    // GPIO should have been set for channel selection
    // After full refresh, last channel is 7 (binary: 111)
    // GPIO4 = bit 0, GPIO5 = bit 1, GPIO8 = bit 2
    ASSERT_EQ(1, g_gpio_mock.levels[4]); // bit 0 of 7
    ASSERT_EQ(1, g_gpio_mock.levels[5]); // bit 1 of 7
    ASSERT_EQ(1, g_gpio_mock.levels[8]); // bit 2 of 7
}

TEST(refresh_calls_led_strip_refresh_per_row) {
    LedController controller(7, 8, 55);

    g_led_strip_mock.refreshes = 0;
    controller.refresh();

    // Should refresh 8 times (once per row)
    ASSERT_EQ(8, g_led_strip_mock.refreshes);
}

TEST(rgbw_color_constructors) {
    RgbwColor c1;
    ASSERT_EQ(0u, c1.red);
    ASSERT_EQ(0u, c1.green);
    ASSERT_EQ(0u, c1.blue);
    ASSERT_EQ(0u, c1.white);

    RgbwColor c2(100, 150, 200, 250);
    ASSERT_EQ(100u, c2.red);
    ASSERT_EQ(150u, c2.green);
    ASSERT_EQ(200u, c2.blue);
    ASSERT_EQ(250u, c2.white);

    RgbwColor c3(128); // brightness constructor
    ASSERT_EQ(128u, c3.red);
    ASSERT_EQ(128u, c3.green);
    ASSERT_EQ(128u, c3.blue);
    ASSERT_EQ(0u, c3.white);
}

// ============ Main ============

int main() {
    std::cout << "\n=== LED Controller Unit Tests ===" << std::endl;
    std::cout << std::endl;

    RUN_TEST(buffer_initialization);
    RUN_TEST(set_pixel_basic);
    RUN_TEST(set_pixel_rgba_overload);
    RUN_TEST(swap_buffers_sets_flag);
    RUN_TEST(swap_buffers_actually_swaps);
    RUN_TEST(clean_sets_all_red);
    RUN_TEST(matrix_bounds_corner_cases);
    RUN_TEST(refresh_uses_gpio_multiplexer);
    RUN_TEST(refresh_calls_led_strip_refresh_per_row);
    RUN_TEST(rgbw_color_constructors);

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    std::cout << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
