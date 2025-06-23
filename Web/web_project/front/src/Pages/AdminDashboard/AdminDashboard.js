import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './AdminDashboard.module.css';

import { getOrders,  acceptOrder  } from '../../Services/order';
import { getReturns, acceptReturn } from '../../Services/return';

export default function AdminDashboard() {
  const nav = useNavigate();

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

  /* ---------- 요청 수락 ---------- */
  const accept = async (type, id) => {
    try {
      if (type === 'order') {
        await acceptOrder(id);
        setOrders((prev) => prev.filter((o) => o.id !== id));
      } else {
        await acceptReturn(id);
        setReturns((prev) => prev.filter((r) => r.id !== id));
      }
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

        <button onClick={() => accept(type, item.id)}>요청 수락</button>
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
    </div>
  );
}
