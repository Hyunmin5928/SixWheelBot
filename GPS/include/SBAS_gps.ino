#include <SoftwareSerial.h>

SoftwareSerial gpsSerial(4, 3); // RX, TX

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);

  Serial.println("GPS 시작...");

  delay_ms(2000);  // GPS 모듈 부팅 대기

  enableSBAS();  // SBAS 기능 활성화

  Serial.println("SBAS 활성화 명령 전송 완료");
}

void loop() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    Serial.write(c); // PC로 GPS NMEA 데이터 출력
  }
}

void sendUBX(byte *msg, byte len) {
  for (byte i = 0; i < len; i++) {
    gpsSerial.write(msg[i]);
  }
}

// SBAS 활성화용 UBX-CFG-SBAS 메시지
void enableSBAS() {
  // UBX-CFG-SBAS 메시지: SBAS ON (0x06 0x16)
  byte ubxSetSBAS[] = {
    0xB5, 0x62,             // Sync chars
    0x06, 0x16,             // Class, ID (CFG-SBAS)
    0x08, 0x00,             // Length (8 bytes payload)
    0x01,                   // SBAS enabled
    0x03,                   // SBAS usage (0x01=range, 0x02=diff corr, 0x03=both)
    0x03,                   // Max SBAS prns
    0x00,                   // Scanmode2 (not used)
    0x00, 0x00, 0x00, 0x00, // Scanmode1 (use all)
    0x21, 0x99              // Checksum (calculated below)
  };

  sendUBX(ubxSetSBAS, sizeof(ubxSetSBAS));
}
