const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;

// Middleware
app.use(express.json({ limit: '10mb' }));
app.use(express.static(__dirname));

// Ensure dataset directories exist
for (let i = 0; i < 10; i++) {
    const dir = path.join(__dirname, 'dataset', i.toString());
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
}

// Get image count endpoint
app.get('/image-count', (req, res) => {
    let totalCount = 0;
    const countByDigit = {};

    for (let i = 0; i < 10; i++) {
        const dir = path.join(__dirname, 'dataset', i.toString());
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

    if (digit < 0 || digit > 9) {
        return res.status(400).json({ success: false, error: 'Digit must be 0-9' });
    }

    // Generate unique ID based on timestamp
    const timestamp = Date.now();
    const filename = `${timestamp}.png`;
    const filepath = path.join(__dirname, 'dataset', digit.toString(), filename);

    // Remove base64 prefix if present
    const base64Data = imageData.replace(/^data:image\/png;base64,/, '');

    // Save file
    fs.writeFile(filepath, base64Data, 'base64', (err) => {
        if (err) {
            console.error('Error saving file:', err);
            return res.status(500).json({ success: false, error: 'Failed to save file' });
        }

        console.log(`✓ Saved: dataset/${digit}/${filename}`);
        res.json({ success: true, filename, path: `dataset/${digit}/${filename}` });
    });
});

app.listen(PORT, () => {
    console.log(`
========================================
  Digit Dataset Creator Server
========================================
  Server running at: http://localhost:${PORT}

  Instructions:
  1. Open http://localhost:${PORT}/draw.html
  2. Select digit (0-9)
  3. Draw the digit
  4. Click "Save to Dataset"

  Images saved to: dataset/[digit]/[timestamp].png
========================================
    `);
});
