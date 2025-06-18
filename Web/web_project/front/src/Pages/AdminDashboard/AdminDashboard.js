// src/Pages/AdminDashboard/AdminDashboard.js
import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './AdminDashboard.module.css';

import { getOrders }  from '../../Services/order';
import { getReturns } from '../../Services/return';

export default function AdminDashboard() {
  const nav = useNavigate();

  /* -------- 권한 체크 -------- */
  useEffect(() => {
    if (localStorage.getItem('ROLE') !== 'ADMIN') nav('/');
  }, [nav]);

  /* -------- 로그아웃 -------- */
  const logout = () => {
    localStorage.clear();
    nav('/');
  };

  /* -------- 데이터 -------- */
  const [orders,  setOrders]  = useState([]);
  const [returns, setReturns] = useState([]);

  useEffect(() => {
    (async () => {
      try {
        setOrders(await getOrders());
        setReturns(await getReturns());
      } catch (e) { console.error(e); }
    })();
  }, []);

  /* -------- 카드 렌더링 -------- */
  const ReqCard = ({ item, type }) => (
    <div className={styles.reqCard}>
      <div><span>회원 아이디&nbsp;:</span>{item.userId}</div>
      {type === 'order' ? (
        <div><span>받는 주소&nbsp;:</span>{item.receiver.address}</div>
      ) : (
        <div><span>보내는 주소&nbsp;:</span>{item.sender.address}</div>
      )}
      <div><span>물품 종류&nbsp;:</span>{item.itemType}</div>

      <button onClick={() => alert(`${item.id} 수락 (TODO)`)}>
        요청 수락
      </button>
    </div>
  );

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
          {orders.length === 0
            ? <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
            : orders.map(o => <ReqCard key={o.id} item={o} type="order" />)}
        </div>
      </section>

      {/* ---------- 반품 요청 ---------- */}
      <section className={styles.section}>
        <h2>반품 요청 내역</h2>

        <div className={styles.reqGroup}>
          {returns.length === 0
            ? <p className={styles.empty}>대기 중인 요청이 없습니다.</p>
            : returns.map(r => <ReqCard key={r.id} item={r} type="return" />)}
        </div>
      </section>
    </div>
  );
}
