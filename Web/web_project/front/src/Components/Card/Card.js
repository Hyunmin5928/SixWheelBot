// src/Components/Card/Card.js
import React from 'react';
import styles from './Card.module.css';

export default function Card({ id, imgSrc, title, price, uploadDate, onClick }) {
  return (
    <div className={styles.card} onClick={onClick}>
      <img src={imgSrc} alt={title} className={styles.image} />
      <div className={styles.info}>
        <h3 className={styles.title}>{title}</h3>
        <p className={styles.price}>{price}</p>
        <p className={styles.date}>{uploadDate}</p>
      </div>
    </div>
  );
}
