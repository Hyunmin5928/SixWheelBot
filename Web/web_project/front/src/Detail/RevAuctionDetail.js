import './RevAuctionDetail.css';
import { useParams, useNavigate } from 'react-router-dom';
import ClipLoader from "react-spinners/ClipLoader";
import CountdownTimer from './CountdownTimer/CountdownTimer.js';
import ChatBid from './Chat-bid.js';
import React, { useEffect, useState } from 'react';
import axios from 'axios';
import { formatDate } from '../Card/formatDate';

export default function RevAuctionDetail() {
    const { id } = useParams();
    const navigate = useNavigate();
    const [loading, setLoading] = useState(false);
    const [isLoggedIn, setIsLoggedIn] = useState(false);
    const [revDetailData, setRevDetailData] = useState({});
    const [biddersList, setBiddersList] = useState([]);
    const [isClosedOrAwarded, setIsClosedOrAwarded] = useState(null);
    const [date, setDate] = useState('');
    const [userId, setUserId] = useState(null);
    const [loginUserId, setLoginUserId] = useState('');

    useEffect(() => {
        const fetchData = async () => {
            try {
                setLoading(true);
                const response = await axios.get(`/revAuctionDetail/${id}`);
                const loginResponse = await axios.get("/checkLogin");
                const revAuctionStatusRes = await axios.get(`/checkRevAuctionStatus/${id}`);

               
                setIsLoggedIn(loginResponse.data.isLoggedIn);
                if (loginResponse.data.isLoggedIn) {
                    setUserId(loginResponse.data.userId);
                    setLoginUserId(loginResponse.data.username);
                }

                setIsClosedOrAwarded(revAuctionStatusRes.data.isClosedOrAwarded);
                setRevDetailData(response.data);
                setBiddersList(response.data.bidders);
                setDate(formatDate(response.data.date));
            } catch (err) {
                console.error('데이터를 가져오는데 실패했습니다.', err);
            } finally {
                setLoading(false);
            }
        };

        fetchData();
    }, [id]);

    useEffect(() => {
        if (isClosedOrAwarded) {
            alert('이미 종료된 경매입니다!');
            navigate('/');
        }
    }, [isClosedOrAwarded, navigate]);
    
    const handleAward = async (bidderId, bidPrice) => {
        try {
          if(window.confirm(`정말로 ${bidderId}님으로 낙찰을 진행하시겠습니까? 가격:${bidPrice}`)) {
            const response = await axios.post(`/revAuction/${id}/award`, {
                bidder_id: bidderId,
                bid_price: bidPrice
            });
            alert(`${response.data}`);
          }

        } catch (error) {
          console.error('낙찰 처리 중 에러 발생:', error);
          alert('낙찰 처리에 실패했습니다.');
        }
    };

    if (loading) {
        return (
            <div className='rev-auction-detail-loading-msg'>
               <div className='rev-auction-detail-loading-inside'>
                 <ClipLoader color="#dcdcdc" loading={loading} size={50} />
                 <span>데이터를 불러오고 있어요!</span>
               </div>
            </div>
        );
    }

    const publisherName = revDetailData.publisherName;
    const isAuthor = isLoggedIn && userId === revDetailData.authorId;

    return (
        <div className='rev-auction-detail-container'>
            <div className='rev-auction-detail-user-info-msg'>
                <span>
                    <i class="bi bi-box-seam-fill"></i>&nbsp;&nbsp;{publisherName} 님의 구매희망 상품
                </span>
            </div>
            <div className='rev-auction-detail-page'>
                <div className='rev-auction-detail-img-space'>
                    <img src={revDetailData.img} alt='역경매 사진' className="rev-auction-detail-img" />
                </div>
                <div className='rev-auction-detail-explanation'>
                    <div className="rev-auction-detail-title">{revDetailData.postTitle}</div>
                    <div className="rev-auction-detail-date-and-viewer">
                        <div className="date">{date}</div>
                        <div className="viewer">조회수 {revDetailData.viewer} 회</div>
                    </div>
                    <div className="rev-auction-detail-desiredPrice-and-time">
                        <span className='desiredPrice'>구매희망 가격 : {revDetailData.desiredPrice} 원</span>
                        <CountdownTimer id = { id } />
                    </div>
                    <div className="rev-auction-detail-content-information">상품정보</div>
                    <div className="rev-auction-detail-content">{revDetailData.postContent}</div>
                    {isAuthor ? '' : <ChatBid revAuctionPostId = { id } loginUserId = { loginUserId }/>}
                </div>
            </div>
            <div className='bid-list-title'>
                <span className='bid-list-title-span'>입찰 목록</span>
            </div>
            <div className='bid-list-table-title'>
                <div className={isAuthor ? 'bid-person-title-width-47' : 'bid-person-title-width-50'}>
                    <span>입찰자</span>
                </div>
                <div className={isAuthor ? 'bid-price-title-width-47' : 'bid-price-title-width-50'}>
                    <span>입찰가격</span>
                </div>
            </div>
            <div className={biddersList.length === 0 ? 'bidders-empty-msg' : 'bid-list-table-content'}>
                {
                    biddersList.length === 0 ? (
                        <div>
                            <span>입찰 중인 사용자가 없어요.</span>
                        </div>
                    ) : (
                        biddersList.map((data, index) => (
                            <div className='bid-person-and-price' key={index}>
                                <div className={isAuthor ? 'bid-person-width-47' : 'bid-person-width-50'}>
                                    <span>{data.bidder_id}</span>
                                </div>
                                <div className={isAuthor ? 'bid-price-width-47' : 'bid-price-width-50'}>
                                    <span>{data.bid_price}</span>
                                </div>
                                {isAuthor ? 
                                    <div className='award'>
                                        <button onClick={() => handleAward(data.bidder_id, data.bid_price)} className='award-btn' type='submit'>낙찰</button>
                                    </div> : ''
                                }
                            </div>
                        ))
                    )
                }
            </div>
        </div>
    )
}