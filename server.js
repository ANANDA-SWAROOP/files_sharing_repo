const express = require('express');
const { Pool } = require('pg');
const { MongoClient } = require('mongodb');
const cors = require('cors');
require('dotenv').config();

const app = express();
app.use(cors());
app.use(express.json());

// PostgreSQL connection
const pgPool = new Pool({
  connectionString: process.env.DATABASE_URL
});

// MongoDB connection
let mongoDb;
MongoClient.connect(process.env.MONGODB_URI)
  .then(client => {
    mongoDb = client.db('cricket_ranker');
    console.log('Connected to MongoDB');
  });

// Example endpoint: Get player with metadata
app.get('/api/players/:id', async (req, res) => {
  try {
    const { id } = req.params;
    
    // Get player from PostgreSQL
    const playerResult = await pgPool.query(
      'SELECT * FROM players WHERE id = $1', [id]
    );
    
    // Get metadata from MongoDB
    const metadata = await mongoDb.collection('player_meta')
      .findOne({ player_id: parseInt(id) });
    
    res.json({
      ...playerResult.rows[0],
      ...metadata
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.listen(3001, () => {
  console.log('Backend running on http://localhost:3001');
});
