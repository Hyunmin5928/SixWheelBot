import { useParams } from 'react-router-dom';
import ClipLoader from "react-spinners/ClipLoader";
import React, { useEffect, useState } from 'react';
import { formatDate } from '../Card/formatDate';
import HeartChat  from './Heart-chat.js';
import EditDelete  from './Edit-delete.js';
import OtherOnSale from '../OtherOnSale/OtherOnSale.js';
import RecProducts from '../RecProducts/RecProducts.js';
import axios from 'axios';
import './Detail.css';

export default function Detail() {
    const { id } = useParams();
    const [isLoggedIn, setIsLoggedIn] = useState(false);
    const [userId, setUserId] = useState(null);
    const [userName, setUserName] = useState(null);
    const [loggedInUsername, setLoggedInUsername] = useState('');
    const [detailData, setDetailData] = useState({}); 
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);
    const [current, setCurrent] = useState(0);
    const [date, setDate] = useState('');
    const [postCategory, setPostCategory] = useState('');

    useEffect(() => {
        const fetchData = async () => {
            try {
                setLoading(true);
                const response = await axios.get(`/detail/${id}`);
                const loginResponse = await axios.get("/checkLogin");
                setIsLoggedIn(loginResponse.data.isLoggedIn);
                if (loginResponse.data.isLoggedIn) {
                    setUserId(loginResponse.data.userId);
                    setUserName(loginResponse.data.username);
                    setLoggedInUsername(loginResponse.data.username);
                    setPostCategory(response.data.category);
                }
                setDetailData(response.data);
                setDate(formatDate(response.data.date));
            } catch (err) {
                console.error('데이터를 가져오는데 실패했습니다.', err);
                setError(err);
            } finally {
                setLoading(false);
            }
        };

        fetchData();
    }, [id]);

    const nextSlide = () => {
        setCurrent(current === detailData.img.length - 1 ? 0 : current + 1);
    };

    const prevSlide = () => {
        setCurrent(current === 0 ? detailData.img.length - 1 : current - 1);
    };

    const moveToSlide = index => {
        setCurrent(index);
    };

    if (loading) {
        return (
            <div className='detail-loading-msg'>
               <div className='detail-loading-inside'>
                 <ClipLoader color="#dcdcdc" loading={loading} size={50} />
                 <span>데이터를 불러오고 있어요!</span>
               </div>
            </div>
        );
    }

    if (!Array.isArray(detailData.img) || detailData.img.length <= 0) {
        return null;
    }

    if (error) {
        return <div>데이터 못 가져옴.</div>;
    }

    const isAuthor = isLoggedIn && userId === detailData.authorId;
    const isAuthor2 = isLoggedIn && userId !== detailData.authorId;
    const publisherName = detailData.publisherName;
    const postAuthorId = detailData.authorId;

    return (
      <div className="detail-container">
        <div className='detail-user-info-msg'>
            <span>
                <i class="bi bi-box-seam-fill"></i>&nbsp;&nbsp;{ publisherName } 님의 상품
            </span>
        </div>
        <div className="detail-page">
          <div className="slider">
            <button className="left-arrow" onClick={prevSlide}>
              &#10094;
            </button>
            <button className="right-arrow" onClick={nextSlide}>
              &#10095;
            </button>
            {detailData.img.map((image, index) => (
              <div
                className={index === current ? "slide active" : "slide"}
                key={index}
              >
                {index === current && (
                  <img src={image} alt={image.alt} className="image" />
                )}
              </div>
            ))}
            <div className="swiper-pagination">
              {detailData.img.map((_, index) => (
                <span
                  key={index}
                  className={`dot ${index === current ? "active" : ""}`}
                  onClick={() => moveToSlide(index)}
                ></span>
              ))}
            </div>
          </div>
          <div className="word-heart-chat">
            <div className="title">{detailData.title}</div>
            <div className="date-viewer-category-type">
              <div className="date">{date}</div>
              <div className="viewer">조회수 {detailData.viewer} 회</div>
              <div className="category">{`[${detailData.category}]`}</div>
              <div className="type">{`[${detailData.type}]`}</div>
            </div>
            <div className="price">{detailData.price} 원</div>
            <div className="content-information">상품정보</div>
            <div className="content">{detailData.content}</div>
            {isAuthor ? <EditDelete id={ id } /> : <HeartChat postId={ detailData._id } myId={ userId } myName={ userName } writerId={ detailData.authorId } writerName={ detailData.publisherName } />}
          </div>
        </div>
        {!isAuthor && <OtherOnSale postAuthorId = { postAuthorId } publisherName = { publisherName } />}
        {isAuthor2 && <RecProducts userId = { userId } loggedInUsername = { loggedInUsername } postCategory = { postCategory } postId = { id } />}
      </div>
    )
}