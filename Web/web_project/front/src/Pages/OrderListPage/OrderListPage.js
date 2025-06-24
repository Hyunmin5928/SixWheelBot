import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { getOrders } from '../../Services/order';
import styles from './OrderListPage.module.css';

export default function OrderListPage() {
  const nav    = useNavigate();
  const userId = localStorage.getItem('USER_ID');
  const [list, setList] = useState([]);

  useEffect(() => { getOrders(userId).then(setList); }, [userId]);

  return (
    <div className={styles.page}>
      <h1>배송 내역</h1>
      {list.length === 0
        ? <p className={styles.empty}>내역이 없습니다.</p>
        : (
          <ul className={styles.cards}>
            {list.map(o => (
              <li key={o.id} onClick={() => nav(`/order/${o.id}`)}>
                <p><strong>주소&nbsp;</strong>{o.address}</p>
                <p><strong>종류&nbsp;</strong>{o.itemType}</p>
                <p><strong>상태&nbsp;</strong>{o.status}</p>
              </li>
            ))}
          </ul>
        )}
    </div>
  );
}
