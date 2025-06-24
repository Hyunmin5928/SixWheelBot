// 주문 상세 페이지
import React, { useEffect, useState } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import logoImg  from '../../assets/logo.png';
import styles   from './OrderDetailPage.module.css';
import {
  getOrderById,        // ↙︎ 새로 추가한 서비스 함수
  lockOrder, unlockOrder, completeOrder
} from '../../Services/order';

export default function OrderDetailPage() {
  const { id }    = useParams();          // /order/:id
  const navigate  = useNavigate();
  const [order, setOrder] = useState({
    receiver: { address: '' },
    itemType: '',
    status:   ''
  });

  /* 주문 상세 로드 -------------------------------------------------- */
  useEffect(() => {
    (async () => {
      try {
        const data = await getOrderById(id);
        setOrder(data);
      } catch (e) {
        alert('주문 정보를 불러오지 못했습니다.');    // 404 등
        navigate('/home');
      }
    })();
  }, [id, navigate]);

  /* 실시간 좌표 스트리밍 (SSE) -------------------------------------- */
  useEffect(() => {
    const src = new EventSource(`/api/v1/orders/${id}/coords/stream`);
    src.onmessage = (e) => {
      const { lat, lng } = JSON.parse(e.data);
      // TODO: 지도 라이브러리를 쓰는 경우 마커 위치 업데이트
      // 지금은 콘솔만 확인합니다.
      console.log('SSE 좌표', lat, lng);
    };
    return () => src.close();
  }, [id]);

  /* 제어 버튼 ------------------------------------------------------- */
  const handleLock     = () => lockOrder(id);
  const handleUnlock   = () => unlockOrder(id);
  const handleComplete = () => completeOrder(id);

  /* ---------------------------------------------------------------- */
  return (
    <div className={styles.page}>
      {/* Header ----------------------------------------------------- */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => navigate('/home')}>
          <img src={logoImg} alt="logo" /><span>SixWheel</span>
        </div>
      </header>

      {/* Profile ---------------------------------------------------- */}
      <section className={styles.profileBox}>
        <div className={styles.profileIcon} />
        <p>배송 상세</p>
      </section>

      {/* 상세 정보 --------------------------------------------------- */}
      <section className={styles.detailContainer}>
        <div className={styles.innerInfo}>
          <div className={styles.field}>
            <label>받는 주소 :</label>
            <input value={order.receiver.address} readOnly />
          </div>
          <div className={styles.field}>
            <label>물품 종류 :</label>
            <input value={order.itemType} readOnly />
          </div>
          <div className={styles.field}>
            <label>상태 :</label>
            <input value={order.status} readOnly />
          </div>
        </div>

        <div className={styles.actions}>
          <button onClick={handleLock}>잠금</button>
          <button onClick={handleUnlock}>잠금 해제</button>
          <button onClick={handleComplete}>배송 완료</button>
        </div>
      </section>

      {/* 지도 ------------------------------------------------------- */}
      <section className={styles.mapSection}>
        <h2>배송로봇 실시간 위치</h2>
        <div id="map" className={styles.mapContainer}>
          {/* 지도를 넣을 경우 이 div 를 마운트 포인트로 사용 */}
        </div>
      </section>
    </div>
  );
}
