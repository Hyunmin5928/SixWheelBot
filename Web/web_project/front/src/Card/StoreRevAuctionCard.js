import './StoreRevAuctionCard.css';
import { formatDate } from './formatDate';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

export default function StoreRevAuctionCard({ id, imgSrc, postTitle, desiredPrice, uploadDate, progressTime, isMyStore }) {
    const navigate = useNavigate();
    const formattedDate = formatDate(uploadDate);

    const handleCardClick = () => {
        navigate(`/revAuctionDetail/${id}`);
    };

    const deleteRevAuctionPost = async () => {
        if (window.confirm('정말로 삭제하시겠습니까?')) {
            try {
                const response = await axios.delete(`/revAuctionPost/${id}`);
                alert(response.data);
                window.location.reload();
            } catch (error) {
                alert('삭제 실패: ' + error.message);
            }
        }
    }

    return (
        <div className='rev-auction-card-container'>
            <div className='rev-auction-card' onClick={handleCardClick}>
                <div className='rev-auction-card-img'>
                    <img src={ imgSrc } alt='이미지'></img>
                </div>
                <div className='rev-auction-card-content'>
                    <div className='rev-auction-card-title'>
                        <span>{ postTitle }</span>
                    </div>
                    <div className='rev-auction-card-price'>
                        <span>희망 가격 : { desiredPrice } 원</span>
                    </div>
                    <div className='rev-auction-card-upload-date'>
                        <span>{ formattedDate }</span>
                    </div>
                    <div className='rev-auction-card-remain'>
                        <div className='rev-auction-card-remain-time'>
                            <span>진행시간 : { progressTime }</span>
                        </div>
                    </div>
                </div>
            </div>
            {isMyStore ? 
                <div className='rev-auction-card-delete' onClick={ deleteRevAuctionPost }>
                    <div className='trash-btn'>
                        <i class="bi bi-trash"></i>
                    </div>
                </div> : ''
            }
        </div>
        
    )
}