import React, { useEffect, useState } from 'react';
import { useNavigate, NavLink } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './OrderRequestPage.module.css';

import { getOrders, createOrder } from '../../Services/order';

export default function OrderRequestPage() {
  /* 로그인 사용자 ID */
  const userId = localStorage.getItem('USER_ID') || 'OOO';
  const nav    = useNavigate();

  /* ───────── 로그아웃 ───────── */
  const logout = () => {
    localStorage.removeItem('TOKEN');
    localStorage.removeItem('USER_ID');
    nav('/');
  };

  /* ───────── 통계 상태 ───────── */
  const [stats, setStats] = useState({ total: 0, prog: 0, done: 0 });

  /* ───────── 입력 상태 ───────── */
  const [addr,   setAddr]   = useState('');
  const [detail, setDetail] = useState('');
  const [item,   setItem]   = useState('');

  const ready = addr.trim() && detail.trim() && item.trim();

  /* ───────── 통계 로딩 ───────── */
  useEffect(() => {
    (async () => {
      try {
        const list = await getOrders();
        setStats({
          total: list.length,
          prog:  list.filter(o => o.status === 'IN_PROGRESS').length,
          done:  list.filter(o => o.status === 'COMPLETED').length,
        });
      } catch (e) {
        console.error('통계 로드 실패', e);
      }
    })();
  }, []);

  /* ───────── 신청 ───────── */
  const submit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      await createOrder({
        userId,                                 // 로그인한 회원 ID
        receiver: {                            // 한 문자열에 합쳐 보냄
          address: `${addr} ${detail}`.trim()
        },
        itemType: item
      });
      alert('배송 신청이 완료되었습니다.');
      setAddr('');
      setDetail('');
      setItem('');
      nav('/home');
    } catch (err) {
      alert(err.response?.data || err.message);
    }
  };

  /* ───────── 렌더링 ───────── */
  return (
    <div className={styles.page}>
      {/* ---------- Header ---------- */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/home')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>

        <nav className={styles.userMenu}>
          <NavLink to="/mypage"   className={styles.menuLink}>마이페이지</NavLink>
          <button onClick={logout} className={styles.menuLink}>로그아웃</button>
          <NavLink to="/category" className={styles.menuLink}>카테고리</NavLink>
        </nav>
      </header>

      {/* ---------- Profile Box ---------- */}
      <section className={styles.profileBox}>
        <div className={styles.profileIcon} />
        <p>{userId} 님의 배송 현황</p>
      </section>

      {/* ---------- Stats ---------- */}
      <section className={styles.statsBox}>
        <header>
          <h3>배송 내역</h3>
          <button onClick={() => nav('/order')}>&gt; 더보기</button>
        </header>
        <ul>
          <li><em>전체</em><strong>{stats.total}</strong></li>
          <li><em>진행중</em><strong>{stats.prog}</strong></li>
          <li><em>종료</em><strong>{stats.done}</strong></li>
        </ul>
      </section>

      {/* ---------- Form ---------- */}
      <form className={styles.form} onSubmit={submit}>
        <h2>받는 주소</h2>

        <label>
          <span>주소</span>
          <input
            value={addr}
            onChange={(e) => setAddr(e.target.value)}
            placeholder="도로명, 지번을 입력하세요."
          />
        </label>

        <label>
          <span>상세주소</span>
          <input
            value={detail}
            onChange={(e) => setDetail(e.target.value)}
            placeholder="건물명 상세주소를 입력하세요. ex) 000동 0000호"
          />
        </label>

        <label>
          <span>물품 종류</span>
          <input
            value={item}
            onChange={(e) => setItem(e.target.value)}
            placeholder="물품 종류를 입력하세요. ex) 폭탄"
          />
        </label>

        <button
          type="submit"
          disabled={!ready}
          className={ready ? styles.submit : styles.disabled}
        >
          배송 신청하기
        </button>
      </form>
    </div>
  );
}
