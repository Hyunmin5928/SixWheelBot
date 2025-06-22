// control_client.js
const dgram = require('dgram');

const PYTHON_IP           = '127.0.0.1';  // Python 서버 IP
const PYTHON_CONTROL_PORT = 6001;         // Python 제어용 포트

function sendControl(type, payload = {}) {
  const msg    = JSON.stringify({ type, ...payload });
  const client = dgram.createSocket('udp4');

  client.send(Buffer.from(msg), PYTHON_CONTROL_PORT, PYTHON_IP, (err) => {
    if (err) console.error('UDP 제어 메시지 전송 에러:', err);
    else     console.log(`Control "${type}" 메시지 전송 완료`);
    client.close();
  });
}

// 사용 예
sendControl('start',  { order_id: 'ORDER123' }); // 배달 시작 요청
sendControl('unlock');                            // 적재함 잠금 해제
sendControl('return');                            // 복귀 명령
