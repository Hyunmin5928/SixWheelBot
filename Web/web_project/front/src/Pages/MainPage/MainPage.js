import React from 'react';
import { useNavigate } from 'react-router-dom';
import Card from '../../Components/Card/Card';
import styles from './MainPage.module.css';

export default function MainPage() {
  const navigate = useNavigate();

  return (
    <div className={styles.container}>
      <h2 className={styles.title}>서비스 선택</h2>
      <div className={styles.cards}>
        <Card
          title="배송 요청"
          detail="새로운 배송을 요청합니다."
          onClick={() => navigate('/order/new')}
        />
        <Card
          title="반품 신청"
          detail="상품 반품을 신청합니다."
          onClick={() => navigate('/return/new')}
        />
        <Card
          title="배송 조회"
          detail="송장번호로 배송 상태를 확인합니다."
          onClick={() => navigate('/order')}
        />
      </div>
    </div>
  );
}
