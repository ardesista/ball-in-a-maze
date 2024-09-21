#include "debug.h"
#include "lcd.h"
#include "splash.h"
#include "adxl345.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ch32v30x_rng.h"

#define FIELD_X0            0
#define FIELD_Y0            32
#define FIELD_X1            (LCD_WIDTH - 1)
#define FIELD_Y1            (LCD_HEIGHT - 1)
#define FIELD_WIDTH         (FIELD_X1 - FIELD_X0 + 1)
#define FIELD_HEIGHT        (FIELD_Y1 - FIELD_Y0 + 1)

#define HUD_BGCOLOR         COLOR_FROMRGB(38, 38, 38)

#define BALL_RADIUS         8
#define BALL_ACCEL_SCALE    16.0f
#define BALL_BOUNCINESS     .75f
#define BALL_DAMPING        1.5f

#define TARGET_EASY         0
#define TARGET_EASY_COLOR   COLOR_FROMRGB(0, 192, 0)
#define TARGET_EASY_PROB    (100 - TARGET_HARD_PROB - TARGET_MID_PROB) // on 100
#define TARGET_EASY_ASCORE  .5f
#define TARGET_EASY_RMIN    25
#define TARGET_EASY_RMAX    38
#define TARGET_EASY_TMIN    5
#define TARGET_EASY_TMAX    12

#define TARGET_MID          1
#define TARGET_MID_COLOR    COLOR_FROMRGB(255, 255, 0)
#define TARGET_MID_PROB     30 // on 100
#define TARGET_MID_ASCORE   1.5f
#define TARGET_MID_RMIN     18
#define TARGET_MID_RMAX     24
#define TARGET_MID_TMIN     3
#define TARGET_MID_TMAX     10

#define TARGET_HARD         2
#define TARGET_HARD_COLOR   COLOR_FROMRGB(255, 32, 32)
#define TARGET_HARD_PROB    15 // on 100
#define TARGET_HARD_ASCORE  8.0f
#define TARGET_HARD_RMIN    14
#define TARGET_HARD_RMAX    17
#define TARGET_HARD_TMIN    3
#define TARGET_HARD_TMAX    5

#define SCORE_DECEL         -1.5f
#define SCORE_DAMPING       .5f
#define SCORE_VTERM_MIN     (SCORE_DECEL / SCORE_DAMPING)
#define SCORE_VTERM_MAX     (TARGET_HARD_ASCORE / SCORE_DAMPING)

#define PHYSICS_FPS          30
#define PHYSICS_DT           (1.f / PHYSICS_FPS)

// Physics step
void TIM2_IRQHandler(void) __attribute__((interrupt("machine")));

static volatile uint32_t frameno = 0;


enum {
    PALETTE_BACKGROUND,
    PALETTE_TARGET,
    PALETTE_BALL,
    PALETTE_BALL_OUTLINE
};

struct {
    fb_t fb;
    uint16_t palette[4];
    uint8_t data[(FIELD_WIDTH * FIELD_HEIGHT) >> 2];
} field_data = {
    .fb = {
        .width   = FIELD_WIDTH,
        .height  = FIELD_HEIGHT,
        .depth   = DEPTH_2BPP,
        .flags   = 0
    },
    .palette = {
        COLOR_BLACK,
        COLOR_GREEN,
        COLOR_GRAY,
        COLOR_BLUE
    }
};

fb_t* field_fb = (fb_t*)&field_data;

typedef struct {
    float x;
    float y;
} vec2_t;

typedef struct {
    vec2_t pos;
    vec2_t vel;
} kinematic_t;

float vec2_dist(const vec2_t* a, const vec2_t* b);

static volatile kinematic_t ball = {
    .pos = { .x = .5f * LCD_WIDTH, .y = .5f * LCD_HEIGHT },
    .vel = { .x = .0f, .y = .0f }
};

static volatile vec2_t target_pos;
static volatile int16_t target_difficulty;
static volatile int16_t target_radius;
static volatile uint32_t target_respawn_frameno;
static volatile float target_ascore = .0f;
static volatile uint32_t ball_in_target = 0;

