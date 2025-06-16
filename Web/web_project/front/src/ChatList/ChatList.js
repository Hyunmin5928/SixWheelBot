import './ChatList.css';
import ClipLoader from "react-spinners/ClipLoader";
import { useParams, useNavigate } from 'react-router-dom';
import { useEffect, useState } from 'react';
import axios from 'axios';

export default function ChatList() {
    const { userId } = useParams();
    const navigate = useNavigate();
    const [chatRooms, setChatRooms] = useState([]);
    const [username, setUsername] = useState('');
    const [loading, setLoading] = useState(false);

    useEffect(() => {   
        const fetchUserData = async () => {
            try {
                const response = await axios.get(`/user/${userId}`);
                setUsername(response.data.username);
            } catch (error) {
                console.error('사용자 정보를 가져오는데 실패했습니다.', error);
            }
        };

        const fetchChatRooms = async () => {
            try {
                setLoading(true);
                const response = await axios.get(`/chat/rooms?userId=${userId}`);
                setChatRooms(response.data.rooms);
            } catch (error) {
                console.error('채팅방 목록을 가져오는데 실패했습니다.', error);
            } finally {
                setLoading(false);
            }
        };

        fetchUserData();
        fetchChatRooms();
    }, [userId]);

    const handleChatRoomClick = (roomId) => {
        navigate(`/chat/detail/${roomId}`);
    };

    if(loading) {
        return (
          <div className='chat-list-loading-msg'>
            <div className='chat-list-loading-inside'>
              <ClipLoader color="#dcdcdc" loading={loading} size={50} />
              <span>데이터를 불러오고 있어요!</span>
            </div>
          </div>
        )
    }

    return (
        <div className='chat-list-container'>
            <div className='chat-list-container-inside'>
                <div className='chat-list-container-title'>
                    <div>
                        <span>나의 채팅목록</span>
                    </div>
                </div>
                <div className='chat-list-screen'>
                    {chatRooms.length > 0 ? (
                        chatRooms.map(room => {
                            const otherMemberName = room.memberName.find(name => name !== username);
                            const lastMessage = room.messages.length > 0 ? room.messages[room.messages.length - 1] : { content: '메시지가 없습니다.' };
                            return (
                                <div key={room._id} onClick={() => handleChatRoomClick(room._id)} className='chat-room-item'>
                                    <div className='chat-room-info'>
                                        <div className='chat-room-names'>
                                            사용자 : {otherMemberName}
                                        </div>
                                        <div className='chat-room-last-message'>
                                            메시지 : {lastMessage.content}
                                        </div>
                                    </div>
                                </div>
                            );
                        })
                    ) : (
                        <div>진행중인 채팅이 없습니다.</div>
                    )}
                </div>
            </div>
        </div>
    )
}