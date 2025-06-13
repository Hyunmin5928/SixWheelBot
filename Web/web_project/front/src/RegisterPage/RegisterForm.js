import { ReactComponent as MyLogo } from "./../EasyLogo2.svg";
import { useNavigate } from "react-router-dom";
import "./RegisterForm.css";
import { useEffect, useRef, useReducer, useState } from "react";
import axios from "axios";

const formReducer = (state, action) => {
  switch (action.type) {
    case "SET_EMAIL":
      return { ...state, email: action.payload, emailError: validateEmail(action.payload) };
    case "SET_EMAIL_CODE":
      return { ...state, emailCode: action.payload };
    case "SET_EMAIL_CODE_SENT":
      return { ...state, emailCodeSent: action.payload };
    case "SET_EMAIL_VERIFIED":
      return { ...state, emailVerified: action.payload };
    case "SET_USERNAME":
      return { ...state, username: action.payload };
    case "SET_PASSWORD":
      return { ...state, password: action.payload };
    case "SET_CONFIRM_PASSWORD":
      return { ...state, confirmPassword: action.payload };
    case "SET_ERRORS":
      return { ...state, ...action.payload };
    case "SET_FORM_VALID":
      return { ...state, formValid: action.payload };
    default:
      return state;
  }
};

const validateEmail = (email) => {
  if (!/^[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/i.test(email)) return "유효하지 않은 이메일 입니다.";
  return "";
};

export default function RegisterForm() {
  const navigate = useNavigate();
  const initialState = {
    email: "",
    emailCode: "",
    emailCodeSent: false,
    emailVerified: false,
    emailError: "",
    username: "",
    password: "",
    confirmPassword: "",
    usernameError: "",
    passwordError: "",
    confirmPasswordError: "",
    formValid: false,
  };
  const [state, dispatch] = useReducer(formReducer, initialState);
  const [timer, setTimer] = useState(300);
  const [timerActive, setTimerActive] = useState(false);

  const emailInputRef = useRef(null);

  useEffect(() => {
    emailInputRef.current.focus();
  }, []);

  useEffect(() => {
    const {
      username,
      password,
      confirmPassword,
      usernameError,
      passwordError,
      confirmPasswordError,
    } = state;
    const isFormValid = !(
      username === "" ||
      password === "" ||
      confirmPassword === "" ||
      usernameError ||
      passwordError ||
      confirmPasswordError
    );
    dispatch({ type: "SET_FORM_VALID", payload: isFormValid });
  }, [
    state.username,
    state.password,
    state.confirmPassword,
    state.usernameError,
    state.passwordError,
    state.confirmPasswordError,
  ]);

  useEffect(() => {
    const validateConfirmPassword = () => {
      const { password, confirmPassword } = state;
      const errors = {};
      if (password !== confirmPassword) {
        errors.confirmPasswordError = "비밀번호가 일치하지 않습니다.";
      } else {
        errors.confirmPasswordError = "";
      }
      dispatch({ type: "SET_ERRORS", payload: errors });
    };
    validateConfirmPassword();
  }, [state.password, state.confirmPassword]);

  useEffect(() => {
    let interval;
    if (timerActive) {
      interval = setInterval(() => {
        setTimer((prevTimer) => (prevTimer > 0 ? prevTimer - 1 : 0));
      }, 1000);
    } else if (!timerActive && timer !== 0) {
      clearInterval(interval);
    }
    return () => clearInterval(interval);
  }, [timerActive, timer]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    const { username, password } = state;
    try {
      const response = await axios.post("/register", { username, password });
      if (response.data.registerSuccess === true) {
        alert("회원가입에 성공하였습니다! 로그인을 해주세요.");
        navigate("/login");
      }
    } catch (error) {
      alert(error.response.data);
    }
  };

  const handleEmailChange = (event) => {
    dispatch({ type: "SET_EMAIL", payload: event.target.value });
  };

  const handleEmailCodeChange = (event) => {
    dispatch({ type: "SET_EMAIL_CODE", payload: event.target.value });
  };

  //------------------------------------------아이디 유효성 검사-------------------------------------------//
  const handleUsernameChange = (event) => {
    const value = event.target.value;
    dispatch({ type: "SET_USERNAME", payload: value });
    validateUsername(value);
  };

  const validateUsername = (value) => {
    const errors = {};
    if (value.trim() === "") {
      errors.usernameError = "아이디를 입력해주세요.";
    } else if (!(value.length >= 6 && value.length <= 12)) {
      errors.usernameError = "아이디는 6글자 이상 12글자 이하이어야 합니다.";
    } else if (!/(?=.*[A-Za-z])(?=.*\d)^[A-Za-z\d]{6,12}$/.test(value)) {
      errors.usernameError = "아이디는 영어와 숫자의 조합만 가능합니다";
    } else {
      errors.usernameError = "";
    }
    dispatch({ type: "SET_ERRORS", payload: errors });
  };
  //---------------------------------------------------------------------------------------------------//

  //------------------------------------비밀번호 유효성 검사-------------------------------------------//
  const handlePasswordChange = (event) => {
    const value = event.target.value;
    dispatch({ type: "SET_PASSWORD", payload: value });
    validatePassword(value);
  };

  const handleConfirmPasswordChange = (event) => {
    const value = event.target.value;
    dispatch({ type: "SET_CONFIRM_PASSWORD", payload: value });
  };

  const validatePassword = (value) => {
    const errors = {};
    if (value.trim() === "") {
      errors.passwordError = "비밀번호를 입력해주세요";
    } else if (value.length < 8) {
      errors.passwordError = "비밀번호의 길이는 8자 이상이어야 합니다.";
    } else if (!/(?=.*[a-zA-Z])(?=.*[0-9])(?=.*[@$!%*#?&]).{8,}/.test(value)) {
      errors.passwordError =
        "비밀번호는 알파벳, 숫자 및 특수문자(@$!%*#?&)를 하나 이상 포함해야 합니다.";
    } else {
      errors.passwordError = "";
    }
    dispatch({ type: "SET_ERRORS", payload: errors });
  };

  const verifyEmail = async () => {
    const { email } = state;

    try {
      const emailCheckResponse = await axios.post('/check-email', { email });
      if (!emailCheckResponse.data.available) {
        dispatch({ type: "SET_EMAIL_VERIFIED", payload: false });
        alert("이미 가입된 이메일입니다.");
        return;
      }

      const response = await axios.post('/verify-email', { email });
      if (response.data.success) {
        dispatch({ type: "SET_EMAIL_CODE_SENT", payload: true });
        alert("인증 이메일이 전송되었습니다.");
        setTimer(300);
        setTimerActive(true);
      } else {
        alert("이메일 전송에 실패했습니다.");
      }
    } catch (error) {
      console.error("이메일을 확인하는 동안 문제가 발생했습니다.:", error);
      dispatch({ type: "SET_EMAIL_VERIFIED", payload: false });
      alert("이메일을 확인하는 동안 문제가 발생했습니다.");
    }
  };

  const verifyEmailCode = async () => {
    const { email, emailCode } = state;

    try {
      const response = await axios.post('/verify-email-code', { email, emailCode });
      if (response.data.success) {
        dispatch({ type: "SET_EMAIL_VERIFIED", payload: true });
        alert(response.data.message);
        setTimerActive(false);
      } else {
        alert(response.data.message);
      }
    } catch (error) {
      console.error("이메일 코드 검증 중 문제가 발생했습니다.:", error);
      alert("이메일 코드 검증 중 문제가 발생했습니다.");
    }
  };

  const formatTime = (seconds) => {
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = seconds % 60;
    return `${minutes}:${remainingSeconds < 10 ? '0' : ''}${remainingSeconds}`;
  };

  //--------------------------------------------------------------------------------------------------//

  return (
    <div className="register_page_background">
      <div className="register_form_screen">
        <div className="register_form_screen_inside_top">
          <div className="top_logo">
            <MyLogo />
          </div>
          <div>
            <span>E A S Y</span>
          </div>
        </div>
        <div className="register_form_input">
          <div>
            <span>이메일</span>
          </div>
          <div className="email-input-div">
            <div className={state.emailError ? "email-input-error" : "email-input-normal"}>
              <input
                className='email-input'
                ref={emailInputRef}
                value={state.email}
                onChange={handleEmailChange}
                type="email"
                placeholder="이메일 주소를 입력하세요"
                required
                disabled={state.emailVerified}
              />
            </div>
            <div className='rand_num_send_div'>
              <button className='rand_num_send_btn' onClick={ verifyEmail } disabled={!state.email || state.emailError || state.emailVerified}>
                인증메일 전송
              </button>
            </div>
          </div>
          {state.emailError && <div className="warn_msg">{state.emailError}</div>}
        </div>
        {state.emailCodeSent && !state.emailVerified && (
          <div className="register_form_input">
            <div className='code-check-space'>
              <div className='code-input-div'>
                <input
                  className='code-input'
                  value={state.emailCode}
                  onChange={handleEmailCodeChange}
                  placeholder="인증번호를 입력하세요"
                  type="text"
                  required
                  disabled={state.emailVerified}
                />
              </div>
              <div className='code-check-div'>
                <button className='code-check-btn' onClick={ verifyEmailCode } disabled={timer === 0}>
                  인증번호 확인
                </button>
              </div>
            </div>
            {timer > 0 && (
              <div className="email-timer">
                <span>남은 시간: {formatTime(timer)}</span>
              </div>
            )}
            {timer === 0 && (
              <div className="warn_msg">
                <span>인증 시간이 만료되었습니다. 다시 시도해주세요.</span>
              </div>
            )}
          </div>
        )}
        <div className="register_form_input">
          <div>
            <span>아이디</span>
          </div>
          <div className={state.usernameError ? "input-error" : ""}>
            <input
              value={state.username}
              onChange={handleUsernameChange}
              placeholder="아이디를 입력해주세요"
            />
          </div>
          {state.usernameError && (
            <div className="warn_msg">
              <span>{state.usernameError}</span>
            </div>
          )}
        </div>
        <div className="register_form_input">
          <div>
            <span>비밀번호</span>
          </div>
          <div className={state.passwordError ? "input-error" : ""}>
            <input
              value={state.password}
              onChange={handlePasswordChange}
              type="password"
              placeholder="비밀번호를 입력해주세요"
              required
            />
          </div>
          {state.passwordError && (
            <div className="warn_msg">
              <span>{state.passwordError}</span>
            </div>
          )}
        </div>
        <div className="register_form_input">
          <div>
            <span>비밀번호 확인</span>
          </div>
          <div className={state.confirmPasswordError ? "input-error" : ""}>
            <input
              value={state.confirmPassword}
              onChange={handleConfirmPasswordChange}
              type="password"
              placeholder="비밀번호를 입력해주세요"
              required
            />
          </div>
          {state.confirmPasswordError && (
            <div className="warn_msg">
              <span>{state.confirmPasswordError}</span>
            </div>
          )}
        </div>
        <div className="register_form_screen_inside">
          <button
            type="submit"
            className={state.formValid && state.emailVerified ? "register_btn" : "disabled_btn"}
            disabled={!state.formValid || !state.emailVerified}
            onClick={handleSubmit}
          >
            가입하기
          </button>
        </div>
      </div>
    </div>
  );
}
