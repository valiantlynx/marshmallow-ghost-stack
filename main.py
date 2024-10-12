import pygame
import sys

# Initialize Pygame
pygame.init()

# Screen setup
screen_width, screen_height = 800, 600
screen = pygame.display.set_mode((screen_width, screen_height))
pygame.display.set_caption("Marshmallow Ghost Tower")
clock = pygame.time.Clock()

# Load marshmallow images (sprites should be named accordingly)
marshmallow_images = {
    "white": pygame.image.load("marshmallow_white.png"),
    "yellow": pygame.image.load("marshmallow_yellow.png"),
    "brown": pygame.image.load("marshmallow_brown.png"),
    "black": pygame.image.load("marshmallow_black.png")
}

# Load the platform image
platform_image = pygame.image.load("wooden_platform.png")

# Load the fire sprite for the bottom
fire_image = pygame.image.load("bonfire.png")

# Load sounds
click_sound = pygame.mixer.Sound("click-sound.mp3")
burn_sound = pygame.mixer.Sound("burn-sound.mp3")

# Score
score = 0
font = pygame.font.Font(None, 36)

# Class to represent marshmallows


class Marshmallow:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.roast_level = "white"  # initial state: white
        self.roast_timer = 0.0

    def draw(self, screen):
        screen.blit(marshmallow_images[self.roast_level], (self.x, self.y))

    def roast(self, dt):
        self.roast_timer += dt
        if 2 < self.roast_timer <= 4:
            self.roast_level = "yellow"
        elif 4 < self.roast_timer <= 6:
            self.roast_level = "brown"
        elif self.roast_timer > 6:
            self.roast_level = "black"

    def is_clicked(self, mouse_pos):
        return pygame.Rect(self.x, self.y, 40, 40).collidepoint(mouse_pos)

    def reset(self):
        self.roast_level = "white"
        self.roast_timer = 0.0


# Create marshmallows
marshmallows = [
    Marshmallow(200, 150),  # level 1, corner 1
    Marshmallow(600, 150),  # level 1, corner 2
    Marshmallow(200, 300),  # level 2, corner 1
    Marshmallow(600, 300)   # level 2, corner 2
]

# Game loop


def draw_score():
    global score
    score_text = font.render(f"Score: {score}", True, (0, 0, 0))
    screen.blit(score_text, (10, 10))


while True:
    dt = clock.tick(60) / 1000.0  # Delta time in seconds
    screen.fill((255, 255, 255))  # Clear screen

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

    # Get mouse position
    mouse_pos = pygame.mouse.get_pos()

    # Draw the bonfire
    screen.blit(fire_image, (screen_width // 2 - 50, screen_height - 100))

    # Roast marshmallows and check for clicks
    for marshmallow in marshmallows:
        marshmallow.roast(dt)
        marshmallow.draw(screen)

        if pygame.mouse.get_pressed()[0]:
            if marshmallow.is_clicked(mouse_pos):
                if marshmallow.roast_level == "brown":
                    score += 5
                    click_sound.play()
                elif marshmallow.roast_level == "yellow":
                    score += 1
                    click_sound.play()
                elif marshmallow.roast_level == "black":
                    score -= 2
                    burn_sound.play()
                marshmallow.reset()  # Reset marshmallow after being clicked

    # Draw score
    draw_score()

    # Update display
    pygame.display.flip()
