#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_ADDR 0x3C

#define VRX_PIN 26  // Eixo X do joystick
#define VRY_PIN 27  // Eixo Y do joystick
#define BUTTON_A 5
#define BUTTON_B 6

#define PLATFORM_WIDTH 128
#define PLATFORM_HEIGHT 8
#define PLATFORM_Y 56
#define SQUARE_SIZE 8
#define GRAVITY 1
#define JUMP_STRENGTH 10
#define MAX_FALL_SPEED 5

#define MAX_OBSTACLES 4  // Número máximo de obstáculos na tela
#define OBSTACLE_WIDTH 8
#define OBSTACLE_HEIGHT 8
#define OBSTACLE_SPEED 2

#define LED_PIN 13  // Pino GPIO para o LED vermelho

typedef struct {
    int x;
    bool active;
} Obstacle;

int square_x = 60, square_y = PLATFORM_Y - SQUARE_SIZE;
int velocity_y = 0;
bool is_jumping = false;
Obstacle obstacles[MAX_OBSTACLES];

ssd1306_t ssd;

void reset_game() {
    square_x = 60;
    square_y = PLATFORM_Y - SQUARE_SIZE;
    velocity_y = 0;
    is_jumping = false;

    // Inicializa obstáculos em posições diferentes
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].x = 128 + (i * 40); // Distância entre os obstáculos
        obstacles[i].active = true;
    }
}

void update_position(int new_x) {
    if (new_x >= 0 && new_x <= PLATFORM_WIDTH - SQUARE_SIZE) {
        square_x = new_x;
    }
}

void jump() {
    if (square_y == PLATFORM_Y - SQUARE_SIZE) {
        is_jumping = true;
        velocity_y = -JUMP_STRENGTH;
    }
}

// Verifica se há colisão entre o quadrado e um obstáculo
bool check_collision(Obstacle *obs) {
    return obs->active &&
           square_x < obs->x + OBSTACLE_WIDTH &&
           square_x + SQUARE_SIZE > obs->x &&
           square_y + SQUARE_SIZE > PLATFORM_Y - OBSTACLE_HEIGHT;
}

int main() {
    stdio_init_all();
    
    // Configura o pino do LED como saída
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // Inicia com o LED apagado
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_init(&ssd, 128, 64, false, I2C_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
    
    reset_game();
    
    uint16_t x_value;

    while (true) {
        if (gpio_get(BUTTON_A) == 0) {
            reset_game();
        }
        
        adc_select_input(1);
        x_value = adc_read();
        sleep_ms(2);
        
        int new_x = square_x;
        if (x_value > 3000) new_x += 2;
        else if (x_value < 1000) new_x -= 2;
        
        update_position(new_x);
        
        if (gpio_get(BUTTON_B) == 0) {
            jump();
        }
        
        velocity_y += GRAVITY;
        if (velocity_y > MAX_FALL_SPEED) {
            velocity_y = MAX_FALL_SPEED;
        }
        
        square_y += velocity_y;
        
        if (square_y >= PLATFORM_Y - SQUARE_SIZE) {
            square_y = PLATFORM_Y - SQUARE_SIZE;
            velocity_y = 0;
            is_jumping = false;
        }

        // Movimento e verificação dos obstáculos
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (obstacles[i].active) {
                obstacles[i].x -= OBSTACLE_SPEED;
                
                if (obstacles[i].x + OBSTACLE_WIDTH <= 0) {
                    // Se um obstáculo sair da tela, o jogo termina
                    reset_game();
                    break;
                }

                if (check_collision(&obstacles[i])) {
                    obstacles[i].active = false;  // Objeto "coletado"
                    
                    // Feedback visual: acende o LED
                    gpio_put(LED_PIN, 1);  // Acende o LED
                    sleep_ms(200);         // Mantém o LED aceso por 200 ms
                    gpio_put(LED_PIN, 0);  // Apaga o LED
                }
            }
        }

        // Renderização
        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, 0, PLATFORM_Y, PLATFORM_WIDTH, PLATFORM_HEIGHT, true, true); // Plataforma
        ssd1306_rect(&ssd, square_x, square_y, SQUARE_SIZE, SQUARE_SIZE, true, false); // Personagem
        
        // Desenhar obstáculos
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (obstacles[i].active) {
                ssd1306_rect(&ssd, obstacles[i].x, PLATFORM_Y - OBSTACLE_HEIGHT, OBSTACLE_WIDTH, OBSTACLE_HEIGHT, true, false);
            }
        }

        ssd1306_send_data(&ssd);
        
        sleep_ms(50);
    }
}