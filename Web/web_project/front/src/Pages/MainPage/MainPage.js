// src/Pages/MainPage/MainPage.js
import React from 'react';
import { useNavigate, NavLink } from 'react-router-dom';

import logoImg       from '../../assets/logo.png';
import orderIcon     from '../../assets/order.png';
import returnIcon    from '../../assets/return.png';
import searchIcon    from '../../assets/search.png';
import serviceBanner from '../../assets/deliveryservices.png';

import styles from './MainPage.module.css';

export default function MainPage() {
  const nav = useNavigate();

  /* 로그인 여부: localStorage에 TOKEN이 있으면 true */
  const isAuth = !!localStorage.getItem('TOKEN');

  /* 버튼별 라우팅 분기 */
  const handleOrderClick  = () => nav(isAuth ? '/order/new'  : '/login');
  const handleReturnClick = () => nav(isAuth ? '/return/new' : '/login');

  /* 카드 메타데이터 */
  const cards = [
    { title: '배송 요청', imgSrc: orderIcon,  onClick: handleOrderClick },
    { title: '반품 신청', imgSrc: returnIcon, onClick: handleReturnClick },
    { title: '배송 조회', imgSrc: searchIcon, onClick: () => nav('/order') },
  ];

  return (
    <div className={styles.wrapper}>
      {/* ---------- Header ---------- */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/')}>
          <img src={logoImg} alt="로고" />
          <span>SixWheel</span>
        </div>

        <nav className={styles.userMenu}>
          <NavLink to="/register">회원가입</NavLink>
          <NavLink to="/login">로그인</NavLink>
          <NavLink to="/category">카테고리</NavLink>
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
        <input placeholder="운송장 번호를 입력하시고 배송 정보를 확인하세요." />
        <button><span className="material-icons">search</span></button>
      </div>

      {/* ---------- 배송 옵션 카드 ---------- */}
      <section className={styles.service}>
        <h2>배송 옵션</h2>
        <div className={styles.circleWrap}>
          {cards.map(({ title, imgSrc, onClick }) => (
            <button key={title} onClick={onClick}>
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
          <div className={styles.sns}>
            <i className="ri-facebook-fill" />
            <i className="ri-instagram-line" />
            <i className="ri-youtube-fill" />
          </div>
          <div className={styles.footerLinks}>
            <strong>Site name</strong>
            <ul><li>Topic</li><li>Page</li><li>Page</li></ul>
            <ul><li>Topic</li><li>Page</li><li>Page</li></ul>
            <ul><li>Topic</li><li>Page</li><li>Page</li></ul>
          </div>
        </div>
      </footer>
    </div>
  );
}
