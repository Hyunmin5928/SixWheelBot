import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './AdminDashboard.module.css';

import {
  getOrders,       // PENDING 조회
  acceptOrder,     // 수락 → PENDING → IN_PROGRESS
  stopOrder,       // 정지 신호
  returnOrder      // 복귀 신호
} from '../../Services/order';

import {
  getReturns,      // PENDING 조회
  acceptReturn,    // 수락
  stopReturn,      // 정지
  returnReturn     // 복귀
} from '../../Services/return';

// T map Static Map API key
const TMAP_API_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';

// Static Map URL 생성기
const makeTmapUrl = ({ lat, lng, zoom = 15, w = 950, h = 320 }) =>
  `https://apis.openapi.sk.com/tmap/staticMap` +
    `?appKey=${TMAP_API_KEY}` +
    `&longitude=${lng}` +
    `&latitude=${lat}` +
    `&coordType=WGS84GEO` +
    `&zoom=${zoom}` +
    `&markers=${lng},${lat}` +
    `&format=PNG` +
    `&width=${w}&height=${h}`;

export default function AdminDashboard() {
  const nav = useNavigate();

  // 권한 체크
  useEffect(() => {
    if (localStorage.getItem('ROLE') !== 'ADMIN') {
      nav('/');
    }
  }, [nav]);

  const logout = () => {
    localStorage.clear();
    nav('/');
  };

  // ■ State
  const [pendingOrders,     setPendingOrders]     = useState([]);
  const [inProgressOrders,  setInProgressOrders]  = useState([]);
  const [pendingReturns,    setPendingReturns]    = useState([]);
  const [inProgressReturns, setInProgressReturns] = useState([]);
  const [trackInfo,         setTrackInfo]         = useState(null);

  // 초기 PENDING 데이터 로드
  useEffect(() => {
    (async () => {
      try {
        const [ords, rets] = await Promise.all([
          getOrders(),
          getReturns()
        ]);
        setPendingOrders(ords);
        setPendingReturns(rets);
      } catch (e) {
        console.error('데이터 로딩 실패', e);
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

  // 요청 수락 핸들러
  const handleAccept = async (type, item) => {
    console.log('[DBG] accept', type, item);   
    try {
      if (type === 'order') {
        await acceptOrder(item.id);
        setInProgressOrders(prev => [...prev, item]);
        setPendingOrders(prev => prev.filter(o => o.id !== item.id));
      } else {
        await acceptReturn(item.id);
        setInProgressReturns(prev => [...prev, item]);
        setPendingReturns(prev => prev.filter(r => r.id !== item.id));
      }
      // 좌표 추적 시작
      setTrackInfo({ id: item.id, type, lat: null, lng: null });
    } catch (e) {
      alert('수락 실패: ' + (e.response?.data || e.message));
    }
  };

  // 정지 핸들러
  const handleStop = async (type, id) => {
    try {
      if (type === 'order') await stopOrder(id);
      else                await stopReturn(id);
      alert('정지 신호 전송됨');
    } catch (e) {
      alert('정지 실패: ' + (e.response?.data || e.message));
    }
  };

  // 복귀 핸들러
  const handleReturn = async (type, id) => {
    try {
      if (type === 'order') await returnOrder(id);
      else                await returnReturn(id);
      alert('복귀 신호 전송됨');
    } catch (e) {
      alert('복귀 실패: ' + (e.response?.data || e.message));
    }
  };

  // 카드 컴포넌트
  const ReqCard = ({ item, type, inProgress }) => {
    const fullAddr = [
      item.receiver?.address || item.address,
      item.receiver?.detail   || item.detail
    ].filter(Boolean).join(' ');

    return (
      <div className={styles.reqCard}>
        <div><span>회원 아이디&nbsp;:</span>{item.userId}</div>
        <div><span>{type==='order'?'받는 주소':'보내는 주소'}&nbsp;:</span>{fullAddr}</div>
        <div><span>물품 종류&nbsp;:</span>{item.itemType}</div>

        {!inProgress
          ? <button
              className={styles.btnAccept}
              onClick={() => handleAccept(type, item)}
            >요청 수락</button>
          : <div className={styles.inProgressBtns}>
              <button onClick={() => handleStop(type, item.id)}>정지</button>
              <button onClick={() => handleReturn(type, item.id)}>복귀</button>
            </div>
        }
      </div>
    );
  };

  return (
    <div className={styles.page}>
      {/* Header */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/admin')}>
          <img src={logoImg} alt="logo" /><span>SixWheel</span>
        </div>
        <nav className={styles.userMenu}>
          <button onClick={logout}>로그아웃</button>
          <button onClick={() => nav('/category')}>카테고리</button>
        </nav>
      </header>

      {/* Profile Pill */}
      <section className={styles.profilePill}>
        <div className={styles.profileIcon} /><strong>관리자 계정</strong>
      </section>

      {/* 1) 배송 요청(PENDING) */}
      <section className={styles.section}>
        <h2>배송 요청 내역</h2>
        <div className={styles.reqGroup}>
          {pendingOrders.length===0
            ? <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
            : pendingOrders.map(o => (
                <ReqCard key={o.id} item={o} type="order" inProgress={false} />
              ))
          }
        </div>
      </section>

      {/* 2) 반품 요청(PENDING) */}
      <section className={styles.section}>
        <h2>반품 요청 내역</h2>
        <div className={styles.reqGroup}>
          {pendingReturns.length===0
            ? <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
            : pendingReturns.map(r => (
                <ReqCard key={r.id} item={r} type="return" inProgress={false} />
              ))
          }
        </div>
      </section>

      {/* 3) 배송 중(IN_PROGRESS) */}
      <section className={styles.section}>
        <h2>배송 중인 물품 상황</h2>
        <div className={styles.reqGroup}>
          {inProgressOrders.length===0
            ? <p className={styles.empty}>진행 중인 물품이 없습니다.</p>
            : inProgressOrders.map(o => (
                <ReqCard key={o.id} item={o} type="order" inProgress={true} />
              ))
          }
        </div>
      </section>

      {/* 4) 반품 중(IN_PROGRESS) */}
      <section className={styles.section}>
        <h2>반품 중인 물품 상황</h2>
        <div className={styles.reqGroup}>
          {inProgressReturns.length===0
            ? <p className={styles.empty}>진행 중인 반품이 없습니다.</p>
            : inProgressReturns.map(r => (
                <ReqCard key={r.id} item={r} type="return" inProgress={true} />
              ))
          }
        </div>
      </section>

      {/* 5) 실시간 위치 (Static Map) */}
      {trackInfo?.lat && trackInfo?.lng && (
        <section className={styles.mapWrapper}>
          <h2>실시간 위치</h2>
          <img
            src={makeTmapUrl(trackInfo)}
            alt="실시간 위치"
          />
        </section>
      )}
    </div>
  );
}
