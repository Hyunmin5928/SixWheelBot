import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './AdminDashboard.module.css';

import { getOrders,  acceptOrder  } from '../../Services/order';
import { getReturns, acceptReturn } from '../../Services/return';


 const TMAP_API_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';


const makeTmapUrl = ({ lat, lng, zoom = 15, w = 950, h = 320 }) =>
  `https://apis.openapi.sk.com/tmap/staticMap` +
  `?appKey=${TMAP_API_KEY}` +
  `&longitude=${lng}` +
  `&latitude=${lat}` +
  `&coordType=WGS84GEO` +        // WGS84 좌표계
  `&zoom=${zoom}` +
  `&markers=${lng},${lat}` +     // 쉼표로 구분
  `&format=PNG` +
  `&width=${w}&height=${h}`;




export default function AdminDashboard() {
  const nav = useNavigate();
  console.log('[DBG] AdminDashboard 렌더');

  /* ---------- 권한 체크 ---------- */
  useEffect(() => {
    if (localStorage.getItem('ROLE') !== 'ADMIN') nav('/');
  }, [nav]);

  /* ---------- 로그아웃 ---------- */
  const logout = () => {
    localStorage.clear();
    nav('/');
  };

  /* ---------- 데이터 ---------- */
  const [orders,  setOrders]  = useState([]);
  const [returns, setReturns] = useState([]);

// ★ 실시간 위치(state)
//    trackInfo = { id, type, lat, lng }
  const [trackInfo, setTrackInfo] = useState(null);

  useEffect(() => {
    (async () => {
      try {
        setOrders(await getOrders());      // PENDING 배송
        setReturns(await getReturns());    // PENDING 반품
      } catch (e) {
        console.error('데이터 로드 실패', e);
      }
    })();
  }, []);


useEffect(() => {
   console.log('[DBG] useEffect enter', trackInfo);
  if (!trackInfo?.id) {
    console.log('[DBG] no id, return');
    return;   }        // (1) 아직 추적 대상 없음
  console.log('[DBG] id ok → open SSE');
  // ───── ① 연결 만들기 ─────
  const url = `/api/v1/${trackInfo.type}s/${trackInfo.id}/coords/stream`;
  console.log('[SSE] connect →', url);

  const src = new EventSource(url);

  src.onopen = () => console.log('[SSE] OPEN', url);

  src.onmessage = (e) => {
     const {lat, lng} = JSON.parse(e.data);
    //const lat = 37.339775;
    //const lng = 127.108942;
    console.log('[SSE] message', { lat, lng });

    setTrackInfo((prev) => {
      const next = { ...prev, lat, lng };
      console.log('[state] setTrackInfo →', next);
      return next;
    });
  };

  src.onerror = (err) => {
    console.error('[SSE] ERROR', err);
  };

  // ───── ② 클린업 ─────
  return () => {
    console.log('[SSE] CLOSE', url);
    src.close();
  };
}, [trackInfo?.id]);



  /* ---------- 요청 수락 ---------- */
  const accept = async (type, item )=> {
    console.log('[DBG] accept', type, item);           // 클릭 여부
    try {
      if (type === 'order') {
        await acceptOrder(item.id);
        setOrders(prev => prev.filter(o => o.id !== item.id));
      } else {
        await acceptReturn(item.id);
        setReturns(prev => prev.filter(r => r.id !== item.id));
      }
      // ★ 지도 트래킹 시작
     // setTrackInfo({ id: item.id, type });
     // ★ accept() 맨 아래에 좌표를 직접 넣어 본다
      setTrackInfo({ id: item.id, type, lat: 37.339775, lng: 127.108942 });
      console.log('[DBG] setTrackInfo done');
    } catch (e) {
      alert('수락 실패: ' + (e.response?.data || e.message));
    }
  };

  /* ---------- 카드 ---------- */
  const ReqCard = ({ item, type }) => {
    /* 주소·상세를 한 줄로 결합 */
    const fullAddr =
      item.address ||                             // 서버에서 이미 합쳐 왔으면 그대로
      (type === 'order'
        ? [item.receiver?.address, item.receiver?.detail]
        : [item.sender?.address,  item.sender?.detail]
      )
        .filter(Boolean)
        .join(' ');

    return (
      <div className={styles.reqCard}>
        <div><span>회원 아이디&nbsp;:</span>{item.userId}</div>
        {type === 'order' ? (
          <div><span>받는 주소&nbsp;:</span>{fullAddr}</div>
        ) : (
          <div><span>보내는 주소&nbsp;:</span>{fullAddr}</div>
        )}
        <div><span>물품 종류&nbsp;:</span>{item.itemType}</div>

        <button onClick={() => accept(type, item)}>요청 수락</button>
      </div>
    );
  };

  return (
    <div className={styles.page}>
      {/* ---------- Header ---------- */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/admin')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>
        <nav className={styles.userMenu}>
          <button onClick={logout}>로그아웃</button>
          <button onClick={() => nav('/category')}>카테고리</button>
        </nav>
      </header>

      {/* ---------- Profile pill ---------- */}
      <section className={styles.profilePill}>
        <div className={styles.profileIcon} />
        <strong>관리자 계정</strong>
      </section>

      {/* ---------- 배송 요청 ---------- */}
      <section className={styles.section}>
        <h2>배송 요청 내역</h2>
        <div className={styles.reqGroup}>
          {orders.length === 0 ? (
            <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
          ) : (
            orders.map((o) => <ReqCard key={o.id} item={o} type="order" />)
          )}
        </div>
      </section>

      {/* ---------- 반품 요청 ---------- */}
      <section className={styles.section}>
        <h2>반품 요청 내역</h2>
        <div className={styles.reqGroup}>
          {returns.length === 0 ? (
            <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
          ) : (
            returns.map((r) => <ReqCard key={r.id} item={r} type="return" />)
          )}
        </div>
      </section>

      {/* ---------- 실시간 지도 ---------- */}
      {trackInfo?.lat && trackInfo?.lng && (
        <section className={styles.mapWrapper}>
          <img
            src={makeTmapUrl(trackInfo)}
            alt="실시간 위치"
            style={{ width: '100%', height: 'auto', borderRadius: '6px' }}
          />
        </section>
      )}

    </div>
  );
}
