// require('dotenv').config()
// const { MongoClient } = require('mongodb');

// const url = process.env.DB_URL
// const connectDB = new MongoClient(url).connect()

// module.exports = connectDB

const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const dbPath = path.resolve(__dirname, 'database.sqlite3');
const db = new sqlite3.Database(dbPath, (err) => {
  if (err) {
    console.error('SQLite 연결 실패:', err.message);
  } else {
    console.log('SQLite 연결 성공');
  }
});

module.exports = db;
