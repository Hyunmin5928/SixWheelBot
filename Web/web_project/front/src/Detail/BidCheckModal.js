import React, { useState } from 'react';
import axios from 'axios';
import './BidCheckModal.css';

export default function BidCheckModal({ onClose, loginUserId, revAuctionPostId }) {
    const maxBidPrice = 100000000;
    const [prevBidPrice, setPrevBidPrice] = useState('');
    const [bidPrice, setBidPrice] = useState('');

    const submitBid = async () => {
        const id = revAuctionPostId;
        if (!bidPrice) return; // 입찰 가격이 입력되지 않았다면 함수 종료
        try {
            if(window.confirm(`정말로 ${bidPrice}원으로 입찰을 진행하시겠습니까? 입찰을 진행하면 취소가 불가능하며 거래 불이행시 불이익이 발생합니다.`)) {
                const bidRes = await axios.post(`/revAuction/${id}/bids`, { bidder_id: loginUserId, bid_price: bidPrice });
                alert(bidRes.data);
                onClose(); // 모달 닫기
                window.location.reload();
            }
        } catch (error) {
            console.error('입찰 중 에러 발생: ', error);
            alert('입찰에 실패했습니다.');
        }
    };

    const formatNumber = (num) => {
        return num.toLocaleString(); //천 단위로 콤마 넣기 
      };
    
    const handleBidPriceChange = (event) => {
        // 쉼표를 포함하여 숫자가 아닌 모든 문자 제거
        const numericValue = event.target.value.replace(/,/g, '').replace(/\D/g, '');
        
        const cleanedNumber = numericValue ? parseInt(numericValue, 10) : "";

        if(cleanedNumber <= maxBidPrice) {
          const formattedNumber = formatNumber(cleanedNumber);
          setBidPrice(formattedNumber);
          setPrevBidPrice(formattedNumber);
        } else {
          setBidPrice(prevBidPrice);
        }
    };

    return (
        <div className="bid-modal">
            <div className="bid-modal-content">
                <span className="close" onClick={onClose}>&times;</span>
                <h2>입찰하기</h2>
                <input
                    type="text"
                    placeholder="입찰 가격 입력"
                    value={bidPrice}
                    onChange={handleBidPriceChange}
                />
                <button onClick={submitBid}>입찰</button>
            </div>
        </div>
    )
}