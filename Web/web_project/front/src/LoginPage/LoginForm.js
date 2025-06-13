import { ReactComponent as MyLogo } from "./../EasyLogo2.svg";
import { useState, useRef, useEffect} from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';
import './LoginForm.css';

export default function LoginForm({ onLogin, isLoggedIn }) {
    const [username, setUsername] = useState('');
    const [password, setPassword] = useState('');
    const usernameInputRef = useRef(null);
    const navigate = useNavigate();

    useEffect(() => {
        if (isLoggedIn) {
          navigate('/');
        }
    }, [isLoggedIn, navigate]);

    const handleLoginSubmit = async () => {
        try {
            await axios.post('/login', { username, password });
            onLogin(true); 
            navigate('/'); //window.location.href = '/' 하면 상태변경 전 페이지를 렌더링 하게 됨(오류)
        } catch (error) {
            alert(`로그인 오류: ${error.response.data}`);
        }
    };

    const loginKeyPress = (event) => {
        if (event.key === 'Enter') {
            handleLoginSubmit();
        }
    }

    return (
        <div className='login_page_background'>
            <div className='login_form_screen'>
                <div className='login_form_screen_inside_top'>
                    <div className='top_logo'>
                        <MyLogo />
                    </div>
                    <div>
                        <span>E A S Y</span>
                    </div>
                </div>
                <div className='login_form_input'>
                    <div>
                        <span>아이디</span>
                    </div>
                    <div>
                        <input 
                          ref={usernameInputRef}
                          placeholder='아이디를 입력해주세요' 
                          name="username"
                          onChange={(e) => setUsername(e.target.value)}
                        />
                    </div>
                </div>
                <div className='login_form_input'>
                    <div>
                        <span>비밀번호</span>
                    </div>
                    <div>
                        <input 
                          type='password' 
                          placeholder='비밀번호를 입력해주세요'
                          name="password"
                          onChange={(e) => setPassword(e.target.value)}
                          onKeyDown={ loginKeyPress }
                        />
                    </div>
                </div>
                <div className='login_form_screen_inside'>
                    <button onClick={ handleLoginSubmit } type='submit' className='login_btn'>로그인</button>
                    <button onClick={ () => navigate('/register') } type='button' className='to_register_btn'>회원가입 하러가기</button>
                </div>
            </div>
        </div>
    )
}