import './Chat.css';
import ClipLoader from "react-spinners/ClipLoader";
import { io } from 'socket.io-client';
import { useParams } from 'react-router-dom';
import { useEffect, useState, useCallback, useRef } from 'react';
import axios from 'axios';

export default function Chat() { 
    const { roomId } = useParams();
    const [socket, setSocket] = useState(null);
    const [inputMsg, setInputMsg] = useState('');
    const [userId, setUserId] = useState(null);
    const [username, setUsername] = useState(null);
    const [messages, setMessages] = useState([]);
    const chatScreenRef = useRef(null);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        const fetchUser = async () => {
            try {
                setLoading(true);
                const response = await axios.get('/checkLogin');
                setUserId(response.data.userId);
                setUsername(response.data.username);
            } catch (error) {
                console.error('사용자 정보를 가져오는데 실패했습니다.', error);
            } finally {
                setLoading(false);
            }
        }
        fetchUser();

        const newSocket = io('http://localhost:8080');
        setSocket(newSocket);
        
        newSocket.emit("req_join", { roomId: roomId });

        newSocket.on('load_messages', (msgs) => {
            setMessages(msgs);
        });

        newSocket.on('message_broadcast', (newMessage) => {
            setMessages((prevMessages) => [...prevMessages, newMessage]);
        });

        return () => {
            newSocket.disconnect();
        };
    }, [roomId]);

    useEffect(() => {
        if (chatScreenRef.current) {
            chatScreenRef.current.scrollTop = chatScreenRef.current.scrollHeight;
        }
    }, [messages]);

    const sendMsg = useCallback(() => {
        if (socket && inputMsg.trim() !== '') {
            socket.emit('send_message', { msg: inputMsg, roomId: roomId, userId: userId, username: username });
            setInputMsg(''); // 메시지 전송 후 입력란 초기화
        }
    }, [socket, inputMsg, roomId, userId, username]);

    const handleInputMsg = (event) => {
        setInputMsg(event.target.value);
    }

    const handleKeyPress = (event) => {
        if (event.key === 'Enter') {
            sendMsg();
        }
    }

    if(loading) {
        return (
          <div className='chat-loading-msg'>
            <div className='chat-loading-inside'>
              <ClipLoader color="#dcdcdc" loading={loading} size={50} />
              <span>데이터를 불러오고 있어요!</span>
            </div>
          </div>
        )
    }

    return (
        <div className='chat-container'>
            <div className='chat-container-inside'>
                <div className='chat-container-title'>
                    <div>
                        <span>채팅 페이지</span>
                    </div>
                </div>
                <div className='chat-screen' ref={chatScreenRef}>
                     {messages.map((message, index) => (
                        <div key={index} className={`message ${message.userId === userId ? 'right' : 'left'}`}>
                                <div className='message-content'>
                                    <strong>{message.username}</strong>
                                    <div>{message.content}</div>
                                </div>
                        </div>
                     ))}
                 </div>  
            </div>
            <div className='input-and-send'>
                <input 
                    className='chat-input-space' 
                    value={ inputMsg } 
                    onChange={ handleInputMsg } 
                    onKeyDown={ handleKeyPress } 
                />
                <button className='chat-msg-send' onClick={ sendMsg }>전송</button>
            </div>
        </div>
    )
}