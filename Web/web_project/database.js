// database.js
const sqlite3 = require('sqlite3').verbose();
const path    = require('path');
const dbPath = path.resolve(__dirname, 'delivery.db');

const db     = new sqlite3.Database(dbPath, (err) => {
  if (err) {
    console.error('SQLite 연결 실패:', err.message);
    process.exit(1);
  }
  console.log('SQLite 연결 성공:', dbPath);
});

db.serialize(() => {
  // 외래키 사용을 원하면 아래 주석 해제
  // db.run('PRAGMA foreign_keys = ON;');

  // 회원 테이블
  db.run(`
    CREATE TABLE IF NOT EXISTS MEMBER (
      MEM_NUM      INTEGER PRIMARY KEY AUTOINCREMENT,
      MEM_EMAIL    TEXT    UNIQUE NOT NULL,
      MEM_PW       TEXT            NOT NULL,
      MEM_NAME     TEXT            NOT NULL,
      MEM_NICKNAME TEXT    UNIQUE NOT NULL,
      MEM_BIRTH    TEXT            NOT NULL,
      MEM_GEN      TEXT            NOT NULL,
      MEM_PHONE    TEXT            NOT NULL,
      MEM_ZIP      TEXT,
      MEM_ADD1     TEXT,
      MEM_ADD2     TEXT,
      MEM_JOINDATE DATETIME        NOT NULL DEFAULT (CURRENT_TIMESTAMP),
      MEM_ADMIN    TEXT            NOT NULL DEFAULT 'N'
    );
  `);

  // 주문(배송) 테이블
  db.run(`
    CREATE TABLE IF NOT EXISTS ORDERS (
      ORDERS_NUM     INTEGER PRIMARY KEY AUTOINCREMENT,
      ORDERS_USER    INTEGER       NOT NULL,
      ORDERS_PHONE   TEXT          NOT NULL,
      ORDERS_STATUS  TEXT          NOT NULL,
      ORDERS_DATE    DATETIME      NOT NULL DEFAULT (CURRENT_TIMESTAMP),
      ORDERS_COST    INTEGER       NOT NULL,
      ORDERS_ZIPCODE TEXT          NOT NULL,
      ORDERS_ADD1    TEXT          NOT NULL,
      ORDERS_ADD2    TEXT          NOT NULL,
      ORDERS_MEMO    TEXT,
      ORDERS_DEL     TEXT          NOT NULL DEFAULT 'N',
      RETURN_STATE   TEXT          NOT NULL DEFAULT 'N',
      RETURN_DATE    DATETIME,
      RETURN_STATUS  TEXT,
      RETURN_COST    INTEGER,
      -- 외래키 설정 (필요시 주석 해제)
      -- FOREIGN KEY(ORDERS_USER) REFERENCES MEMBER(MEM_NUM)
      CHECK(ORDERS_DEL IN ('Y','N')),
      CHECK(RETURN_STATE IN ('Y','N'))
    );
  `);

  // 배달로그 테이블
  db.run(`
    CREATE TABLE IF NOT EXISTS DELIVERY (
      DELIVERY_NUM  INTEGER PRIMARY KEY AUTOINCREMENT,
      DELIVERY_CNT  INTEGER       NOT NULL DEFAULT 0,
      SUCCESS_CNT   INTEGER       NOT NULL DEFAULT 0,
      FAIL_CNT      INTEGER       NOT NULL DEFAULT 0,
      UPDATE_DATE   DATETIME      NOT NULL DEFAULT (CURRENT_TIMESTAMP)
    );
  `);
});

module.exports = db;
