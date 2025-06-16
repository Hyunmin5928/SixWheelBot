import React from 'react';
import { NavLink, useNavigate } from 'react-router-dom';

import logoImg   from '../../assets/logo.png';
import orderIcon from '../../assets/order.png';
import returnIcon from '../../assets/return.png';
import searchIcon from '../../assets/search.png';

import styles from './MainPage.module.css';

export default function MainPage() {
  const nav = useNavigate();

  return (
    <div className={styles.wrapper}>
      {/* 1) 헤더 */}
      <header className={styles.header}>
        <div className={styles.logoArea}>
          <img src={logoImg} alt="로고" />
          <span>SixWheel</span>
        </div>

        <nav className={styles.menu}>
          <NavLink to="/login">로그인</NavLink>
          <NavLink to="/category">카테고리</NavLink>
        </nav>
      </header>

      {/* 2) 히어로 카피 */}
      <section className={styles.hero}>
        <h1>배달의 육륜</h1>
        <p className={styles.copy}>
          쉽고 빠른<br />
          365일 신속<br />
          자율주행 배달로봇<br />
          육륜이
        </p>
      </section>

      {/* 3) 운송장 검색 */}
      <div className={styles.trackingBox}>
        <input placeholder="운송장 번호를 입력하시고 배송 정보를 확인하세요." />
        <button>
          <span className="material-icons">search</span>
        </button>
      </div>

      {/* 4) 서비스 옵션 */}
      <section className={styles.service}>
        <h2>배송 옵션</h2>
        <div className={styles.circleWrap}>
          <button onClick={() => nav('/order/new')}>
            <img src={orderIcon} alt="배송" />
            <span>배송</span>
          </button>
          <button onClick={() => nav('/return/new')}>
            <img src={returnIcon} alt="반품" />
            <span>반품</span>
          </button>
          <button onClick={() => nav('/order')}>
            <img src={searchIcon} alt="조회" />
            <span>조회</span>
          </button>
        </div>
      </section>

      {/* 5) 푸터 */}
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
