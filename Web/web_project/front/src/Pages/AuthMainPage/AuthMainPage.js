// src/Pages/AuthMainPage/AuthMainPage.js
import React, { useEffect } from 'react';
import { useNavigate, NavLink } from 'react-router-dom';

import logoImg       from '../../assets/logo.png';
import orderIcon     from '../../assets/order.png';
import returnIcon    from '../../assets/return.png';
import searchIcon    from '../../assets/search.png';
import serviceBanner from '../../assets/deliveryservices.png';

import styles from '../MainPage/MainPage.module.css';   // ⚙️  동일 CSS 재사용

export default function AuthMainPage() {
  const nav = useNavigate();

  /* 토큰이 없으면 게스트 메인(/)으로 돌려보냄 */
  useEffect(() => {
    if (!localStorage.getItem('TOKEN')) nav('/');
  }, [nav]);

  /* 로그아웃 → 토큰 삭제 후 / 로 이동 */
  const logout = () => {
    localStorage.removeItem('TOKEN');
    nav('/');
  };

  /* 배송·반품·조회 카드 데이터 */
  const cards = [
    { title: '배송 요청', imgSrc: orderIcon,  path: '/order/new' },
    { title: '반품 신청', imgSrc: returnIcon, path: '/return/new' },
    { title: '배송 조회', imgSrc: searchIcon, path: '/order' },
  ];

  return (
    <div className={styles.wrapper}>
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

      {/* ---------- Hero ---------- */}
      <section className={styles.hero}>
        <h1>배달의 육륜</h1>
        <p className={styles.copy}>
          쉽고 빠른<br />
          365일 신속<br />
          자율주행 배달로봇<br />
          육륜이
        </p>
      </section>

      {/* ---------- 운송장 검색 ---------- */}
      <div className={styles.trackingBox}>
        <input
          placeholder="운송장 번호를 입력하시고 배송 정보를 확인하세요."
        />
        <button><span className="material-icons">search</span></button>
      </div>

      {/* ---------- 배송 옵션 ---------- */}
      <section className={styles.service}>
        <h2>배송 옵션</h2>
        <div className={styles.circleWrap}>
          {cards.map(({ title, imgSrc, path }) => (
            <button key={title} onClick={() => nav(path)}>
              <img src={imgSrc} alt={title} />
              <span>{title}</span>
            </button>
          ))}
        </div>
      </section>

      {/* ---------- 배너 ---------- */}
      <section className={styles.banner}>
        <img src={serviceBanner} alt="SixWheel Delivery Services" />
      </section>

      {/* ---------- Footer ---------- */}
      <footer className={styles.footer}>
        <div className={styles.footerInner}>
          {/* SNS·링크 블록은 MainPage와 동일 — 필요 시 채워넣으세요 */}
        </div>
      </footer>
    </div>
  );
}
