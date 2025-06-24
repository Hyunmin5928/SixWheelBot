// src/Pages/ReturnListPage/ReturnListPage.js
import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { getMyReturns } from '../../Services/return';

export default function ReturnListPage() {
  const userId     = localStorage.getItem('USER_ID');
  const [list, set] = useState([]);
  const nav        = useNavigate();

  useEffect(() => {
    getMyReturns(userId).then(set);
  }, [userId]);

  return (
    <div style={{color:'#e0e0e0', padding:'40px'}}>
      <h1>반품 내역</h1>
      {list.length === 0 && <p>내역이 없습니다.</p>}
      {list.map(r => (
        <div key={r.id}
             style={{border:'1px solid #555', padding:'12px', marginTop:'16px',
                     cursor:'pointer'}}
             onClick={() => nav(`/return/${r.id}`)}>
          <p><b>ID</b> : {r.id}</p>
          <p><b>주소</b> : {r.address}</p>
          <p><b>상태</b> : {r.status}</p>
        </div>
      ))}
    </div>
  );
}
