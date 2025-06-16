import axios from 'axios';
import './Heart-chat.css';
import { useNavigate } from 'react-router-dom';
import { useEffect, useState } from 'react';
export default function HeartChat({ postId, myId, myName, writerId, writerName }) {
    const navigate = useNavigate();
    const [isInterested, setIsInterested] = useState(false);
    const [showModal, setShowModal] = useState(false);
    
    useEffect(() => {
        const checkIfInterested = async () => {
            try {
                const response = await axios.get(`/userInterestedProducts`);
                const products = response.data;

                const isAlreadyInterested = products.some(product => product._id === postId);
                setIsInterested(isAlreadyInterested);
            } catch (error) {
                console.error('관심상품을 조회하는데 실패했습니다.', error);
            }
        };
        checkIfInterested();
    }, [postId]);

    const toggleInterested = async () => {
        const response = await axios.get("/checkLogin");
        const userObjId  = response.data.userId;
        const isLoggedIn = response.data.isLoggedIn;
        if (!isLoggedIn) {
            navigate('/login');
        } else {
            try {
                if (isInterested) {
                    await axios.delete(`/interestedProducts?userObjId=${userObjId}&postId=${postId}`);
                } else {
                    await axios.post('/interestedProducts', { userObjId, postId });
                }
                setIsInterested(!isInterested);
                triggerModal();
            } catch (error) {
                console.error('에러가 발생했습니다.: ', error);
            }
        }
    };

    const startChat = async () => {
        try {
            const response = await axios.get("/checkLogin");
            const isLoggedIn = response.data.isLoggedIn;
            if (!isLoggedIn) {
                navigate('/login');
            } else {
                const chatRoomRes = await axios.get("/chat/request", { params: { myId, myName, writerId, writerName } });
                const roomId = chatRoomRes.data.roomId;
                if (chatRoomRes) {
                    navigate(`/chat/detail/${roomId}`);
                } 
            }        
        } catch (error) {
            console.error('에러가 발생했습니다.: ', error);
        }
    };

    const triggerModal = () => {
        setShowModal(true);
        setTimeout(() => {
            setShowModal(false);
        }, 2000); 
    };

    return (
        <div className='heart-chat'>
                <div className='heart' onClick={toggleInterested}>
                    <button className={`heart-btn ${isInterested ? 'interested' : ''}`}>
                        <i class="bi bi-suit-heart"></i>&nbsp;&nbsp;{isInterested ? `관심상품에 추가됨` : `관심상품 추가`}
                    </button>
                </div>
                <div className='chat' onClick={startChat}>
                    <button className='chat-btn'>
                        <i class="bi bi-chat-dots"></i>&nbsp;&nbsp;채팅하기
                    </button>
                </div>
                {showModal && (
                <div className="modal">
                    {isInterested ? '관심 상품에 추가했어요!' : '관심 상품에서 제거했어요!'}
                </div>
                )}
        </div>
    )
}