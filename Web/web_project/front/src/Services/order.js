// src/Services/order.js
// ※ getOrders(조회) 는 그대로 두고, createOrder 만 “가짜”로 바꿉니다.

import axios from 'axios';

/* (이미 있던) 주문 목록 */
export const getOrders = async () => {
  // 서버가 준비돼 있으면 아래 axios 사용
  // const res = await axios.get('/api/order');
  // return res.data;

  /* 서버가 아직 없다면 → 더미 데이터 반환 */
  return [];                          // 통계 0,0,0 으로 표시
};

/* ------------ 새로 정의: createOrder MOCK ------------ */
export const createOrder = async (payload) => {
  /* 
     실제로는 axios.post('/api/order', payload) 를 호출해야 하지만
     서버가 500 을 내기 때문에, 일단 프런트 개발을 위해
     1초 후에 가짜 성공 응답을 돌려주는 방식으로 모킹합니다.
  */
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        id: Date.now(),            // 더미 주문 ID
        status: 'IN_PROGRESS',
        ...payload,                // 요청 내용 echo
      });
    }, 1000);
  });
};
