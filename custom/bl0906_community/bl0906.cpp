#include "bl0906.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bl0906_community {

static const char *const TAG = "bl0906_community";
uint8_t USR_WRPROT_Witable[6]={0xCA, 0x9E, 0x55, 0x55, 0x00, 0xB7};  //用户寄存器可操作指令
uint8_t USR_WRPROT_Onlyread[6]={0xCA, 0x9E, 0x00, 0x00, 0x00, 0x61};  //用户寄存器只读指令

static const uint8_t BL0906_READ_COMMAND = 0x35;  // 读操作命令
static const uint8_t BL0906_WRITE_COMMAND = 0xCA;  // 写操作命令

// BL0906寄存器地址
static const uint8_t BL0906_I_1_RMS = 0x0D;  //电流 1 通道有效值寄存器地址
static const uint8_t BL0906_I_2_RMS = 0x0E;  //电流 2 通道有效值寄存器地址
static const uint8_t BL0906_I_3_RMS = 0x0F;  //电流 3 通道有效值寄存器地址
static const uint8_t BL0906_I_4_RMS = 0x10;  //电流 4 通道有效值寄存器地址
static const uint8_t BL0906_I_5_RMS = 0x13;  //电流 5 通道有效值寄存器地址
static const uint8_t BL0906_I_6_RMS = 0x14;  //电流 6 通道有效值寄存器地址
static const uint8_t BL0906_V_RMS = 0x16;     //电压通道有效值寄存器
static const uint8_t BL0906_WATT_1 = 0X23;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_2 = 0X24;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_3 = 0X25;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_4 = 0X26;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_5 = 0X29;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_6 = 0X2A;    //通道 1 有功功率寄存器
static const uint8_t BL0906_WATT_SUM = 0X2C;       //总有功功率寄存器
static const uint8_t BL0906_CF_1_CNT = 0X30;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_2_CNT = 0X31;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_3_CNT = 0X32;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_4_CNT = 0X33;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_5_CNT = 0X36;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_6_CNT = 0X37;  //通道 1 有功脉冲计数，无符号
static const uint8_t BL0906_CF_SUM_CNT = 0X39;     //总有功脉冲计数，无符号
static const uint8_t BL0906_FREQUENCY = 0X4E;     //总有功脉冲计数，无符号
static const uint8_t BL0906_TEMPERATURE = 0X5E;     //总有功脉冲计数，无符号  
//校正寄存器
static const uint8_t BL0906_RMSGN_1 = 0x6D;  //有效值增益调整寄存器地址,通道1
static const uint8_t BL0906_RMSGN_2 = 0x6E;  //有效值增益调整寄存器地址,通道2
static const uint8_t BL0906_RMSGN_3 = 0x6F;  //有效值增益调整寄存器地址,通道3
static const uint8_t BL0906_RMSGN_4 = 0x70;  //有效值增益调整寄存器地址,通道4
static const uint8_t BL0906_RMSGN_5 = 0x73;  //有效值增益调整寄存器地址,通道5
static const uint8_t BL0906_RMSGN_6 = 0x74;  //有效值增益调整寄存器地址,通道6
static const uint8_t BL0906_RMSOS_1 = 0x78;  //有效值偏置校正寄存器地址,通道1
static const uint8_t BL0906_RMSOS_2 = 0x79;  //有效值偏置校正寄存器地址,通道2
static const uint8_t BL0906_RMSOS_3 = 0x7A;  //有效值偏置校正寄存器地址,通道3
static const uint8_t BL0906_RMSOS_4 = 0x7B;  //有效值偏置校正寄存器地址,通道4
static const uint8_t BL0906_RMSOS_5 = 0x7E;  //有效值偏置校正寄存器地址,通道5
static const uint8_t BL0906_RMSOS_6 = 0x7F;  //有效值偏置校正寄存器地址,通道6  
static const uint8_t BL0906_WATTGN_1 = 0xB7; //有功功率增益调整寄存器,通道1
static const uint8_t BL0906_WATTGN_2 = 0xB8; //有功功率增益调整寄存器,通道2
static const uint8_t BL0906_WATTGN_3 = 0xB9; //有功功率增益调整寄存器,通道3
static const uint8_t BL0906_WATTGN_4 = 0xBA; //有功功率增益调整寄存器,通道4
static const uint8_t BL0906_WATTGN_5 = 0xBD; //有功功率增益调整寄存器,通道5
static const uint8_t BL0906_WATTGN_6 = 0xBE; //有功功率增益调整寄存器,通道6  
  
