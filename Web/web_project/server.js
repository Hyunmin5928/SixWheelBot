// server.js
const express           = require("express");
const requestIp         = require('request-ip');
const cors              = require("cors");
const crypto            = require('crypto');
const bcrypt            = require("bcrypt");
const moment            = require('moment-timezone');
const session           = require("express-session");
const passport          = require("passport");
const LocalStrategy     = require("passport-local").Strategy;
const { createServer }  = require('http');
const { Server }        = require('socket.io');
const path              = require("path");

const db = require('./database.js');  // delivery.db

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(requestIp.mw());

app.use(session({
  secret: process.env.SESSION_SECRET || 'secret',
  resave: false,
  saveUninitialized: false
}));
app.use(passport.initialize());
app.use(passport.session());

// ── Passport LocalStrategy ──────────────────────────────────────────
passport.use(new LocalStrategy({
    usernameField: 'id',
    passwordField: 'password'
  },
  async (id, password, done) => {
    try {
      // MEM_ID 로 조회
      const user = await db.get(
        "SELECT * FROM MEMBER WHERE MEM_ID = ?",
        [id]
      );
      if (!user) {
        return done(null, false, { message: '존재하지 않는 아이디입니다.' });
      }
      // MEM_PW 와 대조
      const match = await bcrypt.compare(password, user.MEM_PW);
      if (!match) {
        return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });
      }
      // 비밀번호 제외한 전체 row 반환
      delete user.MEM_PW;
      return done(null, user);
    } catch (err) {
      return done(err);
    }
  }
));

passport.serializeUser((user, done) => {
  // 세션에는 MEM_NUM만 저장
  done(null, user.MEM_NUM);
});

passport.deserializeUser(async (memNum, done) => {
  try {
    // 비밀번호 제외한 컬럼만 조회
    const user = await db.get(
      `SELECT
         MEM_NUM, MEM_ID, MEM_EMAIL, MEM_NAME,
         MEM_NICKNAME, MEM_BIRTH, MEM_GEN,
         MEM_PHONE, MEM_ZIP, MEM_ADD1, MEM_ADD2,
         MEM_JOINDATE, MEM_ADMIN
       FROM MEMBER
       WHERE MEM_NUM = ?`,
      [memNum]
    );
    done(null, user || false);
  } catch (err) {
    done(err);
  }
});