static volatile float score = .0f, vscore = .0f;
static volatile int32_t sscore = -1, sscore_record = -1;

void rng_init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, ENABLE);
    RNG_Cmd(ENABLE);
}

uint32_t rng_rand(void)
{
    while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET);
    return RNG_GetRandomNumber();
}

void target_respawn(void)
{
int32_t p = rng_rand() % 100;
int32_t sec_min, sec_max;
int16_t radius_min, radius_max;

    if(p < TARGET_HARD_PROB) {
        target_difficulty = TARGET_HARD;
        target_ascore = TARGET_HARD_ASCORE;
        radius_min = TARGET_HARD_RMIN;
        radius_max = TARGET_HARD_RMAX;
        sec_min = TARGET_HARD_TMIN;
        sec_max = TARGET_HARD_TMAX;
    }
    else if(p < (TARGET_HARD_PROB + TARGET_MID_PROB)) {
        target_difficulty = TARGET_MID;
        target_ascore = TARGET_MID_ASCORE;
        radius_min = TARGET_MID_RMIN;
        radius_max = TARGET_MID_RMAX;
        sec_min = TARGET_MID_TMIN;
        sec_max = TARGET_MID_TMAX;
    }
    else {
        target_difficulty = TARGET_EASY;
        target_ascore = TARGET_EASY_ASCORE;
        radius_min = TARGET_EASY_RMIN;
        radius_max = TARGET_EASY_RMAX;
        sec_min = TARGET_EASY_TMIN;
        sec_max = TARGET_EASY_TMAX;
    }

    target_radius = radius_min + rng_rand() % (radius_max - radius_min);
    target_respawn_frameno = frameno + sec_min * PHYSICS_FPS + rng_rand() % ((sec_max - sec_min) * PHYSICS_FPS);
    target_pos.x = target_radius + rng_rand() % (FIELD_WIDTH - 2 * target_radius);
    target_pos.y = target_radius + rng_rand() % (FIELD_HEIGHT - 2 * target_radius);
}

void physics_start(void)
{
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_CounterModeConfig(TIM2, TIM_CounterMode_Up);
    TIM_SetAutoreload(TIM2, 18000 / PHYSICS_FPS);
    TIM_PrescalerConfig(TIM2, 8000 - 1, TIM_PSCReloadMode_Immediate);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

void status_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_ResetBits(GPIOA, GPIO_Pin_3);
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

int main(void)
{
int32_t i;
int16_t x, y, r;
char buf[24];

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	Delay_Init();
    rng_init();
    status_led_init();
    lcd_init();

    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_RED);
    adxl345_init();

    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    lcd_draw_image((LCD_WIDTH - splash_WIDTH) >> 1, ((LCD_HEIGHT - splash_HEIGHT) >> 1) - 18, splash_WIDTH, splash_HEIGHT, splash);
    lcd_puts((LCD_WIDTH >> 1) - 6 * 17, ((LCD_HEIGHT + splash_HEIGHT) >> 1) - 6, "Ridiculous Glitch", COLOR_FROMRGB(0xff, 0xaf, 0x1c), COLOR_BLACK);
    Delay_Ms(3000);

    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    lcd_fill_rect(0, 0, LCD_WIDTH, FIELD_Y0, HUD_BGCOLOR);

    target_respawn();
    physics_start();

    // Display loop
	while(1) {
        // Directly draw HUD on LCD
        snprintf(buf, sizeof(buf) - 1, "%-8ld", sscore);
        lcd_puts(8, 8, buf, COLOR_WHITE, HUD_BGCOLOR);
        snprintf(buf, sizeof(buf) - 1, "%8ld", sscore_record);
        lcd_puts(LCD_WIDTH - 104, 8, buf, COLOR_RED, HUD_BGCOLOR);
        // snprintf(buf, sizeof(buf) - 1, "%05ld", frameno / PHYSICS_FPS);
        // lcd_puts(LCD_WIDTH / 2 - 30, 8, buf, COLOR_WHITE, HUD_BGCOLOR);

        // Draw game field through frame buffer
        fb_fill(field_fb, PALETTE_BACKGROUND);

        x = target_pos.x;
        y = target_pos.y;
        r = target_radius;

        switch(target_difficulty) {
            default:
            case TARGET_EASY:
                field_data.palette[PALETTE_TARGET] = TARGET_EASY_COLOR;
                break;
            case TARGET_MID:
                field_data.palette[PALETTE_TARGET] = TARGET_MID_COLOR;
                break;
            case TARGET_HARD:
                field_data.palette[PALETTE_TARGET] = TARGET_HARD_COLOR;
                break;
        }

        if(ball_in_target) {
            for(i = (r * r) >> 5; i >= 0; --i) {
                float m = (rand() & 0xffff) / 65536.0f;
                float a = (rand() & 0xffff) / 32768.0f * M_PI;
                fb_setpixel(field_fb, x + r * m * cosf(a), y + r * m * sinf(a), PALETTE_TARGET);
            }
        }

        fb_draw_circle(field_fb, x, y, target_radius, PALETTE_TARGET);

        uint8_t c = (24.f + 230.f * (vscore - SCORE_VTERM_MIN) / (SCORE_VTERM_MAX - SCORE_VTERM_MIN));
        field_data.palette[PALETTE_BALL] = COLOR_FROMRGB(c, c, 255);
        fb_fill_circle(field_fb, ball.pos.x, ball.pos.y, BALL_RADIUS, PALETTE_BALL);
        fb_draw_circle(field_fb, ball.pos.x, ball.pos.y, BALL_RADIUS, ball_in_target ? PALETTE_TARGET : PALETTE_BALL_OUTLINE);

        lcd_draw_fb(FIELD_X0, FIELD_Y0, field_fb);
    }
}

