#include "dfrobot_max31855.h"

DFRobot_MAX31855::DFRobot_MAX31855(i2c_inst_t *i2c_port, uint8_t I2C_addr)
{
  _i2c_port = i2c_port;
  _I2C_addr = I2C_addr;
}

void DFRobot_MAX31855::begin(void)
{
  i2c_init(_i2c_port, 100000);
  gpio_set_function(2, GPIO_FUNC_I2C);
  gpio_set_function(3, GPIO_FUNC_I2C);
  gpio_pull_up(2);
  gpio_pull_up(3);
  // Enable I2C hardware
  i2c_set_slave_mode(_i2c_port, false, _I2C_addr);
}

float DFRobot_MAX31855::readCelsius(void)
{
  uint8_t rxbuf[4] = {0};
  readData(0x00, rxbuf, 4);
  if(rxbuf[3] & 0x7){
  }
  if(rxbuf[0] & 0x80){
    rxbuf[0] = 0xff - rxbuf[0];
    rxbuf[1] = 0xff - rxbuf[1];
    float temp = -((((rxbuf[0] << 8)|(rxbuf[1] & 0xfc)) >> 2) + 1) * 0.25;
    return temp;
  }
  float temp = (((rxbuf[0] << 8 )| (rxbuf[1] & 0xfc)) >> 2) * 0.25;
  return temp;
}

int16_t DFRobot_MAX31855::readData(uint8_t Reg, uint8_t *Data, uint8_t len)
{
  i2c_write_blocking(_i2c_port, _I2C_addr, &Reg, 1, true);
  i2c_read_blocking(_i2c_port, _I2C_addr, Data, len, false);
  return len;
}
