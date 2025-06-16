const express = require("express");
const requestIp = require('request-ip');
const cors = require("cors");
const path = require("path");
const crypto = require('crypto');
const bcrypt = require("bcrypt");
const moment = require('moment-timezone');
const sqlite3 = require('sqlite3').verbose();
const { open } = require('sqlite');
const db = require('./database.js');
const session = require("express-session");
const passport = require("passport");
const LocalStrategy = require("passport-local");


const app = express();
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(requestIp.mw());

// 세션 및 Passport 설정
app.use(session({ secret: process.env.SESSION_SECRET || 'secret', resave: false, saveUninitialized: false }));
app.use(passport.initialize());
app.use(passport.session());

passport.use(new LocalStrategy(async (username, password, done) => {
  try {
    const user = await db.get("SELECT * FROM user WHERE username = ?", [username]);
    if (!user) return done(null, false, { message: '존재하지 않는 아이디입니다.' });
    if (await bcrypt.compare(password, user.password)) return done(null, user);
    return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });
  } catch (err) {
    return done(err);
  }
}));
passport.serializeUser((user, done) => done(null, { id: user.id, username: user.username }));
passport.deserializeUser(async (user, done) => {
  try {
    const row = await db.get("SELECT * FROM user WHERE id = ?", [user.id]);
    if (row) delete row.password;
    done(null, row);
  } catch (err) {
    done(err);
  }
});

// 서버 및 WebSocket 시작
// const server = createServer(app);
// const io = new server(server, { cors: { origin: "http://localhost:3000", methods: ["GET","POST"] } });
// server.listen(process.env.PORT||4000, () => console.log("서버 실행 중"));
const { createServer } = require('http');
const { Server }       = require('socket.io');
// … app 설정 위에 …

const server = createServer(app);
const io     = new Server(server, {
  cors: { origin: "http://localhost:3000", methods: ["GET","POST"] }
});

server.listen(process.env.PORT || 4000, () => {
  console.log("🚀 Node 서버 실행 중: http://localhost:4000");
});



//--------------------------------- API 구현 ----------------------------------
// server.js 의 API 구현 직전쯤
// app.get('/', (req, res) => {
//   res.send('서버 정상 작동 중');
// });

// 회원가입
app.post("/register", async (req, res) => {
  const { username, password } = req.body;
  try {
    const exists = await db.get("SELECT * FROM user WHERE username = ?", [username]);
    if (exists) return res.status(400).send("이미 존재하는 아이디입니다.");
    const hash = await bcrypt.hash(password, 10);
    await db.run("INSERT INTO user (username,password) VALUES(?,?)", [username, hash]);
    res.json({ registerSuccess: true });
  } catch (e) { console.error(e); res.status(500).send("회원가입 오류"); }
});

// 로그인
app.post("/login", (req, res, next) => {
  passport.authenticate("local", (err, user, info) => {
    if (err) return res.status(500).json(err);
    if (!user) return res.status(401).json(info);
    req.logIn(user, (e) => { if (e) return next(e); res.json({ loginSuccess: true }); });
  })(req, res, next);
});

// 로그아웃
app.get("/logout", (req, res) => { req.logout(() => res.json({ logoutSuccess: true })); });

// 로그인 상태 확인
app.get("/checkLoggedIn", (req, res) => { res.json({ isLoggedIn: !!req.user }); });

// 로그인 정보 제공
app.get("/checkLogin", (req, res) => {
  if (req.user) res.json({ isLoggedIn: true, id: req.user.id, username: req.user.username });
  else res.json({ isLoggedIn: false });
});

// 마이페이지: 작성자 ID 조회
app.get("/userAuthorID", async (req,res) => {
  try {
    const user = await db.get("SELECT id FROM user WHERE username = ?", [req.query.username]);
    if (user) res.send(user.id.toString());
    else res.status(404).send("사용자를 찾을 수 없습니다.");
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 마이페이지: 내 배송 목록


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