void BL0906::loop() {
//
}

// 有界排空 RX：悬空/噪声线会持续产生字节，无界的 while(available()) 会卡死并触发 WDT。
// 128 字节在 19200 波特率下约 66 ms，足够清掉残留回复，又不会长时间占用主循环。
void BL0906::drain_rx_() {
  for (size_t i = 0; i < 128 && this->available(); i++) {
    this->read();
  }
}

void BL0906::setup() {
//此处可校正BL0906电参数。
this->drain_rx_();
this-> write_array(USR_WRPROT_Witable,sizeof(USR_WRPROT_Witable));//用户寄存器可操作指令
  //电参数校正（参数1：寄存器地址；参数2：校正前值；参数3：校正后值、电参数值与寄存器值的转换系数)
//Bias_correction(BL0906_RMSOS_1, 0.01960, 0);//电流有效值偏置校正，通道1
//Bias_correction(BL0906_RMSOS_2, 0.02673, 0);//电流有效值偏置校正，通道2
//Bias_correction(BL0906_RMSOS_3, 0.02895, 0);//电流有效值偏置校正，通道3
//Bias_correction(BL0906_RMSOS_4, 0.03604, 0);//电流有效值偏置校正，通道4
//Bias_correction(BL0906_RMSOS_5, 0.03056, 0);//电流有效值偏置校正，通道5
//Bias_correction(BL0906_RMSOS_6, 0.02949, 0);//电流有效值偏置校正，通道6
//gain_correction(BL0906_RMSGN_1, 0.07169, 0.068, BL0906_ki);//电流有效值增益调整，通道1
//gain_correction(BL0906_RMSGN_2, 0.07232, 0.068, BL0906_ki);//电流有效值增益调整，通道2
//gain_correction(BL0906_RMSGN_3, 0.07333, 0.068, BL0906_ki);//电流有效值增益调整，通道3
//gain_correction(BL0906_RMSGN_4, 0.07779, 0.068, BL0906_ki);//电流有效值增益调整，通道4
//gain_correction(BL0906_RMSGN_5, 0.07224, 0.068, BL0906_ki);//电流有效值增益调整，通道5
//gain_correction(BL0906_RMSGN_6, 0.14988, 0.068, BL0906_ki);//电流有效值增益调整，通道6
//gain_correction(BL0906_WATTGN_1, 15.13427, 14.5, BL0906_Kp);//功率增益调整，通道1
//gain_correction(BL0906_WATTGN_2, 15.23937, 14.5, BL0906_Kp);//功率增益调整，通道2
//gain_correction(BL0906_WATTGN_3, 15.44956, 14.5, BL0906_Kp);//功率增益调整，通道3
//gain_correction(BL0906_WATTGN_4, 16.57646, 14.5, BL0906_Kp);//功率增益调整，通道4
//gain_correction(BL0906_WATTGN_5, 15.27440, 14.5, BL0906_Kp);//功率增益调整，通道5
//gain_correction(BL0906_WATTGN_6, 31.75744, 14.5, BL0906_Kp);//功率增益调整，通道6
this-> write_array(USR_WRPROT_Onlyread,sizeof(USR_WRPROT_Onlyread));//用户寄存器只读指令
}