// ── 회원가입(register) ───────────────────────────────────────────────
app.post("/register", async (req, res) => {
  const {
    id, 
    password,
    name, 
    zip,
    add1, 
    add2,
    phone, 
    email
  } = req.body;

  try {
    // 이미 존재하는 아이디 또는 이메일 체크
    const exists = await db.get(
      "SELECT 1 FROM MEMBER WHERE MEM_ID = ? OR MEM_EMAIL = ?",
      [id, email]
    );
    if (exists) {
      return res.status(400).send("이미 존재하는 아이디 또는 이메일입니다.");
    }

    // 비밀번호 해시
    const hash = await bcrypt.hash(password, 10);

    // INSERT
    await db.run(
      `INSERT INTO MEMBER (
         MEM_ID, MEM_PW, MEM_NAME, MEM_ZIP, 
         MEM_ADD1, MEM_ADD2, MEM_PHONE, MEM_EMAIL,
       ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [id, hash, name, zip, add1, add2, phone, email]
    );

    res.json({ registerSuccess: true });
  } catch (e) {
    console.error(e);
    res.status(500).send("회원가입 오류");
  }
});

// ── 로그인(login) ───────────────────────────────────────────────────
app.post("/login", (req, res, next) => {
  passport.authenticate("local", (err, user, info) => {
    if (err) {
      console.error("로그인 중 서버 에러:", err);
      return res.status(500).json({ error: err.message });
    }
    if (!user) {
      // info.message에 Passport 전략에서 보낸 메시지가 들어있습니다.
      return res.status(401).json({ error: info.message });
    }
    req.logIn(user, (e) => {
      if (e) return next(e);
      res.json({ loginSuccess: true });
    });
  })(req, res, next);
});

// ── 로그아웃(logout) ─────────────────────────────────────────────────
app.get("/logout", (req, res) => {
  req.logout(() => {
    res.json({ logoutSuccess: true });
  });
});

// ── 로그인 상태 확인 ─────────────────────────────────────────────────
app.get("/checkLoggedIn", (req, res) => {
  res.json({ isLoggedIn: !!req.user });
});

// ── 로그인 사용자 정보 제공 ───────────────────────────────────────────
app.get("/checkLogin", (req, res) => {
  if (req.user) {
    const {
      MEM_NUM, MEM_ID, MEM_EMAIL,
      MEM_NAME, MEM_NICKNAME
    } = req.user;
    res.json({
      isLoggedIn: true,
      memNum: MEM_NUM,
      id: MEM_ID,
      email: MEM_EMAIL,
      name: MEM_NAME,
      nickname: MEM_NICKNAME
    });
  } else {
    res.json({ isLoggedIn: false });
  }
});

// ── 서버 및 Socket.IO 시작 ───────────────────────────────────────────
const server = createServer(app);
const io     = new Server(server, {
  cors: { origin: "http://localhost:3000", methods: ["GET","POST"] }
});
server.listen(process.env.PORT || 4000, () => {
  console.log("🚀 Node 서버 실행 중: http://localhost:4000 http://192.168.0.208:4000");
});



//--------------------------------- API 구현 ----------------------------------
// server.js 의 API 구현 직전쯤
// app.get('/', (req, res) => {
//   res.send('서버 정상 작동 중');
// });


// 인증 메일 전송
app.post('/verify-email', (req, res) => {
  const { email } = req.body;
  const code = generateRandomCode(6);
  req.session.emailCode = code;
  req.session.email = email;
  transporter.sendMail({
    from: `[E A S Y] <${process.env.EMAIL_USERNAME}>`,
    to: email,
    subject: '[E A S Y] 인증번호를 확인해주세요.',
    html: `<h1>이메일 인증</h1><div>인증번호 [${code}]를 입력해주세요.</div>`
  }, (err, info) => {
    if (err) {
      console.error(err);
      res.status(500).json({ success: false, message: '이메일 전송 실패' });
    } else {
      res.json({ success: true, message: '이메일 전송 완료' });
    }
  });
});

// 인증 코드 확인
app.post('/verify-email-code', (req, res) => {
  const { email, emailCode } = req.body;
  const ok = req.session.email === email && req.session.emailCode === emailCode;
  res.json({ success: ok, message: ok ? '인증 성공' : '인증 실패' });
});

// 이메일 중복 체크
app.post('/check-email', async (req, res) => {
  const { email } = req.body;
  try {
    const user = await db.get("SELECT * FROM user WHERE email = ?", [email]);
    res.json({ available: !user });
  } catch (e) {
    console.error('이메일 확인 오류:', e);
    res.status(500).json({ message: '서버 에러' });
  }
});

app.get('/user/:userId', async (req, res) => {
  const id = req.params.userId;
  try {
    const user = await db.get("SELECT username FROM user WHERE id = ?", [id]);
    if (user) res.json({ username: user.username });
    else res.status(404).send('사용자 없음');
  } catch (e) {
    console.error('서버 에러:', e);
    res.status(500).send('서버 에러');
  }
});

// React 앱 build 결과물 정적 서빙
const buildPath = path.resolve(__dirname, 'front', 'build');
app.use(express.static(buildPath));

// 위의 API 경로 외 모든 GET 요청에 대해 React index.html 반환
app.get('*', (req, res) => {
  res.sendFile(path.join(buildPath, 'index.html'));
});

// WebSocket 메시징 예시 유지
// io.on('connection', socket => console.log('소켓 연결'));


//------------------------------------------------------------------------------------------------------------------------------------//