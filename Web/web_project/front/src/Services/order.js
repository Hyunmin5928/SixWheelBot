// src/Services/order.js
import axios from 'axios';

/* ------------------------------------------------------------------
   배송 요청 목록 조회
   ------------------------------------------------------------------ */
export const getOrders = () =>
  axios
    .get('/api/order', { withCredentials: true }) // 세션 쿠키 전송
    .then((res) => res.data);

/* ------------------------------------------------------------------
   새 배송 요청 생성
   payload 예시)
   {
     userId:   'ymh',
     receiver: { address:'서울 …', detail:'101동 1004호' },
     itemType: '서적'
   }
   ------------------------------------------------------------------ */
export const createOrder = (payload) =>
  axios
    .post('/api/order', payload, { withCredentials: true })
    .then((res) => res.data);

export const acceptOrder = (id) =>
  axios.post(`/api/order/${id}/accept`, {}, { withCredentials:true })
       .then(r => r.data);