void BL0906::update() {
  this->drain_rx_();
  read_data(BL0906_FREQUENCY, frequency_reference_, frequency_sensor_ ); //频率
  read_data(BL0906_TEMPERATURE, temperature_reference_, temperature_sensor_); //温度
  read_data(BL0906_V_RMS, voltage_reference_, voltage_sensor_); //电压
  read_data(BL0906_I_1_RMS, current_reference_, current_sensor_1_ ); //电流,通道1
  read_data(BL0906_I_2_RMS, current_reference_, current_sensor_2_ ); //电流,通道2
  read_data(BL0906_I_3_RMS, current_reference_, current_sensor_3_ ); //电流,通道3
  read_data(BL0906_I_4_RMS, current_reference_, current_sensor_4_ ); //电流,通道4
  read_data(BL0906_I_5_RMS, current_reference_, current_sensor_5_ ); //电流,通道5
  read_data(BL0906_I_6_RMS, current_reference_, current_sensor_6_ ); //电流,通道6
  read_data(BL0906_WATT_1, power_reference_, power_sensor_1_ );//功率。通道1 
  read_data(BL0906_WATT_2, power_reference_, power_sensor_2_ );//功率。通道2 
  read_data(BL0906_WATT_3, power_reference_, power_sensor_3_ );//功率。通道3 
  read_data(BL0906_WATT_4, power_reference_, power_sensor_4_ );//功率。通道4 
  read_data(BL0906_WATT_5, power_reference_, power_sensor_5_ );//功率。通道5 
  read_data(BL0906_WATT_6, power_reference_, power_sensor_6_ );//功率。通道6 
  read_data(BL0906_WATT_SUM, power_sum_reference_, power_sensor_sum_ );//功率。通道总和
  read_data(BL0906_CF_1_CNT, energy_reference_, energy_sensor_1_ ); //电量。通道1
  read_data(BL0906_CF_2_CNT, energy_reference_, energy_sensor_2_ ); //电量。通道2
  read_data(BL0906_CF_3_CNT, energy_reference_, energy_sensor_3_ ); //电量。通道3
  read_data(BL0906_CF_4_CNT, energy_reference_, energy_sensor_4_ ); //电量。通道4
  read_data(BL0906_CF_5_CNT, energy_reference_, energy_sensor_5_ ); //电量。通道5
  read_data(BL0906_CF_6_CNT, energy_reference_, energy_sensor_6_ ); //电量。通道6
  read_data(BL0906_CF_SUM_CNT, energy_sum_reference_, energy_sensor_sum_ ); //电量。通道总和
}

  //校验和。SUM 字节为（Addr+Data_L+Data_M+Data_H）&0xFF 取反
uint8_t bl0906_checksum(const uint8_t address, const DataPacket *data) {
  return (address + data->l + data->m + data->h) ^ 0xFF; 
}
  
