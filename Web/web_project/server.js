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

// const db = require('./database.js');  // delivery.db

const dbPromise = require('./database.js');


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
const buildPath = path.resolve(__dirname, 'front', 'build');
app.use(express.static(buildPath));

// 루트 경로로 오면 /mainpage 로 리다이렉트
app.get('/', (req, res) => {
  res.redirect('/MainPage');
});

// 그 외 모든 GET 은 React 라우터가 처리하도록 index.html 반환
app.get('*', (req, res) => {
  res.sendFile(path.join(buildPath, 'index.html'));
});
// ── Passport LocalStrategy ──────────────────────────────────────────
passport.use(new LocalStrategy({
    usernameField: 'userId',
    passwordField: 'password'
  },
  async (userId, password, done) => {
    try {
      // ① Promise<Database>에서 실제 db 인스턴스를 얻습니다.
      const db = await dbPromise;

      // ② DB에서 조회한 raw row를 row 변수에 담고,
      const row = await db.get(
        "SELECT * FROM MEMBER WHERE MEM_ID = ?",
        [userId]
      );
      if (!row) {
        return done(null, false, { message: '존재하지 않는 아이디입니다.' });
      }

      // ③ 비밀번호 검증
      const match = await bcrypt.compare(password, row.MEM_PW);
      if (!match) {
        return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });
      }

      // 세션에 저장할 최소 정보만 담은 객체 생성
      const user = {
        memNum: row.MEM_NUM,
        userId: row.MEM_ID,
        name:   row.MEM_NAME,
        email:  row.MEM_EMAIL
      };
      return done(null, user);
    } catch (err) {
      return done(err);
    }
  }
));

// ── serializeUser ───────────────────────────────────────────
passport.serializeUser((user, done) => {
  done(null, user.memNum);
});

// ── deserializeUser ───────────────────────────────────────────
passport.deserializeUser(async (memNum, done) => {
  try {
    const db = await dbPromise;
    const row = await db.get(`SELECT MEM_NUM, MEM_ID, MEM_NAME, MEM_EMAIL FROM MEMBER WHERE MEM_NUM = ?`, [memNum]);
    if (!row) return done(null, false);
    done(null, {
      memNum: row.MEM_NUM,
      userId: row.MEM_ID,
      name:   row.MEM_NAME,
      email:  row.MEM_EMAIL
    });
  } catch (err) {
    done(err);
  }
});

// ── 회원가입(register) ───────────────────────────────────────────────
app.post("/api/register", async (req, res) => {
  const {
    userId,   // MEM_ID
    password, // MEM_PW
    name,     // MEM_NAME
    zip,      // MEM_ZIP
    add1,     // MEM_ADD1
    add2,     // MEM_ADD2
    phone,    // MEM_PHONE
    email     // MEM_EMAIL
  } = req.body;

  const db = await dbPromise;

   console.log("▶ req.body:", req.body);
   const exists = await db.get(
     "SELECT 1 FROM MEMBER WHERE MEM_ID = ? OR MEM_EMAIL = ?",
     [userId, email]
   );
   console.log("▶ exists (row?):", exists);

  try {
    // 1) 중복 체크
    if (exists) {
      return res.status(400).send("이미 존재하는 아이디 또는 이메일입니다.");
    }

    // 2) 비밀번호 해시
    const hash = await bcrypt.hash(password, 10);

    // 3) INSERT
    await db.run(
      `INSERT INTO MEMBER (
         MEM_ID, MEM_PW, MEM_NAME,
         MEM_ZIP, MEM_ADD1, MEM_ADD2,
         MEM_PHONE, MEM_EMAIL
       ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [userId, hash, name, zip, add1, add2, phone, email]
    );

    // 4) 성공 응답
    res.json({ registerSuccess: true });
  } catch (e) {
    console.error("회원가입 오류:", e);
    res.status(500).send("회원가입 오류");
  }
});

// ── 로그인(login) ───────────────────────────────────────────────────
app.post("/api/login", (req, res, next) => {
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
app.get("/api/logout", (req, res) => {
  req.logout(() => {
    res.json({ logoutSuccess: true });
  });
});

// ── 로그인 상태 확인 ─────────────────────────────────────────────────
app.get("/api/checkLoggedIn", (req, res) => {
  res.json({ isLoggedIn: !!req.user });
});

// ── 로그인 사용자 정보 제공 ───────────────────────────────────────────
app.get("/api/checkLogin", (req, res) => {
  if (req.user) {
    const {
      MEM_NUM, MEM_ID, MEM_EMAIL,
      MEM_NAME, MEM_NICKNAME
    } = req.user;
    res.json({
      isLoggedIn: true,
      memNum: MEM_NUM,
      userId: MEM_ID,
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



