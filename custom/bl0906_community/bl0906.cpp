#include "bl0906.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bl0906_community {

static const char *const TAG = "bl0906_community";
uint8_t USR_WRPROT_Witable[6]={0xCA, 0x9E, 0x55, 0x55, 0x00, 0xB7};  //用户寄存器可操作指令
uint8_t USR_WRPROT_Onlyread[6]={0xCA, 0x9E, 0x00, 0x00, 0x00, 0x61};  //用户寄存器只读指令

static const uint8_t BL0906_READ_COMMAND = 0x35;  // 读操作命令
static const uint8_t BL0906_WRITE_COMMAND = 0xCA;  // 写操作命令

uint8_t bl0906_checksum(const uint8_t address, const DataPacket *data);

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
  
// 非阻塞状态机：每次 loop() 只做一小步（发命令、收 4 字节、推进下一条），
// 不再在 update() 里连续阻塞读 23 个寄存器。
void BL0906::loop() {
  if (this->read_state_ == ReadState::IDLE) {
    return;
  }

  if (this->read_state_ == ReadState::SEND) {
    if (this->read_index_ >= this->read_count_) {
      this->read_state_ = ReadState::IDLE;
      return;
    }
    // 帧间隔：上一条回复刚读完时不要立刻发下一条，BL0906 需要时间处理
    if ((int32_t) (millis() - this->next_send_at_) < 0) {
      return;
    }
    // 只排空“已经迟到”的残留字节；正常回复已在 WAIT 中读走，不应再有内容
    this->drain_rx_();
    const RegisterRead &reg = this->reads_[this->read_index_];
    // 清空滑动窗口，超时日志里的 last= 才是本次尝试实际收到的字节
    this->window_[0] = this->window_[1] = this->window_[2] = this->window_[3] = 0;
    this->window_len_ = 0;
    this->rx_bytes_ = 0;
    this->write_byte(BL0906_READ_COMMAND);
    this->write_byte(reg.address);
    this->read_started_ = millis();
    this->read_state_ = ReadState::WAIT;
    return;
  }

  // WAIT：逐字节滑动窗口。
  // RX 线上可能混入本轮 TX 的回声（0x35 + 地址）或悬空噪声，凑够 4 字节并不代表
  // 就是回复；只有校验和与当前寄存器匹配的 4 字节才接受，其余字节继续滑动丢弃。
  const RegisterRead &reg = this->reads_[this->read_index_];
  while (this->available() > 0) {
    this->window_[0] = this->window_[1];
    this->window_[1] = this->window_[2];
    this->window_[2] = this->window_[3];
    this->window_[3] = this->read();
    this->rx_bytes_++;
    if (this->window_len_ < 4) {
      this->window_len_++;
    }
    if (this->window_len_ < 4) {
      continue;
    }

    DataPacket data;
    data.l = this->window_[0];
    data.m = this->window_[1];
    data.h = this->window_[2];
    data.checksum = this->window_[3];
    data.address = reg.address;
    if (bl0906_checksum(reg.address, &data) == data.checksum) {
      this->publish_register_(reg, data);
      this->advance_read_(false);
      return;
    }
  }

  if (millis() - this->read_started_ >= READ_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Timeout reading register 0x%02X (available=%u, rx_bytes=%u, last=%02X %02X %02X %02X)",
             this->reads_[this->read_index_].address, this->available(), this->rx_bytes_, this->window_[0],
             this->window_[1], this->window_[2], this->window_[3]);
    this->drain_rx_();
    this->window_len_ = 0;
    this->advance_read_(true);
  }
}

// 有界排空 RX：悬空/噪声线会持续产生字节，无界的 while(available()) 会卡死并触发 WDT。
// 128 字节在 19200 波特率下约 66 ms，足够清掉残留回复，又不会长时间占用主循环。
void BL0906::drain_rx_() {
  for (size_t i = 0; i < 128 && this->available(); i++) {
    this->read();
  }
}

void BL0906::setup() {
  // 只把实际配置了传感器的寄存器加入读取列表，未配置的通道不再占用总线
  this->build_reads_();
  //此处可校正BL0906电参数。
  this->drain_rx_();
  this->write_array(USR_WRPROT_Witable, sizeof(USR_WRPROT_Witable));  //用户寄存器可操作指令
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
  this->write_array(USR_WRPROT_Onlyread, sizeof(USR_WRPROT_Onlyread));  //用户寄存器只读指令
  this->read_state_ = ReadState::IDLE;
}

