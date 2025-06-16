import './Card.css';
import { formatDate } from './formatDate';
import { useNavigate } from 'react-router-dom';


export default function Card({ id, imgSrc, title, price, uploadDate }) {
    const navigate = useNavigate();
    const formattedDate = formatDate(uploadDate);

    const handleCardClick = () => {
        navigate(`/detail/${id}`);
    };

    return (
        <div className='card' onClick={handleCardClick}>
            <div className='card-img'>
                <img src={ imgSrc } alt='이미지'></img>
            </div>
            <div className='card-content'>
                <div className='card-title'>
                    <span>{ title }</span>
                </div>
                <div className='card-price'>
                    <span>₩ { price }</span>
                </div>
                <div className='card-time'>
                    <span>{ formattedDate }</span>
                </div>
            </div>
        </div>
    )
}