// src/Components/Navbar/Navbar.js
import React from 'react';
import { NavLink } from 'react-router-dom';
import styles from './Navbar.module.css';

export default function Navbar() {
  return (
    <nav className={styles.navbar}>
      <div className={styles.logo}>
        <NavLink to="/">배송로봇</NavLink>
      </div>
      <ul className={styles.menu}>
        {[
          ['홈', '/'],
          ['배송요청', '/order/new'],
          ['반품요청', '/return/new'],
          ['배송조회', '/order'],
          ['마이페이지', '/mypage'],
          ['관리자', '/admin'],
          ['로그인', '/login'],
          ['회원가입', '/register'],
        ].map(([label, to]) => (
          <li key={to}>
            <NavLink to={to} className={({ isActive }) => isActive ? styles.active : undefined}>
              {label}
            </NavLink>
          </li>
        ))}
      </ul>
    </nav>
  );
}
