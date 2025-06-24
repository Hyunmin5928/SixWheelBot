// src/Pages/ReturnDetailPage/ReturnDetailPage.js
import React, { useEffect, useState } from 'react';
import { useNavigate, useParams } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './ReturnDetailPage.module.css';

/* T-map Static Map */
const TMAP_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';
const makeTmapUrl = ({ lat, lng, zoom = 16, w = 860, h = 400 }) =>
  `https://apis.openapi.sk.com/tmap/staticMap?appKey=${TMAP_KEY}` +
  `&longitude=${lng}&latitude=${lat}&zoom=${zoom}` +
  `&width=${w}&height=${h}&format=PNG&markers=${lng},${lat}&coordType=WGS84GEO`;

export default function ReturnDetailPage() {
  const { id }   = useParams();
  const nav      = useNavigate();

  const [ret,   setRet]   = useState({ sender:{ address:'' }, itemType:'' });
  const [coord, setCoord] = useState(null);

  /* ───── 반품 상세 로드 ───── */
  useEffect(() => {
    fetch(`/api/return/${id}`)
      .then(res => res.json())
      .then(setRet)
      .catch(e  => console.error('반품 상세 로드 실패', e));
  }, [id]);

  /* ───── 좌표 스트리밍 ───── */
  useEffect(() => {
    const es = new EventSource(`/api/v1/returns/${id}/coords/stream`);
    es.onmessage = e => setCoord(JSON.parse(e.data));
    es.onerror   = err => console.error('SSE 오류', err);
    return () => es.close();
  }, [id]);

  /* ───── 제어 버튼 ───── */
  const action = async (path, msg) => {
    try {
      await fetch(path, { method:'POST' });
      alert(msg);
    } catch (e) {
      console.error(e);
      alert('요청 실패');
    }
  };

  return (
    <div className={styles.page}>
      {/* Header */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/home')}>
          <img src={logoImg} alt="logo"/><span>SixWheel</span>
        </div>
      </header>

      {/* Profile pill */}
      <section className={styles.profilePill}>
        <div className={styles.profileIcon}/><strong>반품 상세</strong>
      </section>

      {/* 상세 + 버튼 */}
      <section className={styles.detailBox}>
        <div className={styles.inner}>
          <div className={styles.row}>
            <label>보내는 주소&nbsp;:</label>
            <input value={ret.sender.address} readOnly/>
          </div>
          <div className={styles.row}>
            <label>물품 종류&nbsp;:</label>
            <input value={ret.itemType} readOnly/>
          </div>
        </div>

        <div className={styles.btnCol}>
          <button onClick={() => action(`/api/return/${id}/lock`,     '잠금 되었습니다.')}>잠금</button>
          <button onClick={() => action(`/api/return/${id}/unlock`,   '잠금 해제 되었습니다.')}>잠금 해제</button>
          <button onClick={() => action(`/api/return/${id}/complete`, '반품 시작 되었습니다.')}>반품 시작</button>
        </div>
      </section>

      {/* 지도 */}
      <section className={styles.mapSection}>
        <h2>배송로봇 실시간 위치</h2>
        {coord
          ? <img src={makeTmapUrl(coord)} alt="현재 위치" className={styles.mapImg}/>
          : <div className={styles.mapPlaceholder}>위치 수신 대기 중…</div>}
      </section>
    </div>
  );
}