void BL0906::build_reads_() {
  uint8_t n = 0;
  auto add = [&](uint8_t address, float reference, sensor::Sensor *sensor, ReadKind kind) {
    if (sensor == nullptr || n >= (sizeof(this->reads_) / sizeof(this->reads_[0]))) {
      return;
    }
    this->reads_[n++] = RegisterRead{address, reference, sensor, kind};
  };

  add(BL0906_FREQUENCY, this->frequency_reference_, this->frequency_sensor_, READ_FREQUENCY);
  add(BL0906_TEMPERATURE, 0.0f, this->temperature_sensor_, READ_TEMPERATURE);
  add(BL0906_V_RMS, this->voltage_reference_, this->voltage_sensor_, READ_U24);
  add(BL0906_I_1_RMS, this->current_reference_, this->current_sensor_1_, READ_U24);
  add(BL0906_I_2_RMS, this->current_reference_, this->current_sensor_2_, READ_U24);
  add(BL0906_I_3_RMS, this->current_reference_, this->current_sensor_3_, READ_U24);
  add(BL0906_I_4_RMS, this->current_reference_, this->current_sensor_4_, READ_U24);
  add(BL0906_I_5_RMS, this->current_reference_, this->current_sensor_5_, READ_U24);
  add(BL0906_I_6_RMS, this->current_reference_, this->current_sensor_6_, READ_U24);
  add(BL0906_WATT_1, this->power_reference_, this->power_sensor_1_, READ_S24);
  add(BL0906_WATT_2, this->power_reference_, this->power_sensor_2_, READ_S24);
  add(BL0906_WATT_3, this->power_reference_, this->power_sensor_3_, READ_S24);
  add(BL0906_WATT_4, this->power_reference_, this->power_sensor_4_, READ_S24);
  add(BL0906_WATT_5, this->power_reference_, this->power_sensor_5_, READ_S24);
  add(BL0906_WATT_6, this->power_reference_, this->power_sensor_6_, READ_S24);
  add(BL0906_WATT_SUM, this->power_sum_reference_, this->power_sensor_sum_, READ_S24);
  add(BL0906_CF_1_CNT, this->energy_reference_, this->energy_sensor_1_, READ_U24);
  add(BL0906_CF_2_CNT, this->energy_reference_, this->energy_sensor_2_, READ_U24);
  add(BL0906_CF_3_CNT, this->energy_reference_, this->energy_sensor_3_, READ_U24);
  add(BL0906_CF_4_CNT, this->energy_reference_, this->energy_sensor_4_, READ_U24);
  add(BL0906_CF_5_CNT, this->energy_reference_, this->energy_sensor_5_, READ_U24);
  add(BL0906_CF_6_CNT, this->energy_reference_, this->energy_sensor_6_, READ_U24);
  add(BL0906_CF_SUM_CNT, this->energy_sum_reference_, this->energy_sensor_sum_, READ_U24);

  this->read_count_ = n;
  this->read_index_ = 0;
  this->retries_ = 0;
  this->next_send_at_ = 0;
  this->window_len_ = 0;
  this->read_state_ = ReadState::IDLE;
}

void BL0906::update() {
  // 构建标记：每轮读取打印一次，便于确认 OTA 后实际运行的固件（尤其排查 OTA 回滚）
  ESP_LOGI(TAG, "build %s %s", __DATE__, __TIME__);
  // 只启动一轮读取，真正的收发在 loop() 里分步完成，避免阻塞主循环
  this->drain_rx_();
  if (this->read_count_ == 0) {
    return;
  }
  this->read_index_ = 0;
  this->retries_ = 0;
  this->next_send_at_ = 0;  // 新一轮可以立即发第一条
  this->window_len_ = 0;
  this->read_state_ = ReadState::SEND;
}

void BL0906::advance_read_(bool error) {
  // 失败时先重试同一条寄存器，避免一次偶发错位把整轮读取带偏
  if (error && this->retries_ < MAX_RETRIES) {
    this->retries_++;
    ESP_LOGW(TAG, "Retrying register 0x%02X (%u/%u)", this->reads_[this->read_index_].address,
             this->retries_, MAX_RETRIES);
    this->read_state_ = ReadState::SEND;
    this->next_send_at_ = millis() + RESYNC_GAP_MS;
    return;
  }

  this->retries_ = 0;
  this->read_index_++;
  if (this->read_index_ >= this->read_count_) {
    this->read_index_ = 0;
    this->read_state_ = ReadState::IDLE;
  } else {
    this->read_state_ = ReadState::SEND;
    this->next_send_at_ = millis() + FRAME_GAP_MS;
  }
}

  //校验和。SUM 字节为（Addr+Data_L+Data_M+Data_H）&0xFF 取反
uint8_t bl0906_checksum(const uint8_t address, const DataPacket *data) {
  return (address + data->l + data->m + data->h) ^ 0xFF; 
}
  
//解析并发布一条寄存器回复
bool BL0906::publish_register_(const RegisterRead &reg, const DataPacket &data) {
  if (bl0906_checksum(reg.address, &data) != data.checksum) {
    ESP_LOGW(TAG, "Checksum mismatch for register 0x%02X: %02X %02X %02X %02X (expected %02X)", reg.address, data.l,
             data.m, data.h, data.checksum, bl0906_checksum(reg.address, &data));
    return false;
  }

  ube24_t u24;
  u24.l = data.l;
  u24.m = data.m;
  u24.h = data.h;

  sbe24_t s24;
  s24.l = data.l;
  s24.m = data.m;
  s24.h = (int8_t) data.h;

  float value;
  switch (reg.kind) {
    case READ_S24:
      value = (float) to_int32_t(s24) * reg.reference;
      break;
    case READ_FREQUENCY: {
      uint32_t raw = to_uint32_t(u24);
      if (raw == 0) {
        ESP_LOGW(TAG, "Frequency register is zero");
        return true;  // 回复本身校验正确，只是数值不可用，不需要重试
      }
      value = reg.reference / (float) raw;
      break;
    }
    case READ_TEMPERATURE:
      value = ((float) to_int32_t(s24) - 64.0f) * 12.5f / 59.0f - 40.0f;
      break;
    case READ_U24:
    default:
      value = (float) to_uint32_t(u24) * reg.reference;
      break;
  }

  ESP_LOGD(TAG, "0x%02X = %.4f", reg.address, value);

  if (reg.sensor != nullptr) {
    reg.sensor->publish_state(value);
  }
  return true;
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
  ESP_LOGD(TAG, "RMSOS:%02X%02X%02X%02X%02X%02X",BL0906_WRITE_COMMAND,address,data.l ,data.m, data.h,data.address );
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
  //ESP_LOGD(TAG, "RMSOS:%02X%02X%02X%02X%02X%02X",BL0906_WRITE_COMMAND,address,data.l ,data.m, data.h,data.address );
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
