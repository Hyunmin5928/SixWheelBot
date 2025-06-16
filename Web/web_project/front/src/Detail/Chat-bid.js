import './Chat-bid.css';
import axios from 'axios';
import { useNavigate } from 'react-router-dom';
import React, { useState } from 'react';
import BidCheckModal from './BidCheckModal.js';

export default function ChatBid({ loginUserId, revAuctionPostId }) {
    const navigate = useNavigate();
    const [showBidModal, setShowBidModal] = useState(false);

    const startChat = async () => {
        try {
            const response = await axios.get("/checkLogin");
            const isLoggedIn = response.data.isLoggedIn;
            if (!isLoggedIn) {
                navigate('/login');
            } 
        } catch (error) {
            console.error('에러가 발생했습니다.: ', error);
        }
    }

    const doBid = async () => {
        try {
            const loginResponse = await axios.get('/checkLogin');
            const checkBidderRes = await axios.get('/checkDuplicateBid', { params: { bidderId: loginUserId, revAuctionPostId: revAuctionPostId } });
            const isLoggedIn = loginResponse.data.isLoggedIn;
            if (!isLoggedIn) {
                navigate('/login');
                return;
            } else if(isLoggedIn && checkBidderRes.data === true) {
                alert('이미 입찰 진행 중인 상품입니다.');
            } else {
                setShowBidModal(true);
            }
        } catch (error) {
            console.error('에러가 발생했습니다.: ', error);
        }
    }

    return (
        <div className='chat-bid'>
            <div className='chat-div' onClick={startChat}>
                <button className='chat-btn'>
                    <i class="bi bi-chat-dots"></i>&nbsp;&nbsp;채팅하기
                </button>
            </div>
            <div className='bid-div' onClick={doBid}>
                <button className='bid-btn'>
                    <i class="bi bi-pin-angle"></i>&nbsp;&nbsp;입찰하기
                </button>
            </div>
            {showBidModal && <BidCheckModal revAuctionPostId = { revAuctionPostId } loginUserId = { loginUserId } onClose={() => setShowBidModal(false)} />}
        </div>
    )
}