#include "pico/stdlib.h"
#include <string>
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/textRenderer/TextRenderer.h"
#include "tempsensor/dfrobot_max31855.h"
DFRobot_MAX31855 max31855(i2c1, 0x10);
const uint32_t DEBOUNCE_TIME = 100;
unsigned long time = 0;
using namespace pico_ssd1306;
using namespace std;
bool btn_up = false;
bool btn_down = false;
bool btn_select = false;
bool btn_enter = false;
uint32_t seconds = 0;

float temp;

void up()// interrupt functions to handle buttons
{

    btn_up = true;
}
void down()
{

    btn_down = true;
}
void select()
{

    btn_select = true;
}
void enter()
{

    btn_enter = true;
}

void gpio_callback(uint gpio, uint32_t events)
{
    if ((to_ms_since_boot(get_absolute_time()) - time) > DEBOUNCE_TIME)
    {
        time = (to_ms_since_boot(get_absolute_time()));
        if (gpio == 16)
            up();
        if (gpio == 17)
            down();
        if (gpio == 18)
            select();
        if (gpio == 19)
            enter();
    }
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    gpio_put(4, !gpio_get(4));
    seconds++;
    return true;
}

void start_furnace(bool *furnace_started)
{
    *furnace_started = true;
    gpio_put(5, 0);
    gpio_put(8, 1);
    seconds = 0;
}

void check_temp(uint16_t selected_temp)
{

    if (temp < selected_temp + 10 && temp > selected_temp - 10)
    {
        gpio_put(6, 1); // if within range turn on corresponding LED
    }
    else
    {
        gpio_put(6, 0);
    }
}

void stop_furnace(bool *furnace_started)// stop process
{
    gpio_put(5, 1); 
    gpio_put(8, 0);
    gpio_put(6, 0);
    seconds = 0;
    *furnace_started = false;
}

int main()
{
    int8_t select_mode = 1;
    max31855.begin();
    bool furnace_running = false;
    string modeString;
    const char *modeStringPtr;

    string operationTemperature;
    const char *operationTemperaturePtr;

    uint16_t menu_selected_temp = 200;
    uint16_t selected_temp;
    uint32_t selected_seconds;
    uint32_t remaining_seconds;
    uint32_t menu_selected_seconds = 600;

    stdio_init_all();
    i2c_init(i2c0, 1000000); // Use i2c port with baud rate of 1Mhz
    // Set pins for I2C operation
    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);
    gpio_pull_up(0);
    gpio_pull_up(1);

    struct repeating_timer timer;
    //struct repeating_timer displaytimer;

    gpio_init(4);
    gpio_set_dir(4, GPIO_OUT);
    gpio_init(5);
    gpio_set_dir(5, GPIO_OUT);
    gpio_init(6);
    gpio_set_dir(6, GPIO_OUT);
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);
    SSD1306 display = SSD1306(i2c0, 0x3C, Size::W128xH64);//set the oled display to a resolution of 128x64 pixels

    gpio_set_irq_enabled_with_callback(16, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);// enable interrupts on pins
    gpio_set_irq_enabled(17, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(18, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(19, GPIO_IRQ_EDGE_RISE, true);
    display.setOrientation(0);
    while (1)
    {
        temp = max31855.readCelsius();// reads the temperature for the sensor in celcious
        if (select_mode == 1)
        {
            modeString = "Sel Temp: " + to_string(menu_selected_temp);
        }
        else if (select_mode == -1)
        {
            modeString = "Sel Sec: " + to_string(menu_selected_seconds);
        }
        string tempString = "Cur Temp: " + to_string(temp); 
        const char *modeStringPtr = modeString.c_str();
        const char *tempStringPtr = tempString.c_str();
        if (btn_up) // increase target temp if selected mode is 1 else increase seconds
        {
            if (select_mode == 1)
            {
                menu_selected_temp += 10;
            }
            else
            {
                menu_selected_seconds += 30;
            }
            btn_up = false;
        }
        if (btn_down)
        {
            if (select_mode == 1)
            {
                if (menu_selected_temp -10 <= 0 )
                {
                    menu_selected_temp = 0;
                }
                else
                {
                    menu_selected_temp -= 10;
                }
            }
            else
            {
                if (menu_selected_seconds -30 <= 0 )
                {
                    menu_selected_seconds = 0;
                }
                else
                {
                    menu_selected_seconds -= 30;
                }
            }
            btn_down = false;
        }
        if (btn_select)
        {
            select_mode *= -1;
            display.clear();
            display.sendBuffer();
            btn_select = false;
        }
        if (btn_enter)
        {
            if (furnace_running)
            {
                cancel_repeating_timer(&timer); // cancel previous timer if furnace is running
                stop_furnace(&furnace_running);
            }
            add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer); // add 1s timer to count seconds
            start_furnace(&furnace_running); // starts the process
            selected_temp = menu_selected_temp;
            selected_seconds = menu_selected_seconds;
            btn_enter = false;
        }

        display.clear();
        if (furnace_running)
        {
            check_temp(temp);
            remaining_seconds = selected_seconds - seconds;
            if (remaining_seconds <= 0) // end timer and process if the time has elapsed
            {
                cancel_repeating_timer(&timer);
                stop_furnace(&furnace_running);
            }
            else
            {
                string remaining_sec = "Rem sec: " + to_string(remaining_seconds);
                const char *remainingPtr = remaining_sec.c_str();
                string operationTemperature = "Op Temp: " + to_string(selected_temp); 
                operationTemperaturePtr = operationTemperature.c_str();
                drawText(&display, font_8x8, modeStringPtr, 0, 0);
                drawText(&display, font_8x8, tempStringPtr, 0, 10);
                drawText(&display, font_8x8, remainingPtr, 0, 20);
                drawText(&display, font_8x8, operationTemperaturePtr, 0 , 30);
                display.sendBuffer();
            }
        }
        else
        {
            drawText(&display, font_8x8, modeStringPtr, 0, 0);
            drawText(&display, font_8x8, tempStringPtr, 0, 10);
            display.sendBuffer(); // write to display
        }
    }
}