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
      const row = await db.get(`
       SELECT MEM_NUM, MEM_ID, MEM_PW, MEM_NAME, MEM_EMAIL, MEM_ADMIN
       FROM   MEMBER
       WHERE  MEM_ID = ?`, [userId]);
      if (!row)   return done(null, false, { message: '존재하지 않는 아이디입니다.' });

      const match = await bcrypt.compare(password, row.MEM_PW);
      if (!match) return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });

      return done(null, {
        memNum: row.MEM_NUM,
        userId: row.MEM_ID,
        name:   row.MEM_NAME,
        email:  row.MEM_EMAIL,
        role:   row.MEM_ADMIN === 'Y' ? 'ADMIN' : 'USER'
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
    const row = await db.get(`
     SELECT MEM_NUM, MEM_ID, MEM_NAME, MEM_EMAIL, MEM_ADMIN
     FROM   MEMBER
     WHERE  MEM_NUM = ?`, [memNum]);
    if (!row) return done(null, false);
    done(null, {
      memNum: row.MEM_NUM,
      userId: row.MEM_ID,
      name:   row.MEM_NAME,
      email:  row.MEM_EMAIL,
      role:   row.MEM_ADMIN === 'Y' ? 'ADMIN' : 'USER'
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
app.post('/api/login', (req, res, next) => {
  passport.authenticate('local', (err, user, info) => {
    if (err)   return res.status(500).json({ error: err.message });
    if (!user) return res.status(401).json({ error: info.message });

    req.logIn(user, e => {
      if (e) return next(e);

      // ★ 200 OK + role 포함
      res.json({
        loginSuccess: true,
        userId: user.userId,
        role:   user.role          // 'ADMIN' or 'USER'
      });
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

// ── 회원 프로필 조회 ──────────────────────────────────
//    GET /api/user/abc → { userId:"abc", password:"(hash)", name:"", ... }
app.get('/api/user/:id', async (req, res) => {
  try {
    const db  = await dbPromise;
    const row = await db.get(
      `SELECT
         MEM_ID   AS userId,
         MEM_PW   AS password,
         MEM_NAME AS name,
         MEM_ZIP  AS zip,
         MEM_ADD1 AS address,
         MEM_ADD2 AS detail,
         MEM_PHONE AS phone,
         MEM_EMAIL AS email
       FROM MEMBER
       WHERE MEM_ID = ?`,
      [req.params.id]
    );
    if (!row) return res.status(404).send('해당 회원이 없습니다.');
    res.json(row);                 // 그대로 프런트에 전달
  } catch (err) {
    console.error('프로필 조회 오류:', err);
    res.status(500).send('서버 오류');
  }
});

// ── 회원 프로필 수정 (PUT) ───────────────────────────
app.put('/api/user/:id', async (req, res) => {
  const userId = req.params.id;
  const {
    password, name, zip, address, detail, phone, email
  } = req.body;

  try {
    const db = await dbPromise;

    /* ① 비밀번호는 해시가 필요할 수 있으므로 분기 */
    let hash = null;
    if (password) {
      hash = await bcrypt.hash(password, 10);
    }

    /* ② UPDATE 구문 — 비밀번호 포함/미포함 둘 다 처리 */
    const { changes } = await db.run(
      `UPDATE MEMBER
          SET MEM_PW   = COALESCE(?, MEM_PW),
              MEM_NAME = ?,
              MEM_ZIP  = ?,
              MEM_ADD1 = ?,
              MEM_ADD2 = ?,
              MEM_PHONE= ?,
              MEM_EMAIL= ?
        WHERE MEM_ID   = ?`,
      [hash, name, zip, address, detail, phone, email, userId]
    );

    if (changes === 0) return res.status(404).send('해당 회원이 없습니다.');
    res.json({ updateSuccess: true });
  } catch (err) {
    console.error('프로필 수정 오류:', err);
    res.status(500).send('서버 오류');
  }
});

/* ---------- 배송 신청 ---------- */
app.post('/api/order', async (req, res) => {
  const { userId, receiver, itemType } = req.body;      // ★ itemType 포함
  if (!userId || !receiver?.address || !itemType) return res.status(400).send('값 누락');

  try {
    const db = await dbPromise;
    const { lastID } = await db.run(
      `INSERT INTO ORDER_REQ (MEM_ID, REC_ADDR, REC_DETAIL, ITEM_TYPE)
       VALUES (?, ?, ?, ?)`,
      [userId, receiver.address, receiver.detail, itemType]
    );
    res.json({ id: lastID, ok: true });
  } catch (e) {
    console.error('배송 신청 오류', e);
    res.status(500).send('서버 오류');
  }
});

/* ---------- 배송 대기 목록 ---------- */
app.get('/api/order', async (_req, res) => {
  const db = await dbPromise;
  const rows = await db.all(
    `SELECT ORD_ID as id, MEM_ID as userId,
            (REC_ADDR || ' ' || IFNULL(REC_DETAIL, '')) AS address,
            ITEM_TYPE as itemType, STATUS as status
       FROM ORDER_REQ
      WHERE STATUS = 'PENDING'
      ORDER BY REQ_TIME ASC`
  );
  res.json(rows);
});

/* ---------- 반품 신청 / 목록 ---------- */
app.post('/api/return', async (req, res) => {
  const { userId, sender, itemType } = req.body;
  if (!userId || !sender?.address || !itemType) return res.status(400).send('값 누락');

  try {
    const db = await dbPromise;
    const { lastID } = await db.run(
      `INSERT INTO RETURN_REQ (MEM_ID, SEND_ADDR, SEND_DETAIL, ITEM_TYPE)
       VALUES (?, ?, ?, ?)`,
      [userId, sender.address, sender.detail, itemType]
    );
    res.json({ id: lastID, ok: true });
  } catch (e) {
    console.error('반품 신청 오류', e);
    res.status(500).send('서버 오류');
  }
});

app.get('/api/return', async (_req, res) => {
  const db = await dbPromise;
  const rows = await db.all(
    `SELECT RET_ID as id, MEM_ID as userId,
            SEND_ADDR as address, SEND_DETAIL as detail,
            ITEM_TYPE as itemType, STATUS as status
       FROM RETURN_REQ
      WHERE STATUS = 'PENDING'
      ORDER BY REQ_TIME ASC`
  );
  res.json(rows);
});

/* ── 배송 요청 수락 ─────────────────────── */
app.post('/api/order/:id/accept', async (req, res) => {
  try {
    const db = await dbPromise;
    const { changes } = await db.run(
      `UPDATE ORDER_REQ SET STATUS='ACCEPTED' WHERE ORD_ID = ?`,
      [req.params.id]
    );
    if (changes === 0) return res.status(404).send('해당 주문이 없습니다.');
    //주문 정보 조회
    const order = await db.get(`
    SELECT REC_ADDR, REC_DETAIL FROM ORDER_REQ WHERE ORD_ID = ?
    `, [req.params.id]);
    const TMAP_API_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';
    //주소-> 좌표 API 호출

    // 좌표 -> 경로 API 호출
    const address = order.REC_ADDR + ' ' + (order.REC_DETAIL || '');
    const [city_do, gu_gun, dong, bunji, ...detail] = address.split(' ');
    const detailAddress = detail.join(' ');  // 나머지는 상세주소로
 
    const geoRes = await axios.get('https://apis.openapi.sk.com/tmap/geo/geocoding', {
    params: {
      version: 1,
      city_do: city_do,
      gu_gun: gu_gun,
      dong: dong,
      bunji: bunji,
      detailAddress: detailAddress,
      addressFlag: 'F00',       // 지번/도로명 자동
      coordType: 'WGS84GEO',
      appKey: TMAP_API_KEY
    }
  });


    const coord = geoRes.data.addressInfo.coordinate;
    const startX = 127.1090;
    const startY = 37.3397;
    const endX = coord.lon;
    const endY = coord.lat;

    // 디버깅용 콘솔 출력
    console.log('==== Geocoding 결과 ====');
    console.log('coord:', coord);
    console.log('endX(경도):', coord.lon);
    console.log('endY(위도):', coord.lat);

    // 2. T map API 호출 
    
    const tmapRes = await axios.post(
      'https://apis.openapi.sk.com/tmap/routes/pedestrian?version=1',
      {
        startX, startY, endX, endY,
        reqCoordType: "WGS84GEO",
        resCoordType: "WGS84GEO",
        // ...필요한 옵션 추가
      },
      {
        headers: {
          appKey: TMAP_API_KEY,
          "Content-Type": "application/json",
        }
      }
    );

    // 1. 결과에서 features 배열 뽑기
    const features = tmapRes.data.features;

    // 2. 좌표 추출
    const coordMap = {}; // { 'lat,lon' : turnType }
    features.forEach(feature => {
      const { geometry, properties } = feature;
      if (geometry.type === "LineString") {
        geometry.coordinates.forEach(([lon, lat]) => {
          const key = `${lat},${lon}`;
          if (!(key in coordMap)) {
            coordMap[key] = null; // 기본값
          }
        });
      } else if (geometry.type === "Point") {
        const [lon, lat] = geometry.coordinates;
        const turnType = properties.turnType;
        const key = `${lat},${lon}`;
        coordMap[key] = turnType; // turnType 등록
      }
    });

    // 3. (lat, lon, turnType) 배열 만들기
    const routeCoords = Object.entries(coordMap).map(([key, turnType]) => {
      const [lat, lon] = key.split(',').map(Number);
      return [lat, lon, turnType];
    });

    // 디버깅용 터미널 출력
    console.log('==== 경로 좌표 배열 (lat, lon, turnType) ====');
    routeCoords.forEach((item, idx) => {
      const [lat, lon, turnType] = item;
      console.log(`${idx + 1}: lat=${lat}, lon=${lon}, turnType=${turnType === null ? '없음' : turnType}`);
    });

    const fs = require('fs');

    // 파일로 저장 (예: route.txt)
    fs.writeFileSync(
      'route.txt',
      routeCoords.map(([lat, lon, turnType]) =>
        `${lat},${lon},${turnType === null ? '' : turnType}`
      ).join('\n'),
      { encoding: 'utf8' }
    );


    res.json({ ok: true });
  } catch (e) {
    console.error('order accept 오류', e);
    res.status(500).send('서버 오류');
  }
});

/* ── 반품 요청 수락 ─────────────────────── */
app.post('/api/return/:id/accept', async (req, res) => {
  try {
    const db = await dbPromise;
    const { changes } = await db.run(
      `UPDATE RETURN_REQ SET STATUS='ACCEPTED' WHERE RET_ID = ?`,
      [req.params.id]
    );
    if (changes === 0) return res.status(404).send('해당 반품이 없습니다.');
    res.json({ ok: true });
  } catch (e) {
    console.error('return accept 오류', e);
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
