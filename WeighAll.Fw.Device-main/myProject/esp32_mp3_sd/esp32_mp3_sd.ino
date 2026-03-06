
#include "driver_config.h"

extern Audio audio;

void cb_tcp_check_connection(void) {
    TCP_Client_CheckConnection();

}

void cb_tcp_handle_receive(void) {
    TCP_Client_HandleReceive();

}

void cb_tcp_handle_ping(void) {
    TCP_Client_HandlePing();

}

void cb_udp_broadcast(void) {
    udp_process();
}

void Ethernet_Init()
{
    digitalWrite(W5500_CS, HIGH);
    Ethernet_Initialization();
}

void cb_process_even(void){
    ProcessEven();
}

void setup() {
  InitDebugSerial();
  
  //Speech
  SD_Card_Init();
  I2S_Init();
  pinMode(1, OUTPUT); // Đèn báo đã kết nối Server
  digitalWrite(1, LOW);

  // Ethernet
  Ethernet_Init();
  udp_broadcast_init();

  // Khởi tạo và bắt đầu soft timer
  soft_timer_init();
  soft_timer_start(5000, true, cb_tcp_check_connection); // Kiểm tra kết nối mỗi 1 giây
  soft_timer_start(30, true, cb_tcp_handle_receive);     // Xử lý dữ liệu mỗi 30ms
  soft_timer_start(500, true, cb_tcp_handle_ping);       // Xử lý ping mỗi 500ms
  soft_timer_start(500, true, cb_udp_broadcast); //100–200 ms	nếu muốn phản ứng nhanh hơn
  soft_timer_start(500, true, cb_process_even);

  IPAddress saved_ip;
  uint16_t saved_port;
  if (load_server_config(saved_ip, saved_port)) {
      serverIP = saved_ip;
      serverPort = saved_port;

      // Tùy chọn: tự động kết nối lại server khi khởi động
      if (tcpClient.connect(serverIP, serverPort)) {
          Serial.println("[TCP] Connected to saved server config.");
          EventQueue_Push(EVENT_TCP_CONNECTED, NULL);
      } else {
          Serial.println("[TCP] Failed to connect to saved server.");
          EventQueue_Push(EVENT_TCP_DISCONNECTED, NULL);
      }
  }

}

void loop() {
  audio.loop(); // always needed for audio playback

  GFX_Print_Ethernet_Info_Loop();


}



