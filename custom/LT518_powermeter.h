using namespace esphome::uart;

#include "esphome.h"

class LT518PowerMeter : public PollingComponent, public UARTDevice, public Sensor
{
    public:
        LT518PowerMeter(UARTComponent *parent) : PollingComponent(30000), UARTDevice(parent) {}

        float value = 0;

        uint8_t Read_ID = 0x01;
        uint8_t Tx_Buffer[8];
        uint8_t Rx_Buffer[40];
        uint16_t Rx_Counter = 0;

        uint8_t temp_byte = 0;
        uint8_t *temp_byte_pointer = &temp_byte;

        unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
        {
            unsigned char i;
            unsigned char chk;
            crc = crc ^ crcbuf;
            for (i = 0; i < 8; i++)
            {
                chk = (unsigned char)(crc & 1);
                crc = crc >> 1;
                crc = crc & 0x7fff;
                if (chk == 1)
                    crc = crc ^ 0xa001;
                crc = crc & 0xffff;
            }
            return crc;
        }

        unsigned int chkcrc(unsigned char *buf, unsigned char len)
        {
            unsigned char hi, lo;
            unsigned int i;
            unsigned int crc;
            crc = 0xFFFF;
            for (i = 0; i < len; i++)
            {
                crc = this->calccrc(*buf, crc);
                buf++;
            }
            hi = (unsigned char)(crc % 256);
            lo = (unsigned char)(crc / 256);
            crc = (((unsigned int)(hi)) << 8) | lo;
            return crc;
        }

        void setup() override
        {
            // nothing to do here
        }
        
        void loop() override
        {
            // Use Arduino API to read data, for example
            bool have_message = this->read_message();
        }

        void update() override
        {
            publish_state(value);
            this->send_query_command();
        }

        void send_query_command()
        {
            union crcdata {
                uint16_t word16;
                uint8_t byte[2];
            } crcnow;

            this->Tx_Buffer[0] = this->Read_ID; //135模块的ID号，默认ID为0x01
            this->Tx_Buffer[1] = 0x03;
            this->Tx_Buffer[2] = 0x00;
            this->Tx_Buffer[3] = 0x48;
            this->Tx_Buffer[4] = 0x00;
            this->Tx_Buffer[5] = 0x08;
            crcnow.word16 = this->chkcrc(this->Tx_Buffer, 6);
            this->Tx_Buffer[6] = crcnow.byte[1]; //CRC效验低字节在前
            this->Tx_Buffer[7] = crcnow.byte[0];

            write_array(this->Tx_Buffer, 8);
            ESP_LOGD("custom", ">>> write query command");
        }

        bool read_message()
        {
            union crcdata {
                uint16_t word16;
                uint8_t byte[2];
            } crcnow;

            while (available() >= 1)
            {
                read_byte(this->temp_byte_pointer);
                //   ESP_LOGD("custom", ">>> %#010x", temp_byte);
                this->Rx_Buffer[this->Rx_Counter] = temp_byte;
                this->Rx_Counter++;
            }

            if (this->Rx_Counter >= 37)
            {
                ESP_LOGD("custom", ">>> RX_Counter: %d", this->Rx_Counter);

                if (this->Rx_Buffer[0] == this->Read_ID) //确认ID正确
                {

                    crcnow.word16 = this->chkcrc(Rx_Buffer, this->Rx_Counter - 2);
                    ESP_LOGD("custom", ">>> crcnow.word16: %d", crcnow.word16);

                    if (crcnow.byte[0] == this->Rx_Buffer[this->Rx_Counter - 1] &&
                        crcnow.byte[1] == this->Rx_Buffer[this->Rx_Counter - 2])   //确认CRC校验正确
                    {

                        ESP_LOGD("custom", ">>> crc ok");

                        // unsigned long voltage_data = (((unsigned long)(Rx_Buffer[3])) << 24) |
                        //                             (((unsigned long)(Rx_Buffer[4])) << 16) |
                        //                             (((unsigned long)(Rx_Buffer[5])) << 8) |
                        //                             Rx_Buffer[6];

                        // ESP_LOGD("custom", ">>> voltage_data: %lu", voltage_data);

                        // unsigned long current_data = (((unsigned long)(Rx_Buffer[7])) << 24) |
                        //                             (((unsigned long)(Rx_Buffer[8])) << 16) |
                        //                             (((unsigned long)(Rx_Buffer[9])) << 8) |
                        //                             Rx_Buffer[10];

                        // ESP_LOGD("custom", ">>> current_data: %lu", current_data);

                        unsigned long power_data = (((unsigned long)(this->Rx_Buffer[11])) << 24) |
                                                (((unsigned long)(this->Rx_Buffer[12])) << 16) |
                                                (((unsigned long)(this->Rx_Buffer[13])) << 8) |
                                                this->Rx_Buffer[14];

                        ESP_LOGD("custom", ">>> power_data: %lu", power_data);

                        // unsigned long energy_data = (((unsigned long)(this->Rx_Buffer[15])) << 24) |
                        //                             (((unsigned long)(this->Rx_Buffer[16])) << 16) |
                        //                             (((unsigned long)(this->Rx_Buffer[17])) << 8) |
                        //                             this->Rx_Buffer[18];

                        // ESP_LOGD("custom", ">>> energy_data: %lu", energy_data);

                        // unsigned long pf_data = (((unsigned long)(this->Rx_Buffer[19])) << 24) |
                        //                         (((unsigned long)(this->Rx_Buffer[20])) << 16) |
                        //                         (((unsigned long)(this->Rx_Buffer[21])) << 8) |
                        //                         this->Rx_Buffer[22];

                        // ESP_LOGD("custom", ">>> pf_data: %lu", pf_data);

                        // unsigned long co2_data = (((unsigned long)(this->Rx_Buffer[23])) << 24) |
                        //                         (((unsigned long)(this->Rx_Buffer[24])) << 16) |
                        //                         (((unsigned long)(this->Rx_Buffer[25])) << 8) |
                        //                         this->Rx_Buffer[26];

                        // ESP_LOGD("custom", ">>> co2_data: %lu", co2_data);

                        this->value = power_data / 1.0;
                    }
                }
                this->Rx_Counter = 0;
            }
            return false;
        }
};