//读取数据
void BL0906::read_data(const uint8_t address, const float reference, sensor::Sensor *sensor_) {
  DataPacket buffer;
  ube24_t data_u24;
  sbe24_t data_s24;
  float value=0;
  //读取功率（单通道与总功率）
  if (reference == power_reference_ || reference == power_sum_reference_){
    this-> write_byte(BL0906_READ_COMMAND);
    this-> write_byte(address);
    if (this-> read_array((uint8_t *) &buffer, sizeof(buffer)-1))  {      
      if (bl0906_checksum(address,&buffer)==buffer.checksum) {
        data_s24.l = buffer.l;
        data_s24.m = buffer.m;
        data_s24.h = buffer.h;
        value = (float) to_int32_t(data_s24) * reference;
        if (sensor_ != nullptr) {
          sensor_->publish_state(value);
        }      
      }
    } else {
      ESP_LOGW(TAG, "Junk on wire. Throwing away partial power message");
      this->drain_rx_();
    } 
  }
  //读取电压、电流、单通道电量、总电量
  if (reference == voltage_reference_ || reference == current_reference_ || reference == energy_reference_ || reference == energy_sum_reference_) {
    this-> write_byte(BL0906_READ_COMMAND);
    this-> write_byte(address);
    if (this-> read_array((uint8_t *) &buffer, sizeof(buffer)-1))  {    
    //ESP_LOGW(TAG, "checksum= 0x%02X", bl0906_checksum(BL0906_V_RMS,&buffer));      
      if (bl0906_checksum(address,&buffer)==buffer.checksum) {
        data_u24.l = buffer.l;
        data_u24.m = buffer.m;
        data_u24.h = buffer.h;
        value = (float) to_uint32_t(data_u24) * reference;
        if (sensor_ != nullptr) {
          sensor_->publish_state(value);
        }      
      }
    } else {
      ESP_LOGW(TAG, "Junk on wire. Throwing away partial message");
      this->drain_rx_();
    }     
  }  
    //读取频率
  if (reference == frequency_reference_) {
    this-> write_byte(BL0906_READ_COMMAND);
    this-> write_byte(address);
    if (this-> read_array((uint8_t *) &buffer, sizeof(buffer)-1))  {    
      if (bl0906_checksum(address,&buffer)==buffer.checksum) {
        data_u24.l = buffer.l;
        data_u24.m = buffer.m;
        data_u24.h = buffer.h;
        value = reference / (float) to_uint32_t(data_u24);
        if (sensor_ != nullptr) {
          sensor_->publish_state(value);
        }      
      }
    } else {
      ESP_LOGW(TAG, "Junk on wire. Throwing frequency message");
      this->drain_rx_();
    }     
  }
    //读取芯片温度
  if (reference == temperature_reference_) {
    this-> write_byte(BL0906_READ_COMMAND);
    this-> write_byte(address);
    if (this-> read_array((uint8_t *) &buffer, sizeof(buffer)-1))  {    
      if (bl0906_checksum(address,&buffer)==buffer.checksum) {
        data_s24.l = buffer.l;
        data_s24.m = buffer.m;
        data_s24.h = buffer.h;
        value = (float) to_int32_t(data_s24);
        value = (value - 64) *12.5/59-40;  
        if (sensor_ != nullptr) {
          sensor_->publish_state(value);
        }      
      }
    } else {
      ESP_LOGW(TAG, "Junk on wire. Throwing frequency message");
      this->drain_rx_();
    }     
  }  
}  
//有效值偏置校正
void BL0906::Bias_correction(const uint8_t address, const float measurements, const float Correction) {
  DataPacket data;  
  float ki = 12875*1*(5.1+5.1)*1000/2000/1.097; //电流系数
  float I_RMS0 = measurements * ki;
  float I_RMS = Correction * ki;
  int32_t value = (I_RMS*I_RMS-I_RMS0*I_RMS0) / 256; 
  data.l = value << 24 >>24;
  data.m = value << 16 >>24;
  if (value < 0) {
    data.h = (value << 8 >>24)|0b10000000;    
  }  
  data.address = bl0906_checksum(address, &data);
  ESP_LOGW(TAG, "RMSOS:%02X%02X%02X%02X%02X%02X",BL0906_WRITE_COMMAND,address,data.l ,data.m, data.h,data.address );
  this-> write_byte(BL0906_WRITE_COMMAND);
  this-> write_byte(address);
  this-> write_byte(data.l);
  this-> write_byte(data.m);
  this-> write_byte(data.h);
  this-> write_byte(data.address);
}  
//增益调整
void BL0906::gain_correction(const uint8_t address, const float measurements, const float Correction, const float coefficient) {
  DataPacket data;  
  float I_RMS0 = measurements * coefficient;
  float I_RMS = Correction * coefficient;
  float rms_gn = int((I_RMS/I_RMS0-1) * 65536);
  int16_t value;
  if (rms_gn <= -32767){
    value = -32767;
  } else {
    value = int(rms_gn);  
  }  
  data.h = 0xFF;
  data.m = value >> 8;
  data.l = value << 8 >>8;  
  data.address = bl0906_checksum(address, &data);
  //ESP_LOGW(TAG, "RMSOS:%02X%02X%02X%02X%02X%02X",BL0906_WRITE_COMMAND,address,data.l ,data.m, data.h,data.address );
  this-> write_byte(BL0906_WRITE_COMMAND);
  this-> write_byte(address);
  this-> write_byte(data.l);
  this-> write_byte(data.m);
  this-> write_byte(data.h);
  this-> write_byte(data.address);
}    

  //void BL0906::dump_config() {  // NOLINT(readability-function-cognitive-complexity)
  //ESP_LOGCONFIG(TAG, "BL0906:");
  //LOG_SENSOR("", "Voltage", this->voltage_sensor_);
//}

uint32_t BL0906::to_uint32_t(ube24_t input) { return input.h << 16 | input.m << 8 | input.l; }

int32_t BL0906::to_int32_t(sbe24_t input) { return input.h << 16 | input.m << 8 | input.l; }

}  // namespace bl0906_community
}  // namespace esphome
