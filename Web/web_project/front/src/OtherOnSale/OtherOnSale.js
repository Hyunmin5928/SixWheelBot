import './OtherOnSale.css';
import Card from './../Card/Card.js';
import axios from 'axios';
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

export default function OtherOnSale({ publisherName, postAuthorId }) {
    const navigate = useNavigate();
    const [otherProducts, setOtherProducts] = useState([]);
    const username = publisherName;

    useEffect(() => {
        const fetchPosts = async () => {
            try {
                const myObjectIDResponse = await axios.get('/userAuthorID', { params: { username } });
                const myObjectID = myObjectIDResponse.data;

                const otherOnSaleResponse = await axios.get('/userSalesPosts', { params: { myObjectID } });
                const otherOnSale = otherOnSaleResponse.data;

                if (otherOnSale.length > 1) {
                    setOtherProducts(otherOnSale.slice(0, 4));
                }
            } catch (error) {
                console.error('데이터를 가져오는데 실패했습니다.:', error);
            } 
        };

        fetchPosts();
    }, [username]);

    const lookAroundStore = async () => {
        try {
            navigate(`/store/${postAuthorId}`);
        } catch (error) {
            console.error('다른 상점 둘러보기 실패:', error);
        }
    }

    return (
      <div className="other-onsale-container">
        <div className="other-info-msg">
           <div className='other-user-info-msg'>
              <span>{publisherName}님이 현재 판매 중인 상품이에요.</span>
           </div>
           <div className='look-around' onClick={ lookAroundStore }>
              <span>{`가게 둘러보기 >`}</span>
           </div>
        </div>
        <div className="other-products">
          {otherProducts.map((post, index) => (
            <Card
              key={index}
              id={post._id}
              imgSrc={post.img[0]}
              title={post.title}
              price={post.price}
              uploadDate={post.date}
            />
          ))}
        </div>
      </div>
    );
}