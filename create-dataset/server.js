const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;
const DATASET_ROOT = path.join(__dirname, 'dataset');

API_KEY="EYZFUEKJFAOUIIDQBUSS"

// Middleware
app.use(express.json({ limit: '10mb' }));
app.use(express.static(__dirname));

// Ensure dataset directories exist
for (let i = 0; i < 10; i++) {
    const dir = path.join(DATASET_ROOT, i.toString());
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
}

// Get image count endpoint
app.get('/image-count', (req, res) => {
    let totalCount = 0;
    const countByDigit = {};

    for (let i = 0; i < 10; i++) {
        const dir = path.join(DATASET_ROOT, i.toString());
        if (fs.existsSync(dir)) {
            const files = fs.readdirSync(dir).filter(file => file.endsWith('.png'));
            countByDigit[i] = files.length;
            totalCount += files.length;
        } else {
            countByDigit[i] = 0;
        }
    }

    res.json({ success: true, total: totalCount, byDigit: countByDigit });
});

// Save image endpoint
app.post('/save-image', (req, res) => {
    const { digit, imageData } = req.body;

    if (digit === undefined || !imageData) {
        return res.status(400).json({ success: false, error: 'Missing digit or imageData' });
    }

    const digitNum = parseInt(digit, 10);
    if (Number.isNaN(digitNum) || digitNum < 0 || digitNum > 9) {
        return res.status(400).json({ success: false, error: 'Digit must be an integer between 0 and 9' });
    }

    // Generate unique ID based on timestamp
    const timestamp = Date.now();
    const filename = `${timestamp}_${digitNum}.png`;
    const unsafeRelativePath = path.join(digitNum.toString(), filename);
    const filepath = path.resolve(DATASET_ROOT, unsafeRelativePath);

    // Ensure the resolved path is within the dataset root
    if (!filepath.startsWith(DATASET_ROOT + path.sep)) {
        return res.status(400).json({ success: false, error: 'Invalid file path' });
    }

    // Remove base64 prefix if present
    const base64Data = imageData.replace(/^data:image\/png;base64,/, '');

    // Save file
    fs.writeFile(filepath, base64Data, 'base64', (err) => {
        if (err) {
            console.error('Error saving file:', err);
            return res.status(500).json({ success: false, error: 'Failed to save file' });
        }

        console.log(`✓ Saved: dataset/${digitNum}/${filename}`);
        res.json({ success: true, filename, path: `dataset/${digitNum}/${filename}` });
    });
});

app.listen(PORT, () => {
    console.log(`
========================================
  Digit Dataset Creator Server
========================================
  Server running at: http://localhost:${PORT}

  Instructions:
  1. Open http://localhost:${PORT}/
  2. Select digit (0-9)
  3. Draw the digit
  4. Click "Save to Dataset"

  Images saved to: dataset/[digit]/[timestamp]_[digit].png
========================================
    `);
});