void TIM2_IRQHandler(void)
{
float ax, ay, ascore;
int16_t x, y, z;

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    // Read accelerometer
    adxl345_read_accel(&x, &y, &z);
    ax = (float)y * -BALL_ACCEL_SCALE; // * -0.00390625f;
    ay = (float)x * -BALL_ACCEL_SCALE; // * 0.00390625f;

    // Move ball
    ball.vel.x += (ax - ball.vel.x * BALL_DAMPING) * PHYSICS_DT;
    ball.pos.x += ball.vel.x * PHYSICS_DT;
    if(ball.pos.x < BALL_RADIUS) {
        ball.pos.x = BALL_RADIUS;
        ball.vel.x = -BALL_BOUNCINESS * ball.vel.x;
    }
    else if(ball.pos.x + BALL_RADIUS >= FIELD_WIDTH) {
        ball.pos.x = FIELD_WIDTH - BALL_RADIUS - 1;
        ball.vel.x = -BALL_BOUNCINESS * ball.vel.x;
    }

    ball.vel.y += (ay - ball.vel.y * BALL_DAMPING) * PHYSICS_DT;
    ball.pos.y += ball.vel.y * PHYSICS_DT;
    if(ball.pos.y < BALL_RADIUS) {
        ball.pos.y = BALL_RADIUS;
        ball.vel.y = -BALL_BOUNCINESS * ball.vel.y;
    }
    else if(ball.pos.y + BALL_RADIUS >= FIELD_HEIGHT) {
        ball.pos.y = FIELD_HEIGHT - BALL_RADIUS - 1;
        ball.vel.y = -BALL_BOUNCINESS * ball.vel.y;
    }

    // Update target
    if(frameno >= target_respawn_frameno)
        target_respawn();

    // Check
    ball_in_target = (vec2_dist(&(ball.pos), &target_pos) < target_radius);

    // Update score
    ascore = (ball_in_target ? target_ascore : SCORE_DECEL);
    vscore += (ascore - vscore * SCORE_DAMPING) * PHYSICS_DT;
    score += vscore * PHYSICS_DT;
    if(score < .0f) {
        score = .0f;
        vscore = .0f;
    }

    sscore = (int32_t)score;
    if(sscore > sscore_record)
        sscore_record = sscore;

    ++frameno;
}

float vec2_dist(const vec2_t* a, const vec2_t* b) 
{
    float dx = a->x - b->x, dy = a->y - b->y;
    return sqrtf(dx * dx + dy * dy);
}
