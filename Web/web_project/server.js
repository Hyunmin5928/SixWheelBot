// server.js
const express           = require("express");
const requestIp         = require('request-ip');
const cors              = require("cors");
const bcrypt            = require("bcrypt");
const session           = require("express-session");
const passport          = require("passport");
const LocalStrategy     = require("passport-local").Strategy;
const { createServer }  = require('http');
const { Server }        = require('socket.io');
const path              = require("path");

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

// React build된 파일 서빙
const buildPath = path.resolve(__dirname, 'front', 'build');
app.use(express.static(buildPath));

// ── Passport LocalStrategy ────────────────────────────
passport.use(new LocalStrategy({
    usernameField: 'userId',
    passwordField: 'password'
  },
  async (userId, password, done) => {
    try {
      const db  = await dbPromise;
      const row = await db.get(
        "SELECT * FROM MEMBER WHERE MEM_ID = ?",
        [userId]
      );
      if (!row)   return done(null, false, { message: '존재하지 않는 아이디입니다.' });

      const match = await bcrypt.compare(password, row.MEM_PW);
      if (!match) return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });

      return done(null, {
        memNum: row.MEM_NUM,
        userId: row.MEM_ID,
        name:   row.MEM_NAME,
        email:  row.MEM_EMAIL
      });
    } catch (err) {
      done(err);
    }
  }
));
passport.serializeUser((user, done) => done(null, user.memNum));
passport.deserializeUser(async (memNum, done) => {
  try {
    const db  = await dbPromise;
    const row = await db.get(
      "SELECT MEM_NUM, MEM_ID, MEM_NAME, MEM_EMAIL FROM MEMBER WHERE MEM_NUM = ?",
      [memNum]
    );
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

// ── 회원가입 ─────────────────────────────────────────────
app.post("/api/register", async (req, res) => {
  const { userId, password, name, zip, add1, add2, phone, email } = req.body;
  try {
    const db  = await dbPromise;
    const dup = await db.get(
      "SELECT 1 FROM MEMBER WHERE MEM_ID = ? OR MEM_EMAIL = ?",
      [userId, email]
    );
    if (dup) return res.status(400).send('이미 존재하는 아이디 또는 이메일입니다.');

    const hash = await bcrypt.hash(password, 10);
    await db.run(
      `INSERT INTO MEMBER
         (MEM_ID, MEM_PW, MEM_NAME, MEM_ZIP, MEM_ADD1, MEM_ADD2, MEM_PHONE, MEM_EMAIL)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [userId, hash, name, zip, add1, add2, phone, email]
    );
    res.json({ registerSuccess: true });
  } catch (e) {
    console.error('회원가입 오류:', e);
    res.status(500).send('회원가입 오류');
  }
});

// ── 로그인 ─────────────────────────────────────────────────
app.post("/api/login", (req, res, next) => {
  passport.authenticate("local", (err, user, info) => {
    if (err)   return res.status(500).json({ error: err.message });
    if (!user) return res.status(401).json({ error: info.message });
    req.logIn(user, e => {
      if (e) return next(e);
      res.json({ loginSuccess: true });
    });
  })(req, res, next);
});

// ── 로그아웃 ───────────────────────────────────────────────
app.get("/api/logout", (req, res) => {
  req.logout(() => res.json({ logoutSuccess: true }));
});

// ── 로그인 상태 확인 ───────────────────────────────────────
app.get("/api/checkLoggedIn", (req, res) => {
  res.json({ isLoggedIn: !!req.user });
});

// ── 아이디 찾기 ───────────────────────────────────────────
app.post('/api/find-id', async (req, res) => {
  const { name, email } = req.body;
  try {
    const db  = await dbPromise;
    const row = await db.get(
      "SELECT MEM_ID FROM MEMBER WHERE MEM_NAME = ? AND MEM_EMAIL = ?",
      [name, email]
    );
    if (!row) return res.status(404).json({ message: '일치하는 사용자가 없습니다.' });
    res.json({ userId: row.MEM_ID });
  } catch (err) {
    console.error('아이디 찾기 오류:', err);
    res.status(500).send('서버 오류');
  }
});

// ── 비밀번호 찾기 (존재 확인) ─────────────────────────────
app.post('/api/find-pw', async (req, res) => {
  const { userId, name, email } = req.body;
  try {
    const db  = await dbPromise;
    const row = await db.get(
      `SELECT 1
         FROM MEMBER
        WHERE MEM_ID    = ?
          AND MEM_NAME  = ?
          AND MEM_EMAIL = ?`,
      [userId, name, email]
    );
    if (!row) return res.status(404).send();       // 404: 정보 불일치
    return res.json({ ok: true });                 // 200 + { ok:true }
  } catch (err) {
    console.error('비밀번호 찾기 오류:', err);
    res.status(500).send('서버 오류');
  }
});

// ── 비밀번호 재설정 ────────────────────────────────────
app.post('/api/reset-pw', async (req, res) => {
  const { userId, newPassword } = req.body;
  if (!userId || !newPassword) return res.status(400).send('필수값 누락');
  try {
    const db   = await dbPromise;
    const hash = await bcrypt.hash(newPassword, 10);
    const { changes } = await db.run(
      "UPDATE MEMBER SET MEM_PW = ? WHERE MEM_ID = ?",
      [hash, userId]
    );
    if (changes === 0) return res.status(404).send('해당 아이디가 없습니다.');
    res.json({ resetSuccess: true });
  } catch (err) {
    console.error('비밀번호 재설정 오류:', err);
    res.status(500).send('서버 오류');
  }
});

// ── React SPA 라우팅 ───────────────────────────────────
app.get('/', (_req, res) => res.redirect('/'));
app.get('*', (_req, res) => res.sendFile(path.join(buildPath, 'index.html')));

// ── 서버 시작 ───────────────────────────────────────────
const server = createServer(app);
new Server(server, {
  cors: { origin: "http://localhost:3000", methods: ["GET","POST"] }
});
server.listen(process.env.PORT || 4000, () =>
  console.log("🚀 서버 실행 중: http://localhost:4000")
);
