#include <Arduino.h>
#include "MPU6050/MPU6050.h"
#include "adcObj/adcObj.hpp"
#include <ArduinoLog.h>
#include "driver/can.h"
#include "aditional/aditional.hpp"


#define TX_GPIO_NUM   GPIO_NUM_14
#define RX_GPIO_NUM   GPIO_NUM_27
#define LOG_LEVEL LOG_LEVEL_VERBOSE





u_int16_t status;


MPU6050 mpu;
adcObj damper1(ADC1_CHANNEL_4);
adcObj damper2(ADC1_CHANNEL_4);
adcObj damper3(ADC1_CHANNEL_4);
adcObj damper4(ADC1_CHANNEL_4);

static const can_general_config_t g_config = {.mode = TWAI_MODE_NO_ACK, .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,        
                                                                    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,      
                                                                    .tx_queue_len = 1, .rx_queue_len = 5,                           
                                                                    .alerts_enabled = TWAI_ALERT_ALL,  .clkout_divider = 0,        
                                                                    .intr_flags = ESP_INTR_FLAG_LEVEL1};
static const can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

twai_message_t tx_msg_mpu;
twai_message_t tx_msg_mpu2;
twai_message_t tx_msg_damp;

void setup() {
  Serial.begin(9600);
  Log.begin(LOG_LEVEL, &Serial);

  Wire.begin();

  mpu.initialize();
  status=mpu.testConnection();

   if(status!=0) {
    Log.errorln("MPU6050 not initialized - ERROR STATUS: %d", status);
   }
   else {
    Log.notice("MPU status: %d\n", status);
    //Log.notice("Calculating offsets, do not move MPU6050\n");
    
    Log.notice("Done - MPU initialized\n");
   }

    status = can_driver_install(&g_config, &t_config, &f_config);
    if(status==ESP_OK){
      Log.noticeln("Can driver installed");
    }
    else {
      Log.errorln("Can driver installation failed with error: %s", esp_err_to_name(status));
    }

    status=can_start();
    if(status==ESP_OK){
      Log.noticeln("Can started");
    }
    else {
      Log.errorln("Can starting procedure failed with error: %s", esp_err_to_name(status));
    }

    
  tx_msg_mpu.data_length_code=6;
  tx_msg_mpu.identifier=0x113;
  tx_msg_mpu.flags=CAN_MSG_FLAG_NONE;
  tx_msg_mpu2.data_length_code=6;
  tx_msg_mpu2.identifier=0x114;
  tx_msg_damp.data_length_code=8;
  tx_msg_damp.identifier=0x112;
  tx_msg_damp.flags=CAN_MSG_FLAG_NONE;
}


void loop() {
  Log.notice("\n\n********New loop started********\n");

  
  
  //mpu.update();

  #if LOG_LEVEL==LOG_LEVEL_VERBOSE
  int16_t ax=0;
  int16_t ay=0;
  int16_t az=0;
  int16_t gx=0;
  int16_t gy=0;
  int16_t gz=0;
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
  Log.verboseln("Ax:%d, Ay:%d, Az:%d, Gx:%d, Gy:%d, Gz:%d", ax, ay, az, gx, gy, gz);
  
  int d1 = damper1.getVoltage();
  int d2 = damper2.getVoltage();
  int d3 = damper3.getVoltage();
  int d4 = damper4.getVoltage();
  Log.verboseln("D1: %d, D2: %d, D3: %d, D4: %d", d1, d2, d3, d4);
  
 
  tx_msg_damp.data[0]=d1/100;
  tx_msg_damp.data[1]=d1%100;
  tx_msg_damp.data[2]=d2/100;
  tx_msg_damp.data[3]=d2%100;
  tx_msg_damp.data[4]=d3/100;
  tx_msg_damp.data[5]=d3%100;
  tx_msg_damp.data[6]=d4/100;
  tx_msg_damp.data[7]=d4%100;

  convert(ax, tx_msg_mpu.data);
  convert(ay, tx_msg_mpu.data+2);
  convert(az, tx_msg_mpu.data+4);

  convert(gx, tx_msg_mpu2.data);
  convert(gy, tx_msg_mpu2.data+2);
  convert(gz, tx_msg_mpu2.data+4);

 
  Serial.println(tx_msg_mpu2.data[0]);
  Serial.println(tx_msg_mpu2.data[1]);
  Serial.println(tx_msg_mpu2.data[2]);
  Serial.println(tx_msg_mpu2.data[3]);
  Serial.println(tx_msg_mpu2.data[4]);
  Serial.println(tx_msg_mpu2.data[5]);
  
   
  #else


  tx_msg_damp.data[0]=damper1.getVoltage()/100;
  tx_msg_damp.data[1]=damper1.getVoltage()%100;
  tx_msg_damp.data[2]=damper2.getVoltage()/100;
  tx_msg_damp.data[3]=damper2.getVoltage()%100;
  tx_msg_damp.data[4]=damper3.getVoltage()/100;
  tx_msg_damp.data[5]=damper3.getVoltage()%100;
  tx_msg_damp.data[6]=damper4.getVoltage()/100;
  tx_msg_damp.data[7]=damper4.getVoltage()%100;
  
  int16_t ax=0;
  int16_t ay=0;
  int16_t az=0;
  int16_t gx=0;
  int16_t gy=0;
  int16_t gz=0;
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

  convert(ax, tx_msg_mpu.data);
  convert(ay, tx_msg_mpu.data+2);
  convert(az, tx_msg_mpu.data+4);

  convert(gx, tx_msg_mpu2.data);
  convert(gy, tx_msg_mpu2.data+2);
  convert(gz, tx_msg_mpu2.data+4);



 
  #endif
  
  
  status = can_transmit(&tx_msg_mpu, pdMS_TO_TICKS(1000));
   if(status==ESP_OK) {
    Log.noticeln("Can message sent");
  }
  else {
    Log.errorln("Can message sending failed with error code: %s ;\nRestarting CAN driver", esp_err_to_name(status));
    can_stop();
    can_driver_uninstall();
    can_driver_install(&g_config, &t_config, &f_config);
    status = can_start();
    if(status==ESP_OK) Log.error("Can driver restarted");
  }

    status = can_transmit(&tx_msg_mpu2, pdMS_TO_TICKS(1000));
    if(status==ESP_OK) {
    Log.noticeln("Can message sent");
    }
    else {
    Log.errorln("Can message sending failed with error code: %s ;\nRestarting CAN driver", esp_err_to_name(status));
    can_stop();
    can_driver_uninstall();
    can_driver_install(&g_config, &t_config, &f_config);
    status = can_start();
    if(status==ESP_OK) Log.error("Can driver restarted");
    }


  status = can_transmit(&tx_msg_damp, pdMS_TO_TICKS(1000));
  if(status==ESP_OK) {
    Log.noticeln("Can message sent");
  }
  else {
    Log.errorln("Can message sending failed with error code: %s ;\nRestarting CAN driver", esp_err_to_name(status));
    can_stop();
    can_driver_uninstall();
    can_driver_install(&g_config, &t_config, &f_config);
    status = can_start();
    if(status==ESP_OK) Log.errorln("Can driver restarted");
  }

  
}