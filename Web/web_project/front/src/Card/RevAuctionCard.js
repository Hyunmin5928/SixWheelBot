import './RevAuctionCard.css';
import { formatDate } from './formatDate';
import { useNavigate } from 'react-router-dom';

export default function RevAuctionCard({ id, imgSrc, postTitle, desiredPrice, uploadDate, progressTime}) {
    const navigate = useNavigate();
    const formattedDate = formatDate(uploadDate);

    const handleCardClick = () => {
        navigate(`/revAuctionDetail/${id}`);
    };

    return (
        <div className='rev-aution-card' onClick={handleCardClick}>
            <div className='rev-aution-card-img'>
                <img src={ imgSrc } alt='이미지'></img>
            </div>
            <div className='rev-aution-card-content'>
                <div className='rev-aution-card-title'>
                    <span>{ postTitle }</span>
                </div>
                <div className='rev-aution-card-price'>
                    <span>희망 가격 : { desiredPrice } 원</span>
                </div>
                <div className='rev-aution-card-upload-date'>
                    <span>{ formattedDate }</span>
                </div>
                <div className='rev-aution-card-remain-time'>
                    <span>진행시간 : { progressTime }</span>
                </div>
            </div>
        </div>
    )
}