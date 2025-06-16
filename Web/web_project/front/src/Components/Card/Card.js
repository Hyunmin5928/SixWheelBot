// src/Components/Card/Card.js
import React from 'react';
import styles from './Card.module.css';

/** 공통 카드 컴포넌트 */
export default function Card({ title, detail, imgSrc, onClick }) {
  return (
    <div className={styles.card} onClick={onClick}>
      {/* 아이콘 / 썸네일 */}
      {imgSrc && (
        <div className={styles.iconWrap}>
          <img src={imgSrc} alt={title} className={styles.icon} />
        </div>
      )}

      <h3 className={styles.title}>{title}</h3>
      <p className={styles.detail}>{detail}</p>
    </div>
  );
}
