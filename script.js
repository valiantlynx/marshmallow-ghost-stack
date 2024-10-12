const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const scoreDisplay = document.getElementById('score');
let score = 0;
let marshmallows = [];
let globalSpeedMultiplier = 1.0;

const clickSound = new Audio('click-sound.mp3'); // Load sound files properly
const burnSound = new Audio('burn-sound.mp3');

// Marshmallow object with progress bar
class Marshmallow {
    constructor(x, y, roastingSpeed) {
        this.x = x;
        this.y = y;
        this.size = 40;
        this.roastingSpeed = roastingSpeed;
        this.stage = 0;  // 0: White, 1: Yellow, 2: Brown, 3: Black
        this.timer = 0;  // Internal timer for stage progression
        this.isClicked = false;
    }

    draw() {
        if (this.stage === 0) ctx.fillStyle = '#FFF';    // White
        if (this.stage === 1) ctx.fillStyle = '#FFD700'; // Yellow
        if (this.stage === 2) ctx.fillStyle = '#8B4513'; // Brown
        if (this.stage === 3) ctx.fillStyle = '#000';    // Black

        // Draw marshmallow (circle)
        ctx.beginPath();
        ctx.arc(this.x, this.y, this.size / 2, 0, Math.PI * 2);
        ctx.fill();

        // Draw a face for fun
        ctx.fillStyle = '#000';
        ctx.beginPath();
        ctx.arc(this.x - 8, this.y - 10, 3, 0, Math.PI * 2); // Left eye
        ctx.arc(this.x + 8, this.y - 10, 3, 0, Math.PI * 2); // Right eye
        ctx.fill();
        ctx.beginPath();
        ctx.arc(this.x, this.y + 5, 4, 0, Math.PI);          // Mouth
        ctx.fill();
    }

    roast() {
        // Increment roasting timer based on roastingSpeed and globalSpeedMultiplier
        this.timer += this.roastingSpeed * globalSpeedMultiplier;

        // Progress through roasting stages based on timer
        if (this.timer > 100 && this.timer <= 200) {
            this.stage = 1; // Yellow
        } else if (this.timer > 200 && this.timer <= 300) {
            this.stage = 2; // Brown
        } else if (this.timer > 300) {
            this.stage = 3; // Black (burnt)
        }
    }

    clicked() {
        if (!this.isClicked && this.stage >= 1 && this.stage <= 3) {
            this.isClicked = true;
            clickSound.play(); // Play click sound when marshmallow is clicked
            const points = this.getPoints();
            this.reset();
            return points;
        }
        return 0;
    }

    getPoints() {
        if (this.stage === 2) return 5;  // Brown (perfect roast)
        if (this.stage === 1) return 1;  // Yellow (undercooked)
        if (this.stage === 3) {
            burnSound.play(); // Play burn sound when the marshmallow is burnt
            return -2;        // Burnt (lose points)
        }
        return 0; // White (unroasted)
    }

    reset() {
        this.stage = 0;  // Reset to unroasted
        this.timer = 0;  // Reset timer
        this.isClicked = false;  // Make it ready to roast again
    }
}

// Initialize marshmallows in a grid-like structure
function createMarshmallows() {
    const rows = 5;       // 5 floors of marshmallows
    const margin = 80;    // Space between marshmallows
    const roastingSpeeds = [0.05, 0.07, 0.09, 0.11];  // Different roasting speeds

    for (let i = 0; i < rows; i++) {
        for (let j = 0; j < 2; j++) {
            const x = (canvas.width / 4) + j * (canvas.width / 2);
            const y = margin + i * margin * 1.5;
            marshmallows.push(new Marshmallow(x, y, roastingSpeeds[i % roastingSpeeds.length]));
        }
    }
}

// Detect clicks on marshmallows
canvas.addEventListener('click', (e) => {
    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    marshmallows.forEach(marshmallow => {
        const dist = Math.sqrt((mouseX - marshmallow.x) ** 2 + (mouseY - marshmallow.y) ** 2);
        if (dist < marshmallow.size / 2) {  // Check if click is within the marshmallow's radius
            const points = marshmallow.clicked();
            score += points;
            scoreDisplay.textContent = score;
        }
    });
});

// Main game loop
function gameLoop() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Roast and draw marshmallows
    marshmallows.forEach(marshmallow => {
        marshmallow.roast();
        marshmallow.draw();
    });

    // Gradually increase global roasting speed to make the game harder
    globalSpeedMultiplier += 0.0005;

    requestAnimationFrame(gameLoop);
}

// Start the game
createMarshmallows();
gameLoop();

