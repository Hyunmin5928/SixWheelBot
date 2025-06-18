// src/Services/return.js
// 서버 준비가 끝나면 ★ 표시된 부분을 axios.post 로 되돌리면 됩니다.
import axios from 'axios';

/* ① 반품 목록 */
export const getReturns = async () => {
  // 서버가 준비돼 있으면 아래 axios 사용
  // const res = await axios.get('/api/return');
  // return res.data;

  /* ★ 서버 미구현 시 → 더미 데이터 */
  return [];                         // 통계 0,0,0 으로 표시
};

/* ② 반품 생성 — MOCK 버전 */
export const createReturn = async (payload) => {
  // payload 예: { sender: { address:'...', detail:'...' } }

  /* ★ 서버 미구현 시 → 0.8초 후 가짜 성공 응답 */
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        id:       Date.now(),        // 더미 return ID
        status:   'IN_PROGRESS',
        ...payload,
      });
    }, 800);
  });

  /* ▼ 실제 서버가 준비되면 주석 해제 ↓
  const res = await axios.post('/api/return', payload);
  return res.data;            // { id, status, ... }
  */
};
