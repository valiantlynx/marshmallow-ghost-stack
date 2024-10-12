import pygame
import sys

# Initialize Pygame
pygame.init()

# Screen setup
screen_width, screen_height = 800, 600
screen = pygame.display.set_mode((screen_width, screen_height))
pygame.display.set_caption("Marshmallow Roasting Game")
clock = pygame.time.Clock()

# Load sounds
click_sound = pygame.mixer.Sound("click-sound.mp3")
burn_sound = pygame.mixer.Sound("burn-sound.mp3")

# Colors
WHITE = (255, 255, 255)
YELLOW = (255, 255, 0)
BROWN = (139, 69, 19)
BLACK = (0, 0, 0)

# Score
score = 0
font = pygame.font.Font(None, 36)

# Marshmallow class


class Marshmallow:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.size = 40
        self.roast_level = 0  # 0: white, 1: yellow, 2: brown, 3: black
        self.roast_timer = 0.0

    def draw(self, screen):
        color = WHITE
        if self.roast_level == 1:
            color = YELLOW
        elif self.roast_level == 2:
            color = BROWN
        elif self.roast_level == 3:
            color = BLACK

        pygame.draw.circle(screen, color, (self.x, self.y), self.size)

    def roast(self, dt):
        self.roast_timer += dt
        if 2 < self.roast_timer <= 4:
            self.roast_level = 1  # yellow
        elif 4 < self.roast_timer <= 6:
            self.roast_level = 2  # brown
        elif self.roast_timer > 6:
            self.roast_level = 3  # black

    def is_clicked(self, mouse_pos):
        dist = ((self.x - mouse_pos[0]) ** 2 +
                (self.y - mouse_pos[1]) ** 2) ** 0.5
        return dist < self.size

    def reset(self):
        self.roast_level = 0
        self.roast_timer = 0.0


# Initialize marshmallows
marshmallows = [
    Marshmallow(200, 150),
    Marshmallow(400, 150),
    Marshmallow(600, 150),
    Marshmallow(300, 300),
    Marshmallow(500, 300)
]

# Game loop


def draw_score():
    global score
    score_text = font.render(f"Score: {score}", True, (0, 0, 0))
    screen.blit(score_text, (10, 10))


while True:
    dt = clock.tick(60) / 1000.0  # Delta time in seconds
    screen.fill(WHITE)  # Clear screen

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

    # Get mouse position
    mouse_pos = pygame.mouse.get_pos()

    # Roast marshmallows and check for clicks
    for marshmallow in marshmallows:
        marshmallow.roast(dt)
        marshmallow.draw(screen)

        if pygame.mouse.get_pressed()[0]:
            if marshmallow.is_clicked(mouse_pos):
                if marshmallow.roast_level == 2:  # Perfectly roasted
                    score += 5
                    click_sound.play()
                elif marshmallow.roast_level == 1:  # Under-roasted
                    score += 1
                    click_sound.play()
                elif marshmallow.roast_level == 3:  # Burnt
                    score -= 2
                    burn_sound.play()
                marshmallow.reset()  # Reset marshmallow after being clicked

    # Draw score
    draw_score()

    # Update display
    pygame.display.flip()
